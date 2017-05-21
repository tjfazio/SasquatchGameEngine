#pragma once

#include <stdint.h>

const int32_t SGE_ButtonCount = 255;

enum ActionCodes : uint8_t {
    SGE_Up = 0,
    SGE_Down = 1,
    SGE_Left = 3,
    SGE_Right = 4,
    SGE_Action1 = 5
};

typedef struct SGE_ButtonState {
    uint8_t StartedDown;
    uint8_t EndedDown;
    uint8_t TransitionCount;
} SGE_ButtonState;

typedef struct SGE_ButtonMap {
    uint8_t Primary[SGE_ButtonCount];
    uint8_t Secondary[SGE_ButtonCount];
} SGE_ButtonMap;

class SGE_Controller
{
    private:
        SGE_ButtonState ButtonState1[SGE_ButtonCount];
        SGE_ButtonState ButtonState2[SGE_ButtonCount];
        SGE_ButtonState *ActiveButtonState;
        SGE_ButtonState *InactiveButtonState;
        SGE_ButtonMap Map;
    public:
        void Initialize();
        void MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode);
        void UpdateState(uint8_t keyCode, bool isDown);
        void FinalizeState();
        void GetState(uint8_t actionCode, SGE_ButtonState *buttonStateOut);
};