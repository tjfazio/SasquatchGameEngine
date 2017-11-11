#pragma once

#include <dsound.h>

#include "sasquatch.h"

namespace Sasquatch { namespace Platform
{
    // function pointers for DLLs
    typedef HRESULT (*DirectSoundCreate_T)(LPCGUID lpcGuidDevice, LPDIRECTSOUND * ppDS, LPUNKNOWN pUnkOuter);

    struct Win32_SoundOutput {
        int32_t SoundBufferSize;
        int32_t Cursor;
        int32_t LatencySampleCount;
        bool IsSoundValid;
        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        LPDIRECTSOUNDBUFFER SecondaryBuffer;
    };

    const int32_t Win32_SoundSamplesPerSecond = 44100;
    const int32_t Win32_NumChannels = 2;
    const int32_t Win32_SoundBufferSize = 2 * Win32_NumChannels * sizeof(sample_t) * Win32_SoundSamplesPerSecond;

    void Win32_InitializeDirectSound(
        Win32_SoundOutput *soundOutput,
        HWND hwnd,
        int32_t soundBufferSize,
        int32_t samplesPerSecond);

    void Win32_ClearSoundBuffer(Win32_SoundOutput *soundOutput);

    void Win32_OutputSoundSamples(
        GameClock *gameClock,
        GameState *gameState,
        Win32_SoundOutput *soundOutput, 
        SoundBuffer *gameSoundBuffer);
}}