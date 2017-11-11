#pragma once

#include <stdint.h>

#include "common.h"
#include "input.h"
#include "ResourceHandle.h"
#include "ResourceCache.h"

#define pixel_t uint32_t
#define sample_t int16_t

namespace Sasquatch 
{
    const int32_t ControllerCount = 1;

    enum ControllerId
    {
        Keyboard = 0,
        GamePad1 = 1,
        GamePad2 = 2,
        GamePad3 = 3,
        GamePad4 = 4
    };

    typedef struct GameClock {    
        real32_t TimeDeltaSeconds;
        real32_t GameTimeSeconds;
        real32_t TargetFrameRate;
        real32_t TargetFrameLengthSeconds;
        real32_t FrameVariationSeconds;
    } GameClock;

    // RGB pixels layed out row-wise with given stride
    typedef struct VideoBuffer {
        int32_t Height;
        int32_t Width;
        int32_t Stride;
        pixel_t *Memory;
    } VideoBuffer;

    typedef struct SoundBuffer {
        int32_t NumChannels;
        int32_t SamplesPerSecond;
        int32_t SampleCount;
        int32_t BufferSize;
        sample_t *Memory;
    } SoundBuffer; 

    // Fake gameplay stuff for prototyping
    typedef struct AnimationState {
        int32_t XStart;
        int32_t XVelocity;
        int32_t YStart;
        int32_t YVelocity;
        int32_t BlueWidth;
        int32_t GreenWidth;
    } AnimationState;

    // Fake sound engine stuff for prototyping
    typedef struct SoundState {
        bool IsPlaying;
        int32_t SamplePosition;
    } SoundState;

    typedef struct GameState {
        Input::Controller Controllers[ControllerCount];
        Resources::ResourceCache Resources;
        AnimationState TestAnimation;
        SoundState Sound;
    } GameState;

    void InitializeGameState(GameState *gameState);

    void UpdateAndRender(GameState *gameState, VideoBuffer *videoBuffer);

    void GetSoundSamples(GameState *gameState, SoundBuffer *soundBuffer);
}