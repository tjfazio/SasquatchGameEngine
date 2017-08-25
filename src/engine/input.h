#pragma once

#include <stdint.h>

namespace Sasquatch { namespace Input
{
    const int32_t ButtonCount = 255;

    enum ActionCodes : uint8_t {
        Up = 0,
        Down = 1,
        Left = 3,
        Right = 4,
        Action1 = 5
    };

    typedef struct ButtonState {
        uint8_t StartedDown;
        uint8_t EndedDown;
        uint8_t TransitionCount;
    } ButtonState;

    typedef struct ButtonMap {
        uint8_t Primary[ButtonCount];
        uint8_t Secondary[ButtonCount];
    } ButtonMap;

    class Controller
    {
        private:
            ButtonState ButtonState1[ButtonCount];
            ButtonState ButtonState2[ButtonCount];
            ButtonState *ActiveButtonState;
            ButtonState *InactiveButtonState;
            ButtonMap Map;
        public:
            void Initialize();
            void MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode);
            void UpdateState(uint8_t keyCode, bool isDown);
            void FinalizeState();
            void GetState(uint8_t actionCode, ButtonState *buttonStateOut);
    };
}}