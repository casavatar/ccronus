// description: This header file defines the structures and functions for an enhanced aim assist system in a game, focusing on headshot detection and prioritization.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist

#pragma once

#include <windows.h>

// --- Declaraciones de Clases de Asistencia ---

// Contiene información sobre los hitboxes detectados
struct HitboxInfo {
    POINT headCenter;
    POINT bodyCenter;
    double headConfidence;
    double bodyConfidence;
    int targetDistance;
};

// Sistema para priorizar y calcular posiciones de headshot
class HeadshotPrioritySystem {
private:
    struct HitboxConfig { int headHeight, headSize, bodyWidth, bodyHeight; };
    HitboxConfig closeRange, midRange, longRange;

public:
    HeadshotPrioritySystem();
    HitboxInfo detectHitbox(POINT targetCenter, int estimatedDistance);
    double calculateHeadConfidence(int distance);
    int calculateHeadshotAdjustment(POINT current, HitboxInfo hitbox);
};


// --- Declaraciones de Funciones de Asistencia ---

// Función principal del Aim Assist
void enhancedHeadshotAimAssist();

// Funciones de ayuda para el Aim Assist
double calculateMagnetism(int distance, double confidence);
double getStickyFactor(int distanceToTarget, double predictionConfidence, double userMouseVelocity);