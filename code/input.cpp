#include "input.h"

void SGE_Keyboard::MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode)
{
    Map.Primary[actionCode] = primaryKeyCode;
    Map.Secondary[actionCode] = secondaryKeyCode;
}

inline void SGE_Keyboard::SetState(uint8_t keyCode, bool isDown)
{            
    KeyState[keyCode] = isDown;
}

void SGE_Keyboard::ClearState(void)
{
    for (int i = 0; i < 255; i++)
    {
        KeyState[i] = false;
    }
}

inline bool SGE_Keyboard::IsSet(uint8_t actionCode)
{
    return KeyState[Map.Primary[actionCode]] || KeyState[Map.Secondary[actionCode]];
}