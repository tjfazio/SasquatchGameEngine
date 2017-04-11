#pragma once

#include <stdint.h>

#include "input.h"

#define pixel_t uint32_t

typedef struct tagVideoBuffer {
    int32_t Height;
    int32_t Width;
    void *Memory;
} SGE_VideoBuffer;

// Fake gameplay stuff for prototyping
typedef struct tagAnimationState {
    int32_t XStart;
    int32_t XVelocity;
    int32_t YStart;
    int32_t YVelocity;
    int32_t BlueWidth;
    int32_t GreenWidth;
} AnimationState;

typedef struct gameStateTag {
    SGE_Keyboard Keyboard;
    AnimationState TestAnimation;
} SGE_GameState;

void SGE_Init(SGE_GameState *gameState);

void SGE_UpdateAndRender(SGE_GameState *gameState,
                         SGE_VideoBuffer *videoBuffer);