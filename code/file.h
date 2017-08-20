#pragma once

#include <stdint.h>

namespace Sasquatch { namespace Platform 
{
    typedef struct ReadFileCallbackArgs
    {
        int32_t Success;
        const wchar_t *FilePath;
        void *DestBuffer;
        int32_t DestBufferSize;
        int32_t BytesRead;
    } ReadFileCallbackArgs;

    typedef void (*ReadFileCallback)(ReadFileCallbackArgs);

    void ReadFile(
        const char * filePath, 
        void *destBuffer, 
        int32_t destBufferSize, 
        ReadFileCallback callback);
} }