#include <Windows.h>

#include "common.h"
#include "debug.h"

const int Debug_LogBufferSize = 4096;
global_variable wchar_t g_DebugLogBuffer[Debug_LogBufferSize];
global_variable HANDLE g_DebugLogFile;
global_variable bool g_DebugFileInitialized = false;
global_variable LogLevel g_MinLogLevel;

void Debug_InitializeLog(LogLevel minLogLevel, char * fileName)
{
    g_MinLogLevel = minLogLevel;
    g_DebugLogFile = CreateFile(
        fileName,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (g_DebugLogFile != INVALID_HANDLE_VALUE)
    {
        g_DebugFileInitialized = true;        
    }
}

void Debug_Log(LogLevel level, wchar_t *text)
{
    // TODO: add timestamp
    StringCbPrintfW(g_DebugLogBuffer, Debug_LogBufferSize, L"%d\t%s\n", level, text);
    OutputDebugStringW(g_DebugLogBuffer);
    if (g_DebugFileInitialized 
        && (int)level >= (int)g_MinLogLevel)
    {
        // TODO: write to file asynchronously
    }
}