// assist_optimized.h - CORRECTED AND UPDATED VERSION v3.2.0
// description: Optimized aim assist system header. Fixed redefinition errors.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 3.2.0 - Removed redefinition of TargetInfo and other fixes.
// date: 2025-07-20
// project: Tactical Aim Assist

#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <chrono>
#include <cstdint>
#include <cmath>

// Forward declarations
struct WeaponProfile;
class OptimizedAimAssistSystem;
class StateManager;
class EventSystem;
class OptimizedPIDController;

// Include the one true definition of TargetInfo and other core types from globals.h
#include "globals.h"

// =============================================================================
// AIM ASSIST CONFIGURATION STRUCTURES
// =============================================================================

enum class AimAssistMode {
    Disabled,
    Precision,
    Aggressive,
    Stealth,
    Custom
};

struct AimAssistConfig {
    bool enabled = false;
    bool auto_aim = false;
    bool silent_aim = false;
    bool trigger_bot = false;
    bool recoil_compensation = true;
    bool prediction_enabled = true;
    
    // Sensitivity settings
    double global_sensitivity = 1.0;
    double aim_sensitivity = 1.0;
    double scope_sensitivity = 0.8;
    double movement_sensitivity = 0.6;
    
    // FOV and range settings
    double aim_fov = 100.0;
    double trigger_fov = 50.0;
    double max_distance = 1000.0;
    double min_distance = 10.0;
    
    // Smoothing and prediction
    double smoothing_factor = 0.75;
    double prediction_time = 50.0; // milliseconds
    double reaction_delay = 25.0;  // milliseconds
    
    // Detection avoidance
    bool humanization = true;
    double randomization_factor = 0.1;
    
    AimAssistConfig() = default;
};

struct TargetPriorityConfig {
    bool prefer_head = true;
    bool prefer_closest = true;
    double head_priority_weight = 1.5;
    double distance_weight = 0.8;
    double screen_center_weight = 0.6;
    
    TargetPriorityConfig() = default;
};

// =============================================================================
// AIM CALCULATION STRUCTURES
// =============================================================================

struct AimCalculation {
    double delta_x = 0.0;
    double delta_y = 0.0;
    double smoothed_x = 0.0;
    double smoothed_y = 0.0;
    bool should_aim = false;
    bool should_shoot = false;
};

// =============================================================================
// AIM ASSIST SYSTEM CLASS INTERFACE
// =============================================================================

class OptimizedAimAssistSystem {
public:
    OptimizedAimAssistSystem(StateManager& stateManager, EventSystem& eventSystem);
    ~OptimizedAimAssistSystem();

    // Deleted copy/move constructors to prevent copying.
    OptimizedAimAssistSystem(const OptimizedAimAssistSystem&) = delete;
    OptimizedAimAssistSystem& operator=(const OptimizedAimAssistSystem&) = delete;

    // System control
    bool initialize();
    void shutdown();
    
    // Main loop function, typically triggered by an event
    void updateOnFrame();
    
    bool isRunning() const { return m_isRunning.load(); }
    bool isProcessing() const { return m_isProcessing.load(); }
    const TargetInfo& getCurrentTarget() const { return current_target; }

private:
    // Event subscription
    void subscribeToEvents();

    // Core logic
    std::vector<TargetInfo> detectTargets();
    TargetInfo selectBestTarget(const std::vector<TargetInfo>& targets);
    double calculateTargetPriority(const TargetInfo& target);
    bool validateTarget(const TargetInfo& target);
    AimCalculation calculateAim();
    TargetInfo predictTargetMovement(const TargetInfo& target, double time_ms);

    // Helpers
    void updatePerformanceMetrics();
    bool isSafeToOperate();

    // --- Member Variables ---
    StateManager& m_stateManager;
    EventSystem& m_eventSystem;

    AimAssistConfig m_config;
    TargetPriorityConfig priority_config;
    
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_isInitialized{false};
    std::atomic<bool> m_isProcessing{false};
    
    std::vector<TargetInfo> detected_targets;
    TargetInfo current_target;

    std::unique_ptr<OptimizedPIDController> pid_controller_x;
    std::unique_ptr<OptimizedPIDController> pid_controller_y;
};

// =============================================================================
// GLOBAL FUNCTION DECLARATIONS
// =============================================================================

bool initializeAimAssistSystem();
void shutdownAimAssistSystem();
void runOptimizedAimAssistLoop(); // Kept for backward compatibility with main.cpp
bool isAimAssistSystemActive();

// Utility and debug functions
void logAimAssistEvent(const std::string& event);
std::string getAimAssistSystemStatus();