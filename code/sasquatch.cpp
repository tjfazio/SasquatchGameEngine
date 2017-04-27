#include <assert.h>

#include "common.h"
#include "sasquatch.h"

inline pixel_t GetFakePixel(SGE_GameState *gameState, int32_t x, int32_t y)
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

internal void Animate(SGE_GameState *gameState, SGE_VideoBuffer *videoBuffer)
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
    if (gameState->Keyboard.IsSet(SGE_Up) && !gameState->Keyboard.IsSet(SGE_Down))
    {
        gameState->TestAnimation.YVelocity = -1;
    }
    else if (gameState->Keyboard.IsSet(SGE_Down) && !gameState->Keyboard.IsSet(SGE_Up))
    {
        gameState->TestAnimation.YVelocity = 1;
    }
    else
    {
        gameState->TestAnimation.YVelocity = 0;
    }

    if (gameState->Keyboard.IsSet(SGE_Right) && !gameState->Keyboard.IsSet(SGE_Left))
    {
        gameState->TestAnimation.XVelocity = 1;
    }
    else if (gameState->Keyboard.IsSet(SGE_Left) && !gameState->Keyboard.IsSet(SGE_Right))
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

void SGE_Init(SGE_GameState *gameState)
{    
    gameState->TestAnimation.XStart = 0;
    gameState->TestAnimation.XVelocity = 0;
    gameState->TestAnimation.YStart = 0;
    gameState->TestAnimation.YVelocity = 0;
    gameState->TestAnimation.BlueWidth = 16;
    gameState->TestAnimation.GreenWidth = 32;
}

void SGE_UpdateAndRender(SGE_GameState *gameState, SGE_VideoBuffer *videoBuffer)
{
    assert(gameState != NULL);
    assert(videoBuffer != NULL);

    Animate(gameState, videoBuffer);
}

// fake game state
global_variable uint32_t g_SamplePosition = 0;
global_variable int32_t g_WaveFreq = 440;

void SGE_GetSoundSamples(SGE_GameState *gameState, SGE_SoundBuffer *soundBuffer)
{
    assert(soundBuffer != NULL);
    assert(soundBuffer->BufferSize >= (soundBuffer->NumChannels * soundBuffer->SampleCount * sizeof(sample_t)));
    sample_t amp = gameState->Keyboard.IsSet(SGE_Action1) ? 2500 : 0;
    
    int32_t channelSampleCount = soundBuffer->NumChannels * soundBuffer->SampleCount;
    for (int32_t i = 0; i < channelSampleCount; i++)
    {
        int32_t sampleWaveLength = soundBuffer->SamplesPerSecond / g_WaveFreq;
        // flip signal every 2 * half_wavelength [L'R'L'R'L,R,L,R,]
        int32_t sampleIndex = g_SamplePosition / sampleWaveLength;
        sample_t sample = ((sampleIndex % 2) == 0) ? amp : -amp;
        soundBuffer->Memory[i] = sample;
        g_SamplePosition++;
    }
}