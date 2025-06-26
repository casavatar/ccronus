// description: Header file for mouse and keyboard simulation functions, including fire modes and tactical movements.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <windows.h>

// Movimiento del Ratón
void predictiveMouseMove(int dx, int dy, int movementType = 0);

// Modos de Disparo
void controlledAutomaticFire();
void enhancedIntelligentRapidFire();
void tacticalFire(); // New Tactical fire mode function

// Movimientos Tácticos
void executeSmartDiagonalSprint(bool leftDirection);
void executePredictiveSlide();
void executeAntiDetectionDiveBack();
void executeSmartCornerBounce(bool leftCorner);
void executePredictiveCutback();
void executeDropShotSupineSlide();
void executeSlideCancelDirectional();
void executeDiveDirectionalIntelligent();
void executeOmnidirectionalSlide();
void executeMovementTest();