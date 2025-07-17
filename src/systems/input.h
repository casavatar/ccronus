// description: This header file declares functions for handling enhanced key presses and raw input from mouse and keyboard devices.
// It includes necessary libraries and provides a structure for handling input events in the Tactical Aim Assist project.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist
//

#pragma once
#include <windows.h>

void handleEnhancedKeyPress(int vk);
void HandleRawMouseInput(const RAWMOUSE& rawMouse);
void HandleRawKeyboardInput(const RAWKEYBOARD& rawKeyboard);