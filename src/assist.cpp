// description: This file implements the advanced, PID-based aim assist system.
// It prioritizes headshots and uses dynamic smoothing for a natural feel.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.9.2
// date: 2025-06-26
// project: Tactical Aim Assist

#include "assist.h"
#include "globals.h"
#include "systems.h"
#include "movements.h"
#include "profiles.h"
#include <cmath>
#include <algorithm>

extern std::atomic<PlayerMovementState> g_playerMovementState;

// Headshot Priority System Implementation
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

// Magnetism and Sticky Factor Calculations
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


// --- REENGINEERED: Aim Assist Main Function ---
void enhancedHeadshotAimAssist() {
    if (!g_assistEnabled.load() || !g_pidX || !g_pidY || !g_predictiveAim || !g_smoothingSystem) return;

    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::AIMING);
    }
    
    g_predictiveAim->updateCursorHistory();
    
    // 1. Get Prediction Data
    POINT predicted_target = g_predictiveAim->getPredictedTarget();
    double confidence = g_predictiveAim->getPredictionConfidence();
    
    if (confidence < 0.25) {
        g_pidX->reset();
        g_pidY->reset();
        return;
    }
    
    // 2. Dynamic PID Tuning based on Movement State
    const auto& currentProfile = g_weaponProfiles[g_activeProfileIndex.load()];
    PlayerMovementState currentState = g_playerMovementState.load();
    PIDParams current_pid;

    switch (currentState) {
        case PlayerMovementState::Sprinting:
        case PlayerMovementState::Walking:
            current_pid = currentProfile.pid_states.at("moving");
            break;
        case PlayerMovementState::Strafing:
            current_pid = currentProfile.pid_states.at("strafing");
            break;
        case PlayerMovementState::Stationary:
        default:
            current_pid = currentProfile.pid_states.at("stationary");
            break;
    }
    g_pidX->updateParams(current_pid.kp, current_pid.ki, current_pid.kd);
    g_pidY->updateParams(current_pid.kp, current_pid.ki, current_pid.kd);
    
    // 3. Error Calculation
    POINT current_pos;
    GetCursorPos(&current_pos);
    int error_x = predicted_target.x - current_pos.x;
    int error_y = predicted_target.y - current_pos.y;

    // 4. Decoupling of Intent
    double user_mouse_velocity = g_predictiveAim->getUserMouseVelocity();
    double intent_decoupling_factor = 1.0 - std::min(1.0, user_mouse_velocity / 25.0);
    
    // 5. PID Calculation
    double correction_x = g_pidX->calculate(error_x) * intent_decoupling_factor;
    double correction_y = g_pidY->calculate(error_y) * intent_decoupling_factor;

    // 6. Dynamic Smoothing
    POINT target_velocity_vec = g_predictiveAim->getTargetVelocity();
    double target_velocity_scalar = std::hypot(static_cast<double>(target_velocity_vec.x), static_cast<double>(target_velocity_vec.y));
    double smoothing_factor = g_smoothingSystem->getSmoothingFactor(true, true, target_velocity_scalar);

    int final_dx = static_cast<int>(correction_x * smoothing_factor);
    int final_dy = static_cast<int>(correction_y * smoothing_factor);

    // 7. Micro-correction
    if (std::abs(error_x) < 5 && std::abs(error_y) < 5) {
        final_dx += (error_x > 0) ? 1 : ((error_x < 0) ? -1 : 0);
        final_dy += (error_y > 0) ? 1 : ((error_y < 0) ? -1 : 0);
    }
    
    // 8. Execute Mouse Movement
    if (std::abs(final_dx) > 0 || std::abs(final_dy) > 0) {
        predictiveMouseMove(final_dx, final_dy, 3);
    }
}