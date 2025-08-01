// assist_optimized.cpp - FINAL REFACTORED VERSION v5.0.0
// --------------------------------------------------------------------------------------
// description: Optimized aim assist system implementation, fully compatible with the
//              new detailed data structures from globals.h.
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 5.0.0 - Adapted to new globals.h v5.1.0 data structures.
// license: GNU General Public License v3.0
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "assist_optimized.h"
#include "state_manager.h"
#include "event_system.h"
#include "simd_math.h"
#include "pid_optimized.h"
#include "globals.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <random>
#include <cctype>

// Global instance of the Aim Assist System
std::unique_ptr<OptimizedAimAssistSystem> g_aimAssistSystem;

namespace {
    thread_local std::mt19937 gen(std::random_device{}());
    float randomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }
}

// =============================================================================
// OptimizedAimAssistSystem CLASS IMPLEMENTATION
// =============================================================================

OptimizedAimAssistSystem::OptimizedAimAssistSystem(StateManager& stateManager, EventSystem& eventSystem)
    : m_stateManager(stateManager), m_eventSystem(eventSystem) {}

OptimizedAimAssistSystem::~OptimizedAimAssistSystem() {
    shutdown();
}

bool OptimizedAimAssistSystem::initialize() {
    if (m_isInitialized) return true;
    logMessage("AimAssistSystem: Initializing...");

    m_config = m_stateManager.getAppSettings().aim_config;
    
    pid_controller_x = createOptimizedPID(1.0, 0.1, 0.05);
    pid_controller_y = createOptimizedPID(1.0, 0.1, 0.05);

    if (!pid_controller_x || !pid_controller_y) {
        logError("AimAssistSystem: CRITICAL: Failed to create PID controllers.");
        return false;
    }

    subscribeToEvents();
    m_isInitialized = true;
    m_isRunning.store(true);
    logMessage("AimAssistSystem: Initialized successfully.");
    return true;
}

void OptimizedAimAssistSystem::shutdown() {
    if (!m_isInitialized) return;
    logMessage("AimAssistSystem: Shutting down...");
    m_isRunning.store(false);
    m_isInitialized = false;
}

void OptimizedAimAssistSystem::updateOnFrame() {
    if (!m_isRunning.load() || !m_stateManager.isAimAssistEnabled() || m_isProcessing.load()) {
        return;
    }

    m_isProcessing.store(true);
    if (!isSafeToOperate()) {
        m_isProcessing.store(false);
        return;
    }

    detected_targets = detectTargets();
    current_target = selectBestTarget(detected_targets);

    if (current_target.isValid()) {
        AimCalculation aim_calc = calculateAim();
        if (aim_calc.should_aim) {
            // UPDATED: Use the new MovementCommand constructor
            MovementCommand move_cmd(aim_calc.smoothed_x, aim_calc.smoothed_y, m_config.smoothing_factor);
            m_eventSystem.publishEventWithData(EventType::ExecuteMouseMovement, move_cmd);
        }
        if (aim_calc.should_shoot) {
            m_eventSystem.publishEvent(EventType::ExecutePrimaryFire);
        }
    }
    
    m_isProcessing.store(false);
}

void OptimizedAimAssistSystem::subscribeToEvents() {
    m_eventSystem.subscribe(EventType::FrameUpdate, "AimAssist_FrameUpdateHandler",
        [this](const BaseEvent&) { this->updateOnFrame(); });
    // ... other subscriptions ...
}

std::vector<TargetInfo> OptimizedAimAssistSystem::detectTargets() {
    // Simulates detection using the new TargetInfo structure
    std::vector<TargetInfo> targets;
    POINT screen_center = m_stateManager.getScreenCenter();
    
    int num_targets = randomFloat(0, 3);
    for (int i = 0; i < num_targets; ++i) {
        TargetInfo target;
        target.x = screen_center.x + randomFloat(-m_config.aim_fov, m_config.aim_fov);
        target.y = screen_center.y + randomFloat(-m_config.aim_fov, m_config.aim_fov);
        target.confidence = randomFloat(0.7f, 1.0f);
        target.distance = SIMDMath::fastDistance(target.x, target.y, screen_center.x, screen_center.y);
        target.velocity_x = randomFloat(-5, 5);
        target.is_moving = std::abs(target.velocity_x) > 1.0;
        target.is_head = (randomFloat(0, 1) > 0.5); // 50% chance of being a head
        target.is_enemy = true;
        target.priority = TargetPriority::MEDIUM;
        
        if (target.isValid()) {
            targets.push_back(target);
        }
    }
    return targets;
}

TargetInfo OptimizedAimAssistSystem::selectBestTarget(const std::vector<TargetInfo>& targets) {
    if (targets.empty()) return {};

    TargetInfo best_target;
    double max_priority_score = -1.0;

    for (const auto& target : targets) {
        double priority = calculateTargetPriority(target);
        if (priority > max_priority_score) {
            max_priority_score = priority;
            best_target = target;
        }
    }
    return best_target;
}

double OptimizedAimAssistSystem::calculateTargetPriority(const TargetInfo& target) {
    // UPDATED: Logic now uses the richer TargetInfo data
    const auto& priority_cfg = m_stateManager.getAppSettings().priority_config;
    POINT screen_center = m_stateManager.getScreenCenter();
    double score = 0.0;

    // Headshot preference
    if (priority_cfg.prefer_head && target.is_head) {
        score += priority_cfg.head_priority_weight;
    }

    // Proximity to center screen preference
    double screen_dist = SIMDMath::fastDistance(target.x, target.y, screen_center.x, screen_center.y);
    score += (1.0 - (screen_dist / screen_center.x)) * priority_cfg.screen_center_weight;

    // Closest target preference
    if (priority_cfg.prefer_closest) {
        score += (1.0 - (target.distance / m_config.max_distance)) * priority_cfg.distance_weight;
    }
    
    return score * target.confidence;
}

bool OptimizedAimAssistSystem::validateTarget(const TargetInfo& target) {
    // This now calls the member function of the struct itself
    return target.isValid();
}

AimCalculation OptimizedAimAssistSystem::calculateAim() {
    AimCalculation calc;
    if (!current_target.isValid()) return calc;
    
    POINT cursor_pos = m_stateManager.getCursorPosition();
    TargetInfo final_target = current_target;

    if (m_config.prediction_enabled && final_target.is_moving) {
        final_target.predict(m_config.prediction_time);
    }

    // UPDATED: Use the new predicted members if prediction happened
    double target_x = final_target.is_moving ? final_target.predicted_x : final_target.x;
    double target_y = final_target.is_moving ? final_target.predicted_y : final_target.y;
    
    double error_x = target_x - cursor_pos.x;
    double error_y = target_y - cursor_pos.y;
    
    if (std::abs(error_x) < 1.0 && std::abs(error_y) < 1.0) return calc;
    
    double dt = 1.0 / 144.0;
    double adj_x = pid_controller_x->calculate(error_x, dt);
    double adj_y = pid_controller_y->calculate(error_y, dt);

    auto smoothed = SIMDMath::fastSmooth(0, 0, adj_x, adj_y, m_config.smoothing_factor);
    calc.smoothed_x = smoothed.first;
    calc.smoothed_y = smoothed.second;
    
    double dist_to_target = SIMDMath::fastDistance(cursor_pos.x, cursor_pos.y, target_x, target_y);
    if (dist_to_target <= m_config.aim_fov) calc.should_aim = true;
    if (m_config.trigger_bot && dist_to_target <= m_config.trigger_fov) calc.should_shoot = true;
    
    return calc;
}

bool OptimizedAimAssistSystem::isSafeToOperate() {
    return true;
}

// =============================================================================
// GLOBAL C-STYLE API IMPLEMENTATION
// =============================================================================
bool initializeAimAssistSystem() {
    if (g_aimAssistSystem) return true;
    if (!getStateManager() || !getEventSystem()) {
        logError("AimAssistSystem Error: StateManager or EventSystem must be initialized first.");
        return false;
    }
    g_aimAssistSystem = std::make_unique<OptimizedAimAssistSystem>(*getStateManager(), *getEventSystem());
    return g_aimAssistSystem->initialize();
}

void shutdownAimAssistSystem() {
    if (g_aimAssistSystem) {
        g_aimAssistSystem->shutdown();
        g_aimAssistSystem.reset();
    }
}

void runOptimizedAimAssistLoop() {
    while (STATE_MGR() && STATE_MGR()->isRunning()) {
        if (isAimAssistSystemActive()) {
            if(getEventSystem()) getEventSystem()->publishEvent(EventType::FrameUpdate);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool isAimAssistSystemActive() {
    return g_aimAssistSystem && g_aimAssistSystem->isRunning();
}