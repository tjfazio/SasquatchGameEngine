#pragma once

#include <stdint.h>

#include "common.h"
#include "input.h"

#define pixel_t uint32_t
#define sample_t int16_t

const int32_t SGE_ControllerCount = 1;

enum SGE_ControllerId
{
    SGE_Keyboard = 0,
    SGE_GamePad1 = 1,
    SGE_GamePad2 = 2,
    SGE_GamePad3 = 3,
    SGE_GamePad4 = 4
};

typedef struct SGE_GameClock {    
    real32_t TimeDeltaSeconds;
    real32_t GameTimeSeconds;
    real32_t TargetFrameRate;
    real32_t TargetFrameLengthSeconds;
    real32_t FrameVariationSeconds;
} SGE_GameClock;

// RGB pixels layed out row-wise with given stride
typedef struct SGE_VideoBuffer {
    int32_t Height;
    int32_t Width;
    int32_t Stride;
    pixel_t *Memory;
} SGE_VideoBuffer;

typedef struct SGE_SoundBuffer {
    int32_t NumChannels;
    int32_t SamplesPerSecond;
    int32_t SampleCount;
    int32_t BufferSize;
    sample_t *Memory;
} SGE_SoundBuffer; 

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
typedef struct tagSoundState {
    bool IsPlaying;
    int32_t SamplePosition;
} SoundState;

typedef struct tagGameState {
    SGE_Controller Controllers[SGE_ControllerCount];
    AnimationState TestAnimation;
    SoundState Sound;
} SGE_GameState;

void SGE_Init(SGE_GameState *gameState);

void SGE_UpdateAndRender(SGE_GameState *gameState,
                         SGE_VideoBuffer *videoBuffer);

void SGE_GetSoundSamples(SGE_GameState *gameState,
                         SGE_SoundBuffer *soundBuffer);