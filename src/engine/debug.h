#pragma once

namespace Sasquatch { namespace Debug 
{
    enum LogLevel
    {
        Verbose = 0,
        Information = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    void InitializeLog(LogLevel minLogLevel, char * fileName);

    void Log(LogLevel level, char *text);
} }