#include <windows.h>
#include <stdint.h>
#include <assert.h>

#include "common.h"
#include "debug.h"

const int64_t Debug_LogBufferSize = 1024;
const int64_t Debug_FileBufferSize = KILOBYTES(64);
const int32_t Debug_MaxLogRetries = 10;
const char Debug_MagicValue0 = (char)0xba;
const char Debug_MagicValue1 = (char)0xce;
const DWORD Debug_ThreadSleepMs = 15;

global_variable wchar_t g_NewLine[2];

typedef struct Debug_LogFile
{
    volatile LONG BufferWriteCursor;
    volatile LONG FileWriteCursor;
    LogLevel MinLogLevel;
    HANDLE File;
    uint8_t FileInitialized;
    int32_t FileBufferSize;
    void *FileBufferMemory;
} Debug_LogFile;

typedef struct Debug_LogHeader
{
    uint16_t Magic;
    uint16_t Length;
} Debug_LogHeader;

global_variable Debug_LogFile g_LogFile;
global_variable HANDLE g_LogThreadHandle;
global_variable DWORD g_LogThreadId;

DWORD WINAPI Debug_FlushLogBufferToFile(LPVOID lpParam);

void Debug_InitializeLog(LogLevel minLogLevel, char * fileName)
{
    g_NewLine[0] = L'\r';
    g_NewLine[1] = L'\n';

    g_LogFile.MinLogLevel = minLogLevel;
    g_LogFile.File = CreateFile(
        fileName,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (g_LogFile.File != INVALID_HANDLE_VALUE)
    {
        g_LogFile.FileBufferMemory = VirtualAlloc(NULL, Debug_FileBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        g_LogFile.FileBufferSize = Debug_FileBufferSize;
        if (g_LogFile.FileBufferMemory != NULL)
        {
            g_LogFile.FileInitialized = true;
        }     
    }

    g_LogThreadHandle = CreateThread(
        NULL,
        0,
        Debug_FlushLogBufferToFile,
        &g_LogFile,
        0,
        &g_LogThreadId
    );
    if (!g_LogThreadHandle)
    {
        OutputDebugStringW(L"Log thread initialization failed!");
    }
}

void Debug_Log(LogLevel level, wchar_t *text)
{
    wchar_t debugLogBuffer[Debug_LogBufferSize];
    // TODO: add timestamp
    StringCbPrintfW(debugLogBuffer, Debug_LogBufferSize, L"%d\t%s\r\n", level, text);
    OutputDebugStringW(debugLogBuffer);
    if (g_LogFile.FileInitialized 
        && (int)level >= (int)g_LogFile.MinLogLevel)
    {
        uint16_t logSize = (uint16_t)(wcslen(debugLogBuffer) * sizeof(wchar_t));
        int32_t bytesToWrite = sizeof(Debug_LogHeader) + logSize;
        int32_t estimatedFreeBytes = g_LogFile.FileWriteCursor - g_LogFile.BufferWriteCursor;
        if (g_LogFile.FileWriteCursor <= g_LogFile.BufferWriteCursor)
        {
            estimatedFreeBytes += Debug_FileBufferSize;
        }
        if (bytesToWrite < estimatedFreeBytes)
        {
            LONG currentCursor = g_LogFile.BufferWriteCursor;
            LONG targetCursor = currentCursor + bytesToWrite;
            if (targetCursor > g_LogFile.FileBufferSize)
            {
                targetCursor -= g_LogFile.FileBufferSize;
            }
            bool reserveSuccess = false;
            for (int i = 0; i < Debug_MaxLogRetries; i++)
            {
                if (currentCursor == InterlockedCompareExchange(&g_LogFile.BufferWriteCursor, targetCursor, currentCursor))
                {
                    reserveSuccess = true;
                    break;
                }
            }
            if (reserveSuccess)
            {
                char *targetBuffer = (char *)g_LogFile.FileBufferMemory;
                char *sourceBuffer = (char *)debugLogBuffer;
                LONG firstStart = (currentCursor + sizeof(Debug_LogHeader)) % g_LogFile.FileBufferSize;
                LONG firstEnd = firstStart + logSize;
                if (firstEnd > g_LogFile.FileBufferSize)
                {
                    firstEnd = g_LogFile.FileBufferSize;
                }
                LONG firstLength = firstEnd - firstStart;
                for (int i = 0; i < firstLength; i++)
                {
                    targetBuffer[firstStart + i] = sourceBuffer[i];
                }

                LONG secondEnd = logSize - firstLength;
                for (int i = 0; i < secondEnd; i++)
                {
                    targetBuffer[i] = sourceBuffer[firstLength + i];
                }

                LONG sizeCursor = (currentCursor + sizeof(uint16_t)) % g_LogFile.FileBufferSize;
                targetBuffer[sizeCursor] = (char)(logSize >> 8);
                targetBuffer[sizeCursor + 1] = (char)(logSize);
                
                targetBuffer[currentCursor] = Debug_MagicValue0;
                targetBuffer[currentCursor + 1] = Debug_MagicValue1;
            }
            else
            {
                OutputDebugStringW(L"DebugLog: Unable to reserve space after retrying!");
            }
        }
    }
    else
    {
        OutputDebugStringW(L"DebugLog: Not enough buffer space!");
    }
}

DWORD WINAPI Debug_FlushLogBufferToFile(LPVOID lpParam)
{
    Debug_LogFile *logFile = (Debug_LogFile *)lpParam;
    while (true)
    {
        char *buffer = (char *)logFile->FileBufferMemory;
        if (logFile->BufferWriteCursor != logFile->FileWriteCursor)
        {
            LONG currentCursor = logFile->FileWriteCursor;
            if (buffer[currentCursor] == Debug_MagicValue0 
                && buffer[currentCursor + 1] == Debug_MagicValue1)
            {
                buffer[currentCursor] = 0;
                buffer[currentCursor + 1] = 0;

                LONG sizeCursor = (currentCursor + 2*sizeof(char)) % Debug_FileBufferSize;
                uint16_t logSizeHigh = ((uint16_t)buffer[sizeCursor]) << 8;
                uint16_t logSize = logSizeHigh | (uint16_t)buffer[sizeCursor + 1];
                buffer[sizeCursor] = 0;
                buffer[sizeCursor + 1] = 0;
                
                LONG firstStart = (sizeCursor + sizeof(uint16_t)) % Debug_FileBufferSize;
                LONG firstEnd = firstStart + logSize;
                if (firstEnd > Debug_FileBufferSize)
                {
                    firstEnd = Debug_FileBufferSize; 
                }
                LONG firstSize = firstEnd - firstStart;
                const void *firstBuffer = (const void *)&buffer[firstStart];
                DWORD bytesWritten;
                WriteFile(
                    logFile->File,
                    firstBuffer,
                    (DWORD)firstSize,
                    &bytesWritten,
                    NULL
                );
                assert(bytesWritten == (DWORD)firstSize);
                memset((void *)firstBuffer, 0, firstSize);
                
                LONG secondSize = (LONG)logSize - firstSize;
                if (secondSize > 0)
                {
                    const void *secondBuffer = (const void *)buffer;
                    WriteFile(
                        logFile->File,
                        secondBuffer,
                        (DWORD)secondSize,
                        &bytesWritten,
                        NULL
                    );
                    assert(bytesWritten == (DWORD)secondSize);
                    memset((void *)secondBuffer, 0, secondSize);
                }

                WriteFile(
                    logFile->File,
                    g_NewLine,
                    (DWORD)2,
                    &bytesWritten,
                    NULL
                );

                LONG updatedWriteCursor = (firstStart + logSize) % Debug_FileBufferSize;
                InterlockedExchange(&logFile->FileWriteCursor, updatedWriteCursor);
                FlushFileBuffers(logFile->File);
                continue;
            }            
        }
        Sleep(Debug_ThreadSleepMs);
    }
    return 0;
}