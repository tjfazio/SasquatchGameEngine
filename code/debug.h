#pragma once

enum LogLevel
{
    Debug_Verbose = 0,
    Debug_Information = 1,
    Debug_Warning = 2,
    Debug_Error = 3,
    Debug_Critical = 4
};

void Debug_InitializeLog(LogLevel minLogLevel, char * fileName);

void Debug_Log(LogLevel level, char *text);