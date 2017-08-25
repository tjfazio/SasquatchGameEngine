#include <dsound.h>

#include "debug.h"
#include "win32_sound.h"
#include "sasquatch.h"

namespace Sasquatch { namespace Platform
{
    void Win32_InitializeDirectSound(
        Win32_SoundOutput *soundOutput,
        HWND hwnd,
        int32_t soundBufferSize,
        int32_t samplesPerSecond)
    {
        HMODULE directSoundLib = LoadLibrary("dsound.dll");
        if (!directSoundLib)
        { 
            Debug::Log(Debug::Error, L"Failed to load dsound.dll");
            return;
        }

        DirectSoundCreate_T directSoundCreate = (DirectSoundCreate_T)GetProcAddress(directSoundLib, "DirectSoundCreate");
        LPDIRECTSOUND directSound;
        if (!directSoundCreate || !SUCCEEDED(directSoundCreate(NULL, &directSound, NULL)))
        {
            Debug::Log(Debug::Error, L"Failed to create DirectSound structure");
            return;
        }
        if (!SUCCEEDED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)))
        {
            Debug::Log(Debug::Error, L"Failed to set DirectSound cooperative level");
            return;
        }

        DSBUFFERDESC primaryBufferDescription = {};
        primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
        primaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
        primaryBufferDescription.dwBufferBytes = 0;
        primaryBufferDescription.dwReserved = 0;
        primaryBufferDescription.lpwfxFormat = NULL;
        primaryBufferDescription.guid3DAlgorithm = GUID_NULL;
        if (!SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDescription, &soundOutput->PrimaryBuffer, NULL)))
        {
            Debug::Log(Debug::Error, L"Failed to create primary DirectSound buffer");
            return;
        }

        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = Win32_NumChannels;
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.wBitsPerSample = sizeof(sample_t) * 8;
        waveFormat.nBlockAlign = waveFormat.nChannels * sizeof(sample_t);
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;
        if (!SUCCEEDED(soundOutput->PrimaryBuffer->SetFormat(&waveFormat)))
        {
            Debug::Log(Debug::Error, L"Failed to set DirectSound format");
            return;
        }

        DSBUFFERDESC secondaryBufferDescription;
        secondaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
        secondaryBufferDescription.dwFlags = 0;
        secondaryBufferDescription.dwBufferBytes = soundBufferSize;
        secondaryBufferDescription.dwReserved = 0;
        secondaryBufferDescription.lpwfxFormat = &waveFormat;
        secondaryBufferDescription.guid3DAlgorithm = GUID_NULL;
        if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDescription, &soundOutput->SecondaryBuffer, NULL)))
        {
            Debug::Log(Debug::Error, L"Failed to create secondary DirectSound buffer");
            return;
        }
        soundOutput->SoundBufferSize = soundBufferSize;
    }

    void Win32_ClearSoundBuffer(Win32_SoundOutput *soundOutput)
    {
        void *soundBufferSection1;
        DWORD soundBytes1;
        void *soundBufferSection2;
        DWORD soundBytes2;
        HRESULT lockResult = soundOutput->SecondaryBuffer->Lock(
                    0, soundOutput->SoundBufferSize,
                    &soundBufferSection1, &soundBytes1, 
                    &soundBufferSection2, &soundBytes2,
                    0);
        if (SUCCEEDED(lockResult))
        {
            assert(soundBytes2 == 0);
            for (uint32_t i = 0; i < soundBytes1; i++)
            {
                (*(uint8_t*)soundBufferSection1) = 0;
            }
            HRESULT unlockResult =
                soundOutput->SecondaryBuffer->Unlock(
                    soundBufferSection1, soundBytes1,
                    soundBufferSection2, soundBytes2);
            if (!SUCCEEDED(unlockResult))
            {
                Debug::Log(Debug::Error, L"Failed to unlock buffer when clearing it");
            }
        }
        else
        {
            Debug::Log(Debug::Warning, L"Failed to lock and clear buffer");
        }
    }


    internal void Win32_OutputSoundSamples(
        GameClock *gameClock,
        GameState *gameState,
        Win32_SoundOutput *soundOutput, 
        SoundBuffer *gameSoundBuffer)
    {
        const int debugMessageMaxLength = 255;
        wchar_t debugMessage[debugMessageMaxLength];
        size_t debugMessageBufferSize = debugMessageMaxLength * sizeof(wchar_t);

        DWORD playCursor;
        DWORD writeCursor;
        if(SUCCEEDED(soundOutput->SecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
        {
            if (!soundOutput->IsSoundValid)
            {
                Debug::Log(Debug::Warning, L"Resetting write cursor");
                soundOutput->Cursor = writeCursor;
            } 
            StringCbPrintfW(debugMessage, debugMessageBufferSize, L"Play:%d\tWrite:%d\tCursor:%d", playCursor, writeCursor, soundOutput->Cursor);
            Debug::Log(Debug::Verbose, debugMessage);

            real32_t soundSeconds = gameClock->TargetFrameLengthSeconds + gameClock->FrameVariationSeconds;

            int32_t frameSoundBytes = (int32_t)(soundSeconds 
                * gameSoundBuffer->SamplesPerSecond 
                * sizeof(sample_t) 
                * gameSoundBuffer->NumChannels);
            int32_t targetCursor = playCursor + frameSoundBytes;
            if (targetCursor > soundOutput->SoundBufferSize)
            {
                targetCursor -= soundOutput->SoundBufferSize;
            }
            int32_t previousCursor = soundOutput->Cursor;
            int32_t bytesToWrite = targetCursor - previousCursor;
            if (bytesToWrite == 0)
            {
                Debug::Log(Debug::Warning, L"Play cursor hasn't advanced since last frame. Strange.");
                return;
            }
            if (bytesToWrite < 0)
            {
                // looped around
                bytesToWrite += soundOutput->SoundBufferSize;
            }
            
            if (bytesToWrite > gameSoundBuffer->BufferSize)
            {
                Debug::Log(Debug::Warning, L"Sound lagging behind, not enough space in buffer to catch up");
                bytesToWrite = gameSoundBuffer->BufferSize;
            }

            StringCbPrintfW(debugMessage, debugMessageBufferSize, L"Sound bytes to write: %d\n", bytesToWrite);
            OutputDebugStringW(debugMessage);

            assert(bytesToWrite % (gameSoundBuffer->NumChannels * sizeof(sample_t)) == 0);
            gameSoundBuffer->SampleCount = bytesToWrite / (gameSoundBuffer->NumChannels * sizeof(sample_t));

            GetSoundSamples(gameState, gameSoundBuffer);

            void *soundBufferSection1;
            DWORD soundBytes1;
            void *soundBufferSection2;
            DWORD soundBytes2;
            HRESULT lockResult = soundOutput->SecondaryBuffer->Lock(
                soundOutput->Cursor, bytesToWrite,
                &soundBufferSection1, &soundBytes1, 
                &soundBufferSection2, &soundBytes2,
                0);
            if(SUCCEEDED(lockResult))
            {
                int32_t sampleIndex = 0;
                int32_t section1SampleCount = soundBytes1 / sizeof(sample_t);
                sample_t *section1 = (sample_t *)soundBufferSection1;
                for (int i = 0; i < section1SampleCount; i++)
                {
                    section1[i] = gameSoundBuffer->Memory[sampleIndex];
                    sampleIndex++;
                }

                sample_t *section2 = (sample_t *)soundBufferSection2;
                int32_t section2SampleCount = soundBytes2 / sizeof(sample_t);
                for (int i = 0; i < section2SampleCount; i++)
                {
                    section2[i] = gameSoundBuffer->Memory[sampleIndex];
                    sampleIndex++;
                }

                soundOutput->Cursor = targetCursor;

                if (!SUCCEEDED(soundOutput->SecondaryBuffer->Unlock(
                        soundBufferSection1, soundBytes1,
                        soundBufferSection2, soundBytes2)))
                {                    
                    Debug::Log(Debug::Error, L"Sound bytes unlock failed\n");
                    soundOutput->IsSoundValid = false;
                }
                else
                {
                    soundOutput->IsSoundValid = true;
                }
            }
            else
            {                    
                Debug::Log(Debug::Error, L"Sound bytes lock failed\n");
                soundOutput->IsSoundValid = false;
            }
        }
        else
        {
            Debug::Log(Debug::Error, L"Get sound cursors failed\n");
            soundOutput->IsSoundValid = false;
        }
    }
}}