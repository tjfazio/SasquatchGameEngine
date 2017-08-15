#include <windows.h>

#include "debug.h"
#include "file.h"

typedef struct ReadFileOperation
{
    const wchar_t *FilePath;
    void *DestBuffer;
    int32_t DestBufferSize;
    int32_t IsReady;
    volatile LONG IsActive;
    SGE_ReadFileCallback Callback;
} ReadFileOperation;

const int32_t MaxOutstandingReads = 16;
const DWORD FileReadSleepMs = 250;

// TODO: implement priority queue
global_variable ReadFileOperation g_ReadOperations[MaxOutstandingReads];
HANDLE g_ReadFileThread;

static void RunCallbackAndClearOperation(
    ReadFileOperation& operation, 
    SGE_ReadFileCallbackArgs callbackArgs)
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

static DWORD ExecuteFileReads(LPVOID lpParam)
{
    const int debugMessageMaxLength = 512;
    wchar_t debugMessage[debugMessageMaxLength];

    while (true)
    {        
        ReadFileOperation *operation = nullptr;
        for (int i = 0; i < MaxOutstandingReads; i++)
        {
            if (g_ReadOperations[i].IsActive)
            {
                operation = &g_ReadOperations[i];
                break;
            }
        }

        if (operation == nullptr)
        {
            SleepEx(FileReadSleepMs, true);
            continue;
        }

        SGE_ReadFileCallbackArgs failedCallbackArgs;
        failedCallbackArgs.Success = 0;
        failedCallbackArgs.FilePath = operation->FilePath;
        failedCallbackArgs.DestBuffer = operation->DestBuffer;
        failedCallbackArgs.BytesRead = 0;

        HANDLE fileHandle = CreateFileW(
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
            StringCbPrintfW(debugMessage, debugMessageMaxLength, L"Failed to open file %s to read: %d", 
                operation->FilePath, 
                GetLastError());
            Debug_Log(Debug_Error, debugMessage);
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
            StringCbPrintfW(debugMessage, debugMessageMaxLength, L"ReadFileEx call for %s failed: %d", 
                operation->FilePath, 
                GetLastError());
            Debug_Log(Debug_Error, debugMessage);
            RunCallbackAndClearOperation(*operation, failedCallbackArgs);
            continue;
        }

        SGE_ReadFileCallbackArgs callbackArgs;
        callbackArgs.Success = TRUE;
        callbackArgs.FilePath = operation->FilePath;
        callbackArgs.DestBuffer = operation->DestBuffer;
        callbackArgs.DestBufferSize = operation->DestBufferSize;
        callbackArgs.BytesRead = bytesRead;

        RunCallbackAndClearOperation(*operation, failedCallbackArgs);
    } 
}

BOOL SGE_InitializeFileSystem()
{
    HANDLE threadHandle = CreateThread(
        NULL,
        0,
        ExecuteFileReads,
        NULL,
        0,
        NULL
    );
    if (!threadHandle)
    {
        Debug_Log(Debug_Error, L"File read thread creation failed!");
        return false;
    }
    g_ReadFileThread = threadHandle;
    return true;
}

void SGE_ReadFile(
    const wchar_t * filePath, 
    void *destBuffer, 
    int32_t destBufferSize, 
    SGE_ReadFileCallback callback)
{
    SGE_ReadFileCallbackArgs failedCallbackArgs;
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
        Debug_Log(Debug_Error, L"ReadFile: Already at max outstanding file read operations");
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