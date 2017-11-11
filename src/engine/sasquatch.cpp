#include <assert.h>

#include "common.h"
#include "sasquatch.h"
#include "input.h"


namespace
{
    using namespace Sasquatch;

    using namespace Sasquatch::Input;

    inline pixel_t GetFakePixel(GameState *gameState, int32_t x, int32_t y)
    {
        pixel_t blue = 0;
        if (x >= gameState->TestAnimation.XStart
            && x < gameState->TestAnimation.XStart + gameState->TestAnimation.BlueWidth)
        {
            blue = 0xFF;
        }   

        pixel_t green = 0;
        if (y >= gameState->TestAnimation.YStart
            && y < gameState->TestAnimation.YStart + gameState->TestAnimation.GreenWidth)
        {
            green = 0xFF;
        }

        pixel_t red = 0;

        return (red << 16) | (green << 8) | blue;
    }

    void Animate(GameState *gameState, VideoBuffer *videoBuffer)
    {
        int32_t width = videoBuffer->Width;
        int32_t height = -1 * videoBuffer->Height;
        int32_t stride = videoBuffer->Stride;
        pixel_t *pixels = videoBuffer->Memory;

        // Windows-specific logic
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                pixels[y * stride + x] = GetFakePixel(gameState, x, y);
            }
        }

        // fake game logic
        ButtonState upState;
        ButtonState downState;
        ButtonState leftState;
        ButtonState rightState;
        
        gameState->Controllers[0].GetState(Up, &upState);
        gameState->Controllers[0].GetState(Down, &downState);
        gameState->Controllers[0].GetState(Left, &leftState);
        gameState->Controllers[0].GetState(Right, &rightState);
        
        if (upState.EndedDown && !downState.EndedDown)
        {
            gameState->TestAnimation.YVelocity = -1;
        }
        else if (downState.EndedDown && !upState.EndedDown)
        {
            gameState->TestAnimation.YVelocity = 1;
        }
        else
        {
            gameState->TestAnimation.YVelocity = 0;
        }

        if (rightState.EndedDown && !leftState.EndedDown)
        {
            gameState->TestAnimation.XVelocity = 1;
        }
        else if (leftState.EndedDown && !rightState.EndedDown)
        {
            gameState->TestAnimation.XVelocity = -1;
        }
        else
        {
            gameState->TestAnimation.XVelocity = 0;
        }

        if ((gameState->TestAnimation.XStart < 0 && gameState->TestAnimation.XVelocity < 0)
            || (gameState->TestAnimation.XStart + gameState->TestAnimation.BlueWidth > width) && gameState->TestAnimation.XVelocity > 0)
        {
            gameState->TestAnimation.XVelocity = 0;
        }
        if ((gameState->TestAnimation.YStart < 0 && gameState->TestAnimation.YVelocity < 0)
            || (gameState->TestAnimation.YStart + gameState->TestAnimation.GreenWidth > height) 
            && gameState->TestAnimation.YVelocity > 0)
        {
            gameState->TestAnimation.YVelocity = 0;
        }
        gameState->TestAnimation.XStart += gameState->TestAnimation.XVelocity;
        gameState->TestAnimation.YStart += gameState->TestAnimation.YVelocity;
    }

    // fake game state
    global_variable uint32_t g_SamplePosition = 0;
    global_variable int32_t g_WaveFreq = 440;
    // THIS IS REALLY BAD BECAUSE THE DEVICE MIGHT HAVE A DIFFERENT SAMPLE RATE
    // SO THIS IS JUST DEBUG CODE OK
    const int32_t SamplesPerSecond = 44100;
    global_variable sample_t g_StereoSquareWave[2*SamplesPerSecond];
    const int32_t g_StereoSquareWaveSampleCount = 2 * (SamplesPerSecond / 4);
}

namespace Sasquatch
{
    void InitializeGameState(GameState *gameState)
    {    
        for (int i = 0; i < ControllerCount; i++)
        {
            gameState->Controllers[i].Initialize();
        }
        
        gameState->TestAnimation.XStart = 0;
        gameState->TestAnimation.XVelocity = 0;
        gameState->TestAnimation.YStart = 0;
        gameState->TestAnimation.YVelocity = 0;
        gameState->TestAnimation.BlueWidth = 16;
        gameState->TestAnimation.GreenWidth = 32;

        sample_t amp = 2000;
        int32_t sampleWaveLength = SamplesPerSecond / g_WaveFreq;
        for (int32_t i = 0; i < 2 * SamplesPerSecond; i++)
        {
            // flip signal every 2 * half_wavelength [L'R'L'R'L,R,L,R,]
            int32_t sampleIndex = i / sampleWaveLength;
            sample_t sample = ((sampleIndex % 2) == 0) ? amp : -amp;
            g_StereoSquareWave[i] = sample;
        }

        gameState->Sound.IsPlaying = false;
        gameState->Sound.SamplePosition = 0;
    }

    void UpdateAndRender(GameState *gameState, VideoBuffer *videoBuffer)
    {
        assert(gameState != NULL);
        assert(videoBuffer != NULL);

        Animate(gameState, videoBuffer);
    }

    void GetSoundSamples(GameState *gameState, SoundBuffer *soundBuffer)
    {
        assert(soundBuffer != NULL);
        assert(soundBuffer->NumChannels > 0);
        assert(soundBuffer->SampleCount >= 0);
        assert(soundBuffer->BufferSize >= (soundBuffer->NumChannels * soundBuffer->SampleCount * (int32_t)sizeof(sample_t)));
        
        ButtonState buttonState;
        gameState->Controllers[0].GetState(Action1, &buttonState);

        if (buttonState.TransitionCount > 0 && !gameState->Sound.IsPlaying)
        {
            gameState->Sound.IsPlaying = true;
            gameState->Sound.SamplePosition = 0;
        }
        
        int32_t channelSampleCount = soundBuffer->NumChannels * soundBuffer->SampleCount;
        for (int32_t i = 0; i < channelSampleCount; i++)
        {
            sample_t sample;
            if (gameState->Sound.IsPlaying)
            {
                sample = g_StereoSquareWave[gameState->Sound.SamplePosition];
                gameState->Sound.SamplePosition++;
                if (gameState->Sound.SamplePosition >= g_StereoSquareWaveSampleCount)
                {
                    gameState->Sound.IsPlaying = false;
                    gameState->Sound.SamplePosition = 0;
                }
            }
            else 
            {
                sample = 0;
            }
            soundBuffer->Memory[i] = sample;
        }
    }
}