#pragma once

#include <stdint.h>

typedef struct SGE_ReadFileCallbackArgs
{
    int32_t Success;
    const wchar_t *FilePath;
    void *DestBuffer;
    int32_t DestBufferSize;
    int32_t BytesRead;
} SGE_ReadFileCallbackArgs;

typedef void (*SGE_ReadFileCallback)(SGE_ReadFileCallbackArgs);

void SGE_ReadFile(
    const char * filePath, 
    void *destBuffer, 
    int32_t destBufferSize, 
    SGE_ReadFileCallback callback);