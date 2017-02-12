#pragma once

#include <stdint.h>

typedef struct tagKeyboardMap {
    uint8_t Primary[255];
    uint8_t Secondary[255];
} KeyboardMap;

class SGE_Keyboard
{
    private:
        uint8_t KeyState[255];
        KeyboardMap Map;
    public:
        void MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode);
        void SetState(uint8_t keyCode, bool isDown);
        void ClearState(void);
        bool IsSet(uint8_t actionCode);
};