#include "sasquatch.h"

pixel_t GetFakePixel(SGE_GameState *gameState, int32_t x, int32_t y)
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

void Animate(SGE_GameState *gameState, SGE_VideoBuffer *videoBuffer)
{
    int32_t width = videoBuffer->Width;
    int32_t height = -1 * videoBuffer->Height;
    pixel_t *pixels = (pixel_t *)(videoBuffer->Memory);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            pixels[y * width + x] = GetFakePixel(gameState, x, y);
        }
    }

    // fake game logic
    if (gameState->Keyboard.IsSet(VIC_Up) && !gameState->Keyboard.IsSet(VIC_Down))
    {
        gameState->TestAnimation.YVelocity = -1;
    }
    else if (gameState->Keyboard.IsSet(VIC_Down) && !gameState->Keyboard.IsSet(VIC_Up))
    {
        gameState->TestAnimation.YVelocity = 1;
    }
    else
    {
        gameState->TestAnimation.YVelocity = 0;
    }

    if (gameState->Keyboard.IsSet(VIC_Right) && !gameState->Keyboard.IsSet(VIC_Left))
    {
        gameState->TestAnimation.XVelocity = 1;
    }
    else if (gameState->Keyboard.IsSet(VIC_Left) && !gameState->Keyboard.IsSet(VIC_Right))
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
    Animate(gameState, videoBuffer);
}