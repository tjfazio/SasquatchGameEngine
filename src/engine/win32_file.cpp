#include <windows.h>
#include <assert.h>
#include <strsafe.h>

#include "debug.h"
#include "file.h"

namespace
{
    using namespace Sasquatch;
    using namespace Sasquatch::Platform;

    typedef struct ReadFileOperation
    {
        const char *FilePath;
        void *DestBuffer;
        int32_t DestBufferSize;
        int32_t IsReady;
        volatile LONG IsActive;
        ReadFileCallback Callback;
    } ReadFileOperation;
    
    const int32_t MaxOutstandingReads = 16;
    const DWORD FileReadSleepMs = 250;
    // TODO: implement priority queue
    ReadFileOperation g_ReadOperations[MaxOutstandingReads];
    HANDLE g_ReadFileThread;
    
    void RunCallbackAndClearOperation(
        ReadFileOperation& operation, 
        ReadFileCallbackArgs callbackArgs)
    {
        if (operation.Callback)
        {
            operation.Callback(callbackArgs);
        }
        operation.FilePath = NULL;
        operation.DestBuffer = NULL;
        operation.DestBufferSize = 0;
        operation.IsReady = 0;
        operation.Callback = NULL;
        int32_t prevActive = InterlockedExchange(&operation.IsActive, 0);
        assert(prevActive == 1);
    }
    
    static int32_t s_lastUsedOperationSlot = MaxOutstandingReads;

    DWORD ExecuteFileReads(LPVOID lpParam)
    {
        const int debugMessageMaxLength = 512;
        char debugMessage[debugMessageMaxLength];
    
        while (true)
        {        
            ReadFileOperation *operation = nullptr;
            int32_t index;
            for (int32_t i = 1; i <= MaxOutstandingReads; i++)
            {
                index = (s_lastUsedOperationSlot + i) % MaxOutstandingReads;
                if (g_ReadOperations[index].IsActive)
                {
                    operation = &g_ReadOperations[index];
                    break;
                }
            }            
    
            if (operation == nullptr)
            {
                SleepEx(FileReadSleepMs, true);
                continue;
            }

            s_lastUsedOperationSlot = index;
    
            ReadFileCallbackArgs failedCallbackArgs;
            failedCallbackArgs.Success = 0;
            failedCallbackArgs.FilePath = operation->FilePath;
            failedCallbackArgs.DestBuffer = operation->DestBuffer;
            failedCallbackArgs.BytesRead = 0;
    
            HANDLE fileHandle = CreateFileA(
                operation->FilePath,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                StringCbPrintfA(debugMessage, debugMessageMaxLength, "Failed to open file %s to read: %d", 
                    operation->FilePath, 
                    GetLastError());
                Debug::Log(Debug::Error, debugMessage);
                RunCallbackAndClearOperation(*operation, failedCallbackArgs);
                continue;
            }
    
            DWORD bytesRead;
            BOOL result = ReadFile(
                fileHandle,
                operation->DestBuffer,
                operation->DestBufferSize,
                &bytesRead,
                NULL
            );
            if (!result)
            {
                StringCbPrintfA(debugMessage, debugMessageMaxLength, "ReadFileEx call for %s failed: %d", 
                    operation->FilePath, 
                    GetLastError());
                Debug::Log(Debug::Error, debugMessage);
                RunCallbackAndClearOperation(*operation, failedCallbackArgs);
                continue;
            }
    
            ReadFileCallbackArgs callbackArgs;
            callbackArgs.Success = TRUE;
            callbackArgs.FilePath = operation->FilePath;
            callbackArgs.DestBuffer = operation->DestBuffer;
            callbackArgs.DestBufferSize = operation->DestBufferSize;
            callbackArgs.BytesRead = bytesRead;
    
            RunCallbackAndClearOperation(*operation, failedCallbackArgs);
        } 
    }
}

namespace Sasquatch { namespace Platform
{
    bool InitializeFileSystem()
    {
        HANDLE threadHandle = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)ExecuteFileReads,
            NULL,
            0,
            NULL
        );
        if (!threadHandle)
        {
            Debug::Log(Debug::Error, "File read thread creation failed!");
            return false;
        }
        g_ReadFileThread = threadHandle;
        return true;
    }

    void ReadFile(
        const char * filePath, 
        void *destBuffer, 
        int32_t destBufferSize, 
        ReadFileCallback callback)
    {
        ReadFileCallbackArgs failedCallbackArgs;
        failedCallbackArgs.Success = 0;
        failedCallbackArgs.FilePath = filePath;
        failedCallbackArgs.DestBuffer = destBuffer;
        failedCallbackArgs.BytesRead = 0;

        ReadFileOperation *operation = nullptr;
        int operationIndex;
        for (operationIndex = 0; operationIndex < MaxOutstandingReads; operationIndex++)
        {
            if (InterlockedCompareExchange(&g_ReadOperations[operationIndex].IsActive, 1, 0) == 0)
            {
                operation = &g_ReadOperations[operationIndex];
                break;
            }
        }
        if (operation == nullptr)
        {
            Debug::Log(Debug::Error, "ReadFile: Already at max outstanding file read operations");
            if (callback != nullptr)
            {
                callback(failedCallbackArgs);
            }
            return;        
        }
        
        operation->FilePath = filePath;
        operation->DestBuffer = destBuffer;
        operation->DestBufferSize = destBufferSize;
        operation->Callback = callback;
        operation->IsReady = 1;
    }
}}