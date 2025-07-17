// description: Header file for mouse and keyboard simulation functions, including fire modes and tactical movements.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <windows.h>

// Mouse and Keyboard Simulation Functions
void predictiveMouseMove(int dx, int dy, int movementType = 0);

// Modos de Disparo
void controlledAutomaticFire(); // Controlled automatic fire mode
void enhancedIntelligentRapidFire(); // New enhanced rapid fire mode
void tacticalFire(); // New tactical fire mode

// Tactical Movements
void executeSmartDiagonalSprint(bool leftDirection);
void executePredictiveSlide();
void executeAntiDetectionDiveBack();
void executeSmartCornerBounce(bool leftCorner);
void executePredictiveCutback();
void executeDropShotSupineSlide();
void executeSlideCancelDirectional();
void executeDiveDirectionalIntelligent();
void executeOmnidirectionalSlide();
void executeContextualStrafeJump();
void executeMovementTest();