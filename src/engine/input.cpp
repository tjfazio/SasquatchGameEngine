#include <assert.h>

#include "input.h"

namespace Sasquatch { namespace Input
{
    void Controller::Initialize()
    {
        ActiveButtonState = ButtonState1;
        InactiveButtonState = ButtonState2;
    }

    void Controller::MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode)
    {
        Map.Primary[actionCode] = primaryKeyCode;
        Map.Secondary[actionCode] = secondaryKeyCode;
    }

    void Controller::UpdateState(uint8_t keyCode, bool isDown)
    {            
        assert(keyCode < ButtonCount);
        if ((bool)InactiveButtonState[keyCode].EndedDown != isDown)
        {
            InactiveButtonState[keyCode].TransitionCount++;
            InactiveButtonState[keyCode].EndedDown = isDown;
        }
    }

    void Controller::FinalizeState()
    {
        // flip active and inactive state
        ActiveButtonState = (ActiveButtonState == ButtonState1) ? ButtonState2 : ButtonState1;
        InactiveButtonState = (InactiveButtonState == ButtonState1) ? ButtonState2 : ButtonState1;

        for (int i = 0; i < ButtonCount; i++)
        {
            InactiveButtonState[i].StartedDown = ActiveButtonState[i].EndedDown;
            InactiveButtonState[i].EndedDown = ActiveButtonState[i].EndedDown;
            InactiveButtonState[i].TransitionCount = 0;
        }
    }

    void Controller::GetState(uint8_t actionCode, ButtonState *buttonStateOut)
    {
        ButtonState primaryButton = ActiveButtonState[Map.Primary[actionCode]];
        ButtonState secondaryButton = ActiveButtonState[Map.Secondary[actionCode]];

        assert((int32_t)primaryButton.TransitionCount + (int32_t)secondaryButton.TransitionCount < 255);

        buttonStateOut->StartedDown = (primaryButton.StartedDown || secondaryButton.StartedDown);
        buttonStateOut->EndedDown = (primaryButton.EndedDown || secondaryButton.EndedDown);
        buttonStateOut->TransitionCount = primaryButton.TransitionCount + secondaryButton.TransitionCount;
    }
} }