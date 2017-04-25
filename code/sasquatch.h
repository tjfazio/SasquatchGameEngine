#pragma once

#include <stdint.h>

#include "input.h"

#define pixel_t uint32_t
#define sample_t int16_t

// RGB pixels layed out row-wise with given stride
typedef struct tagSGEVideoBuffer {
    int32_t Height;
    int32_t Width;
    int32_t Stride;
    pixel_t *Memory;
} SGE_VideoBuffer;

typedef struct tagSGESoundBuffer {
    int32_t NumChannels;
    int32_t SamplesPerSecond;
    int32_t SampleCount;
    int32_t BufferSize;
    sample_t *Memory;
} SGE_SoundBuffer; 

// Fake gameplay stuff for prototyping
typedef struct tagAnimationState {
    int32_t XStart;
    int32_t XVelocity;
    int32_t YStart;
    int32_t YVelocity;
    int32_t BlueWidth;
    int32_t GreenWidth;
} AnimationState;

typedef struct tagGameState {
    SGE_Keyboard Keyboard;
    AnimationState TestAnimation;
} SGE_GameState;

void SGE_Init(SGE_GameState *gameState);

void SGE_UpdateAndRender(SGE_GameState *gameState,
                         SGE_VideoBuffer *videoBuffer);

void SGE_GetSoundSamples(SGE_SoundBuffer *soundBuffer);