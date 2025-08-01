// globals.h - CORRECTED VERSION v5.1.0
// description: Global type definitions, constants, and stateless utility functions.
// version: 5.1.0 - Added missing constructors and consolidated TargetInfo members.
// date: 2025-07-21
// project: Tactical Aim Assist

#pragma once
#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <cmath>

// Windows API includes
#ifdef _WIN32
#include <windows.h>
#endif

// =============================================================================
// GLOBAL APPLICATION STATE (EXTERN DECLARATION)
// =============================================================================

extern std::atomic<bool> g_application_running;
extern std::atomic<bool> g_debugMode;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

struct WeaponProfile;
struct TargetInfo;
struct MovementCommand;
struct ModifierInfo;
struct Keybindings; // Forward declare the Keybindings struct

// =============================================================================
// ENUMERATIONS (TYPE DEFINITIONS)
// =============================================================================

enum class PlayerMovementState {
    Stationary, Walking, Running, Crouching, Jumping, Falling, Sprinting, Strafing, Sliding
};

enum class SystemState {
    INACTIVE, INITIALIZING, ACTIVE, PAUSED, SHUTTING_DOWN, ERROR_STATE
};

enum class TargetPriority {
    NONE, LOW, MEDIUM, HIGH, CRITICAL
};

enum class WeaponType {
    UNKNOWN, ASSAULT_RIFLE, SNIPER_RIFLE, SMG, SHOTGUN, PISTOL, LMG
};

enum class FireMode {
    Single,                             // Single shot
    Controlled,                         // Controlled burst
    Automatic,                          // Full automatic
    Rapid,                              // Rapid fire
    Tactical,                           // Tactical mode
    Custom                              // Custom mode
};

// =============================================================================
// PID CONTROLLER PARAMETERS STRUCTURE
// =============================================================================

struct PIDParameters {
    double kp = 0.7;                    // Proportional gain
    double ki = 0.3;                    // Integral gain
    double kd = 0.2;                    // Derivative gain
    double max_output = 50.0;           // Maximum output limit
    double integral_limit = 100.0;      // Integral windup limit
    double derivative_limit = 25.0;     // Derivative kick limit
    
    // Default constructor
    PIDParameters() = default;
    
    // Parameterized constructor
    PIDParameters(double p, double i, double d, double max_out = 50.0, 
                 double int_limit = 100.0, double deriv_limit = 25.0)
        : kp(p), ki(i), kd(d), max_output(max_out), 
          integral_limit(int_limit), derivative_limit(deriv_limit) {}
    
    // Validation
    bool isValid() const {
        return kp >= 0.0 && ki >= 0.0 && kd >= 0.0 && 
               max_output > 0.0 && integral_limit > 0.0 && derivative_limit > 0.0;
    }
    
    // Reset to defaults
    void reset() {
        kp = 0.7;
        ki = 0.3;
        kd = 0.2;
        max_output = 50.0;
        integral_limit = 100.0;
        derivative_limit = 25.0;
    }
    
    // Scale all parameters by a factor
    void scale(double factor) {
        kp *= factor;
        ki *= factor;
        kd *= factor;
        max_output *= factor;
        integral_limit *= factor;
        derivative_limit *= factor;
    }
};

// =============================================================================
// RECOIL PATTERN POINT STRUCTURE
// =============================================================================

struct RecoilPoint {
    double x = 0.0;                     // Horizontal offset
    double y = 0.0;                     // Vertical offset
    
    RecoilPoint() = default;
    RecoilPoint(double x_offset, double y_offset) : x(x_offset), y(y_offset) {}
    
    // Vector operations
    RecoilPoint operator+(const RecoilPoint& other) const {
        return RecoilPoint(x + other.x, y + other.y);
    }
    
    RecoilPoint operator-(const RecoilPoint& other) const {
        return RecoilPoint(x - other.x, y - other.y);
    }
    
    RecoilPoint operator*(double scalar) const {
        return RecoilPoint(x * scalar, y * scalar);
    }
    
    // Magnitude
    double magnitude() const {
        return std::sqrt(x * x + y * y);
    }
    
    // Normalize
    RecoilPoint normalize() const {
        double mag = magnitude();
        if (mag > 0.0) {
            return RecoilPoint(x / mag, y / mag);
        }
        return RecoilPoint(0.0, 0.0);
    }
};

// =============================================================================
// ADVANCED SETTINGS STRUCTURE
// =============================================================================

struct AdvancedSettings {
    bool enable_adaptive_aim = true;
    bool enable_dynamic_fov = true;
    bool enable_velocity_prediction = true;
    bool enable_acceleration_prediction = false;
    bool enable_target_switching = true;
    bool enable_recoil_prediction = true;
    
    double adaptive_sensitivity_factor = 1.0;
    double dynamic_fov_multiplier = 1.2;
    double velocity_prediction_weight = 0.7;
    double acceleration_prediction_weight = 0.3;
    double target_switching_threshold = 0.5;
    double recoil_prediction_weight = 0.8;
    
    // Advanced timing
    double prediction_horizon_ms = 200.0;
    double smoothing_decay_rate = 0.95;
    double confidence_threshold = 0.6;
    double accuracy_threshold = 0.8;
    
    AdvancedSettings() = default;
    
    void resetToDefaults() {
        enable_adaptive_aim = true;
        enable_dynamic_fov = true;
        enable_velocity_prediction = true;
        enable_acceleration_prediction = false;
        enable_target_switching = true;
        enable_recoil_prediction = true;
        
        adaptive_sensitivity_factor = 1.0;
        dynamic_fov_multiplier = 1.2;
        velocity_prediction_weight = 0.7;
        acceleration_prediction_weight = 0.3;
        target_switching_threshold = 0.5;
        recoil_prediction_weight = 0.8;
        
        prediction_horizon_ms = 200.0;
        smoothing_decay_rate = 0.95;
        confidence_threshold = 0.6;
        accuracy_threshold = 0.8;
    }
};

// =============================================================================
// KEYBINDING CONSTANTS (COMPILE-TIME CONSTANTS)
// =============================================================================

// Define modifier constants if not already defined by Windows API
#ifndef MOD_ALT
#define MOD_ALT         0x0001
#define MOD_CONTROL     0x0002
#define MOD_SHIFT       0x0004
#define MOD_WIN         0x0008
#endif
#ifndef MOD_NONE
#define MOD_NONE        0x0000
#endif

// Modifier combinations
constexpr int MOD_CTRL_ALT = (MOD_CONTROL | MOD_ALT);
constexpr int MOD_CTRL_SHIFT = (MOD_CONTROL | MOD_SHIFT);
constexpr int MOD_ALT_SHIFT = (MOD_ALT | MOD_SHIFT);
constexpr int MOD_CTRL_ALT_SHIFT = (MOD_CONTROL | MOD_ALT | MOD_SHIFT);

// =============================================================================
// CORE DATA STRUCTURES (TYPE DEFINITIONS)
// =============================================================================

struct Keybindings {
    unsigned int exit_vk = VK_F12; int exit_mod = MOD_NONE;
    unsigned int smart_sprint_left_vk = VK_LSHIFT; int smart_sprint_left_mod = MOD_NONE;
    unsigned int smart_sprint_right_vk = VK_RSHIFT; int smart_sprint_right_mod = MOD_NONE;
    unsigned int aim_assist_toggle_vk = VK_F1; int aim_assist_toggle_mod = MOD_NONE;
    unsigned int trigger_bot_toggle_vk = VK_F2; int trigger_bot_toggle_mod = MOD_NONE;
    unsigned int profile_next_vk = VK_F5; int profile_next_mod = MOD_NONE;
    unsigned int profile_prev_vk = VK_F6; int profile_prev_mod = MOD_NONE;
    unsigned int stats_reset_vk = VK_F3; int stats_reset_mod = MOD_NONE;
    unsigned int status_print_vk = VK_F4; int status_print_mod = MOD_NONE;
    unsigned int debug_toggle_vk = VK_F7; int debug_toggle_mod = MOD_CONTROL;
    unsigned int test_mode_vk = VK_F8; int test_mode_mod = MOD_CONTROL;
    unsigned int emergency_stop_vk = VK_ESCAPE; int emergency_stop_mod = MOD_CTRL_SHIFT;
    unsigned int sens_increase_vk = VK_OEM_PLUS; int sens_increase_mod = MOD_CONTROL;
    unsigned int sens_decrease_vk = VK_OEM_MINUS; int sens_decrease_mod = MOD_CONTROL;
    unsigned int fov_increase_vk = VK_UP; int fov_increase_mod = MOD_CONTROL;
    unsigned int fov_decrease_vk = VK_DOWN; int fov_decrease_mod = MOD_CONTROL;
    unsigned int smooth_increase_vk = VK_RIGHT; int smooth_increase_mod = MOD_CONTROL;
    unsigned int smooth_decrease_vk = VK_LEFT; int smooth_decrease_mod = MOD_CONTROL;
    unsigned int silent_aim_toggle_vk = VK_F9; int silent_aim_toggle_mod = MOD_CTRL_ALT;
    unsigned int prediction_toggle_vk = VK_F10; int prediction_toggle_mod = MOD_CTRL_ALT;
    unsigned int recoil_toggle_vk = VK_F11; int recoil_toggle_mod = MOD_CTRL_ALT;
    unsigned int super_emergency_vk = VK_F12; int super_emergency_mod = MOD_CTRL_ALT_SHIFT;
};

struct WeaponProfile {
    std::string name = "Default";
    WeaponType type = WeaponType::UNKNOWN;
    
    // Basic settings
    double sensitivity = 1.0;
    double smoothing = 0.75;
    double prediction = 0.5;
    double recoil_compensation = 0.8;
    double aim_speed = 1.0;
    double trigger_delay = 50.0; // milliseconds
    
    // Targeting settings
    double headshot_multiplier = 1.5;
    double body_multiplier = 1.0;
    double limb_multiplier = 0.8;
    double max_range = 1000.0;
    double optimal_range = 300.0;
    double min_range = 50.0;
    
    // FOV settings
    double aim_fov = 100.0;
    double trigger_fov = 50.0;
    double scan_fov = 200.0;
    
    // Advanced settings
    bool enable_prediction = true;
    bool enable_recoil_comp = true;
    bool enable_auto_aim = false;
    bool enable_trigger_bot = false;
    bool enable_silent_aim = false;
    bool enable_adaptive_sensitivity = true;
    
    // Timing settings
    double reaction_time = 100.0; // ms
    double acquisition_time = 50.0; // ms
    double lock_time = 200.0; // ms
    
    // Usage tracking - ADDED FOR PROFILES.H COMPATIBILITY
    uint64_t usage_count = 0;
    std::chrono::steady_clock::time_point last_used = std::chrono::steady_clock::now();
    double effectiveness_rating = 0.0;
    double accuracy_rating = 0.0;  // ADDED MISSING MEMBER
    
    // ADDED MISSING MEMBERS FOR PROFILES.CPP COMPATIBILITY
    FireMode fire_mode = FireMode::Single;
    double fire_delay_base = 50.0;
    double fire_delay_variance = 5.0;
    double smoothing_factor = 0.75;
    double prediction_aggressiveness = 0.5;
    
    // PID states for different movement contexts
    std::unordered_map<std::string, PIDParameters> pid_states;
    
    // Recoil pattern
    std::vector<RecoilPoint> recoil_pattern;
    
    // Advanced settings structure
    AdvancedSettings advanced;
    
    // Constructors
    WeaponProfile() = default;
    WeaponProfile(const std::string& n, WeaponType t, double sens, double smooth)
        : name(n), type(t), sensitivity(sens), smoothing(smooth) {}
        
    // Utility methods
    bool isValid() const;
    std::string typeToString() const;
    void resetToDefaults();
    void incrementUsage();
    void updateEffectiveness(double rating);
    
    // ADDED MISSING METHODS FOR PROFILES.CPP COMPATIBILITY
    void initializeDefaultPIDStates();
    PIDParameters getPIDParameters(const std::string& movement_state) const;
    void setPIDParameters(const std::string& movement_state, const PIDParameters& params);
    bool hasPIDState(const std::string& movement_state) const;
    std::vector<std::string> getAvailablePIDStates() const;
    
    void addRecoilPoint(double x, double y);
    void addRecoilPoint(const RecoilPoint& point);
    void clearRecoilPattern();
    RecoilPoint getRecoilOffset(int shot_number) const;
    size_t getRecoilPatternSize() const;
    void scaleRecoilPattern(double factor);
    void normalizeRecoilPattern();
    
    void updateAccuracy(double accuracy);
    double getUsageScore() const;
    std::chrono::duration<double> getTimeSinceLastUse() const;
    std::string fireModeString() const;
    void resetStatistics();
    std::string toString() const;
    
    bool operator==(const WeaponProfile& other) const;
    bool operator!=(const WeaponProfile& other) const;
};

struct TargetInfo {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    double confidence = 0.0;
    double distance = 0.0;
    TargetPriority priority = TargetPriority::NONE;
    
    // Prediction data
    double velocity_x = 0.0;
    double velocity_y = 0.0;
    double predicted_x = 0.0;
    double predicted_y = 0.0;
    
    // Timing data
    std::chrono::steady_clock::time_point first_detected;
    std::chrono::steady_clock::time_point last_updated;
    uint64_t frame_count = 0;
    
    // Target classification
    bool is_head = false;
    bool is_body = false;
    bool is_moving = false;
    bool is_enemy = true;
    
    TargetInfo() : first_detected(std::chrono::steady_clock::now()),
                   last_updated(std::chrono::steady_clock::now()) {}
                   
    bool isValid() const;
    double getAge() const;
    void updatePosition(int new_x, int new_y);
    void calculateVelocity();
    void predict(double time_ahead_ms);
};

struct MovementCommand {
    double delta_x = 0.0;
    double delta_y = 0.0;
    double smoothing_factor = 0.75;
    double confidence = 1.0;
    bool should_execute = false;
    
    // Timing information
    std::chrono::steady_clock::time_point timestamp;
    uint64_t sequence_id = 0;
    double execution_time_ms = 0.0;
    
    // Movement type
    PlayerMovementState movement_context = PlayerMovementState::Stationary;
    bool is_prediction = false;
    bool is_correction = false;
    
    MovementCommand() : timestamp(std::chrono::steady_clock::now()) {}
    MovementCommand(double dx, double dy, double smooth = 0.75, double conf = 1.0)
        : delta_x(dx), delta_y(dy), smoothing_factor(smooth), confidence(conf),
          should_execute(true), timestamp(std::chrono::steady_clock::now()) {}
          
    bool isValid() const;
    double getAge() const;
    void applySmoothing(double factor);
};

// =============================================================================
// GLOBAL CONSTANTS
// =============================================================================

namespace GlobalConstants {
    constexpr double MAX_SENSITIVITY = 10.0;
    constexpr double MIN_SENSITIVITY = 0.1;
    const std::string CONFIG_DIR = "./config/";
    const std::string MAIN_CONFIG_FILE = CONFIG_DIR + "main_config.json";
    const std::string LOG_FILE = "./logs/system.log";
}

// =============================================================================
// STATELESS UTILITY FUNCTION DECLARATIONS
// =============================================================================

// Logging
void logMessage(const std::string& message);
void logError(const std::string& message);
void logWarning(const std::string& message);
void logDebug(const std::string& message);

// Time
uint64_t getCurrentTimeMs();
std::string getCurrentTimeString();

// Input & Keybinding Helpers
int getCurrentModifierState(); // Checks keyboard state for CTRL/ALT/SHIFT
std::string getVirtualKeyName(unsigned int vk);
std::string getKeybindingString(unsigned int vk, int mod);

// Enum to String Converters
std::string playerMovementStateToString(PlayerMovementState state);

#endif // GLOBALS_H