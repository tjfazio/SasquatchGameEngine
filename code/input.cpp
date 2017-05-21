#include <assert.h>

#include "input.h"

void SGE_Controller::Initialize()
{
    ActiveButtonState = ButtonState1;
    InactiveButtonState = ButtonState2;
}

void SGE_Controller::MapAction(uint8_t actionCode, uint8_t primaryKeyCode, uint8_t secondaryKeyCode)
{
    Map.Primary[actionCode] = primaryKeyCode;
    Map.Secondary[actionCode] = secondaryKeyCode;
}

inline void SGE_Controller::UpdateState(uint8_t keyCode, bool isDown)
{            
    assert(keyCode < SGE_ButtonCount);
    if ((bool)InactiveButtonState[keyCode].EndedDown != isDown)
    {
        InactiveButtonState[keyCode].TransitionCount++;
        InactiveButtonState[keyCode].EndedDown = isDown;
    }
}

void SGE_Controller::FinalizeState()
{
    // flip active and inactive state
    ActiveButtonState = (ActiveButtonState == ButtonState1) ? ButtonState2 : ButtonState1;
    InactiveButtonState = (InactiveButtonState == ButtonState1) ? ButtonState2 : ButtonState1;

    for (int i = 0; i < SGE_ButtonCount; i++)
    {
        InactiveButtonState[i].StartedDown = ActiveButtonState[i].EndedDown;
        InactiveButtonState[i].EndedDown = ActiveButtonState[i].EndedDown;
        InactiveButtonState[i].TransitionCount = 0;
    }
}

inline void SGE_Controller::GetState(uint8_t actionCode, SGE_ButtonState *buttonStateOut)
{
    SGE_ButtonState primaryButton = ActiveButtonState[Map.Primary[actionCode]];
    SGE_ButtonState secondaryButton = ActiveButtonState[Map.Secondary[actionCode]];

    assert((int32_t)primaryButton.TransitionCount + (int32_t)secondaryButton.TransitionCount < 255);

    buttonStateOut->StartedDown = (primaryButton.StartedDown || secondaryButton.StartedDown);
    buttonStateOut->EndedDown = (primaryButton.EndedDown || secondaryButton.EndedDown);
    buttonStateOut->TransitionCount = primaryButton.TransitionCount + secondaryButton.TransitionCount;
}