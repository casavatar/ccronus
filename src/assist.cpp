// descption: This file implements the assistive aiming system for headshot prioritization in a game.
// It includes a class for hitbox detection and functions for calculating magnetism and sticky factors based on distance and confidence.
// The main function `enhancedHeadshotAimAssist` integrates these components to provide an enhanced aiming experience.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist
//

#include "assist.h"
#include "globals.h"
#include "systems.h"
#include "movements.h" // Para predictiveMouseMove
#include <cmath>
#include <algorithm> // Para std::min y std::max

// --- Implementación de Clases de Asistencia ---

HeadshotPrioritySystem::HeadshotPrioritySystem() {
    closeRange = {45, 8, 30, 60};
    midRange = {35, 6, 25, 50};
    longRange = {25, 4, 20, 40};
}

HitboxInfo HeadshotPrioritySystem::detectHitbox(POINT targetCenter, int estimatedDistance) {
    HitboxInfo hitbox;
    HitboxConfig config = closeRange;
    if (estimatedDistance > 100) config = longRange;
    else if (estimatedDistance > 50) config = midRange;

    hitbox.headCenter.x = targetCenter.x;
    hitbox.headCenter.y = targetCenter.y - config.headHeight;
    hitbox.bodyCenter = targetCenter;
    hitbox.headConfidence = calculateHeadConfidence(estimatedDistance);
    hitbox.bodyConfidence = 0.9;
    hitbox.targetDistance = estimatedDistance;
    return hitbox;
}

double HeadshotPrioritySystem::calculateHeadConfidence(int distance) {
    if (distance < 30) return 0.85;
    if (distance < 60) return 0.70;
    if (distance < 100) return 0.55;
    return 0.40;
}

int HeadshotPrioritySystem::calculateHeadshotAdjustment(POINT current, HitboxInfo hitbox) {
    int verticalDiff = hitbox.headCenter.y - current.y;
    if (hitbox.targetDistance > 100) verticalDiff -= 5;
    return verticalDiff;
}


// --- Implementación de Funciones de Asistencia ---

double calculateMagnetism(int distance, double confidence) {
    double distanceFactor = 1.0;
    if (distance < 30) distanceFactor = 0.8;
    else if (distance < 60) distanceFactor = 0.6;
    else if (distance < 100) distanceFactor = 0.4;
    else distanceFactor = 0.25;
    
    return distanceFactor * (0.5 + confidence * 0.5);
}

double getStickyFactor(int distanceToTarget, double predictionConfidence, double userMouseVelocity) {
    double baseSticky = 0.8;
    double velocityThreshold = 10.0;
    double distanceThreshold = 30.0;
    double stickyMultiplier = 1.0;

    if (userMouseVelocity > velocityThreshold) {
        stickyMultiplier *= std::max(0.0, 1.0 - ((userMouseVelocity - velocityThreshold) / 20.0));
    }

    if (distanceToTarget < distanceThreshold && userMouseVelocity < velocityThreshold) {
        stickyMultiplier *= 1.2;
    }
    
    stickyMultiplier *= predictionConfidence;
    return baseSticky * stickyMultiplier;
}

void enhancedHeadshotAimAssist() {
    if (!g_assistEnabled.load() || !g_predictiveAim) return;

    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::AIMING);
    }
    
    g_predictiveAim->updateCursorHistory();
    
    POINT predicted = g_predictiveAim->getPredictedTarget();
    double confidence = g_predictiveAim->getPredictionConfidence();
    
    if (confidence < 0.15) return;
    
    POINT current;
    GetCursorPos(&current);
    
    int distance = static_cast<int>(std::sqrt(
        std::pow(static_cast<double>(predicted.x) - SCREEN_CENTER_X, 2) + 
        std::pow(static_cast<double>(predicted.y) - SCREEN_CENTER_Y, 2)
    ) / 10);
    
    static HeadshotPrioritySystem headshotSystem; // Instancia estática local
    HitboxInfo hitbox = headshotSystem.detectHitbox(predicted, distance);
    
    bool aimForHead = (hitbox.headConfidence > 0.5 && distance < 80);
    POINT aimTarget = aimForHead ? hitbox.headCenter : hitbox.bodyCenter;
    
    int error_x = aimTarget.x - current.x;
    int error_y = aimTarget.y - current.y;
    
    double magnetStrength = calculateMagnetism(distance, confidence);
    double humanAccuracy = g_antiDetection ? g_antiDetection->getHumanAccuracy() : 0.9;
    
    double userMouseVelocity = g_predictiveAim->getUserMouseVelocity();
    double stickyStrength = getStickyFactor(distance, confidence, userMouseVelocity);

    if (std::abs(error_x) > 3 || std::abs(error_y) > 3) {
        double correctionFactor = confidence * humanAccuracy * magnetStrength * stickyStrength;
        
        if (aimForHead && std::abs(error_y) < 30) {
            correctionFactor *= 1.5;
        }
        
        int snap_x = static_cast<int>(error_x * correctionFactor);
        int snap_y = static_cast<int>(error_y * correctionFactor);
        
        int maxLimit = static_cast<int>(25 * confidence * magnetStrength);
        snap_x = std::min(std::max(snap_x, -maxLimit), maxLimit);
        snap_y = std::min(std::max(snap_y, -maxLimit), maxLimit);
        
        int moveType = (distance < 50) ? 3 : 2;
        
        if (std::abs(snap_x) > 1 || std::abs(snap_y) > 1) {
            predictiveMouseMove(snap_x, snap_y, moveType);
            
            if (aimForHead && std::abs(error_x) < 20 && std::abs(error_y) < 20) {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                
                int microY = headshotSystem.calculateHeadshotAdjustment(current, hitbox);
                if (std::abs(microY) > 2 && std::abs(microY) < 15) {
                    predictiveMouseMove(0, static_cast<int>(microY * 0.7), 3);
                }
            }
            
            if (confidence > 0.7 && g_predictiveAim) {
                POINT velocity = g_predictiveAim->getTargetVelocity();
                if (std::abs(velocity.x) > 50 || std::abs(velocity.y) > 50) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    
                    int lead_x = static_cast<int>(velocity.x * 0.02 * distance / 100.0);
                    int lead_y = static_cast<int>(velocity.y * 0.02 * distance / 100.0);
                    
                    lead_x = std::min(std::max(lead_x, -8), 8);
                    lead_y = std::min(std::max(lead_y, -8), 8);
                    
                    if (std::abs(lead_x) > 1 || std::abs(lead_y) > 1) {
                        predictiveMouseMove(lead_x, lead_y, 3);
                    }
                }
            }
        }
    }
    
    static std::chrono::steady_clock::time_point lastSticky = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSticky).count() > 50) {
        if (std::abs(error_x) < 15 && std::abs(error_y) < 15 && confidence > 0.6) {
            int sticky_x = static_cast<int>(error_x * 0.15);
            int sticky_y = static_cast<int>(error_y * 0.15);
            
            if (std::abs(sticky_x) > 0 || std::abs(sticky_y) > 0) {
                predictiveMouseMove(sticky_x, sticky_y, 3);
            }
        }
        lastSticky = now;
    }
}