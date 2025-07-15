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

// --- REENGINEERED: Predictive Aim Assist Main Function ---
void enhancedHeadshotAimAssist() {
    if (!g_assistEnabled.load() || !g_pidX || !g_pidY || !g_predictiveAim || !g_smoothingSystem) return;

    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::AIMING);
    }
    
    g_predictiveAim->updateCursorHistory();
    
    // 1. Get Prediction Data
    POINT current_predicted_pos = g_predictiveAim->getPredictedTarget();
    double confidence = g_predictiveAim->getPredictionConfidence();
    
    if (confidence < 0.30) { // Increased confidence threshold for stability
        g_pidX->reset();
        g_pidY->reset();
        return;
    }
    
    // 2. Dynamic PID Tuning
    const auto& currentProfile = g_weaponProfiles[g_activeProfileIndex.load()];
    // ... (código de selección de PID dinámico sin cambios) ...

    // 3. Lead-Aiming Calculation
    POINT target_velocity = g_predictiveAim->getTargetVelocity();
    POINT target_acceleration = g_predictiveAim->getAcceleration(); // Assuming this function exists now
    
    // Define a lead time (can be made more complex, e.g., based on bullet speed)
    const double lead_time_ms = 80.0; 
    double lead_factor = (lead_time_ms / 1000.0) * currentProfile.prediction_aggressiveness;

    POINT lead_adjustment;
    lead_adjustment.x = static_cast<long>(target_velocity.x * lead_factor);
    lead_adjustment.y = static_cast<long>(target_velocity.y * lead_factor);

    POINT final_aim_target = { current_predicted_pos.x + lead_adjustment.x, current_predicted_pos.y + lead_adjustment.y };

    // 4. Error Calculation
    POINT current_pos;
    GetCursorPos(&current_pos);
    int error_x = final_aim_target.x - current_pos.x;
    int error_y = final_aim_target.y - current_pos.y;

    // 5. Decoupling and Dampening
    double user_mouse_velocity = g_predictiveAim->getUserMouseVelocity();
    double intent_decoupling_factor = 1.0 - std::min(1.0, user_mouse_velocity / 30.0);

    double acceleration_magnitude = std::hypot(static_cast<double>(target_acceleration.x), static_cast<double>(target_acceleration.y));
    double acceleration_dampening = 1.0 - std::min(1.0, acceleration_magnitude / 50.0) * 0.5; // Reduce assist by up to 50% for erratic targets

    // 6. PID Calculation with all factors
    double correction_x = g_pidX->calculate(error_x) * intent_decoupling_factor * acceleration_dampening;
    double correction_y = g_pidY->calculate(error_y) * intent_decoupling_factor * acceleration_dampening;

    // 7. Dynamic Smoothing
    double smoothing_factor = g_smoothingSystem->getSmoothingFactor(true, true, std::hypot(target_velocity.x, target_velocity.y));

    int final_dx = static_cast<int>(correction_x * smoothing_factor);
    int final_dy = static_cast<int>(correction_y * smoothing_factor);

    // 8. Execute Mouse Movement
    if (std::abs(final_dx) > 0 || std::abs(final_dy) > 0) {
        predictiveMouseMove(final_dx, final_dy, 3);
    }
}