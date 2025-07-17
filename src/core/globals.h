// globals.h - COMPLETE UPDATED VERSION v3.1.6
// description: Global variables and system-wide declarations
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.6 - Fixed all missing variables and declarations
// date: 2025-07-17
// project: Tactical Aim Assist

#pragma once
#ifndef GLOBALS_H
#define GLOBALS_H

#include <atomic>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <mutex>

// Windows API includes
#ifdef _WIN32
#include <windows.h>
#endif

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

struct WeaponProfile;
struct TargetInfo;
struct AimAssistConfig;
struct MovementCommand;
struct ModifierInfo;

// =============================================================================
// ENUMERATIONS
// =============================================================================

enum class PlayerMovementState {
    Stationary,
    Walking,
    Running,
    Crouching,
    Jumping,
    Falling,
    Sprinting,
    Strafing,
    Sliding
};

enum class SystemState {
    INACTIVE,
    INITIALIZING,
    ACTIVE,
    PAUSED,
    SHUTTING_DOWN,
    ERROR_STATE
};

enum class TargetPriority {
    NONE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class WeaponType {
    UNKNOWN,
    ASSAULT_RIFLE,
    SNIPER_RIFLE,
    SMG,
    SHOTGUN,
    PISTOL,
    LMG
};

// =============================================================================
// KEYBINDING SYSTEM DECLARATIONS - COMPLETE MOD CONSTANTS
// =============================================================================

// Check if Windows API modifier constants are available
#ifdef _WIN32

// Verify Windows constants are properly defined
#ifndef MOD_ALT
#define MOD_ALT         0x0001
#define MOD_CONTROL     0x0002
#define MOD_SHIFT       0x0004
#define MOD_WIN         0x0008
#define MOD_NOREPEAT    0x4000
#endif

// Define our own MOD_NONE for consistency
#ifndef MOD_NONE
#define MOD_NONE        0x0000
#endif

#else
// Non-Windows platforms - define our own constants
#ifndef MOD_CONSTANTS_DEFINED
#define MOD_CONSTANTS_DEFINED
constexpr int MOD_NONE = 0x0000;
constexpr int MOD_ALT = 0x0001;
constexpr int MOD_CONTROL = 0x0002;
constexpr int MOD_SHIFT = 0x0004;
constexpr int MOD_WIN = 0x0008;
constexpr int MOD_NOREPEAT = 0x4000;
#endif
#endif // _WIN32

// =============================================================================
// EXTENDED MODIFIER COMBINATIONS - COMPLETE DECLARATIONS
// =============================================================================

// Basic two-key combinations
#ifndef MOD_COMBINATIONS_DEFINED
#define MOD_COMBINATIONS_DEFINED

// Two-key combinations
constexpr int MOD_CTRL_ALT = (MOD_CONTROL | MOD_ALT);
constexpr int MOD_CTRL_SHIFT = (MOD_CONTROL | MOD_SHIFT);
constexpr int MOD_ALT_SHIFT = (MOD_ALT | MOD_SHIFT);
constexpr int MOD_CTRL_WIN = (MOD_CONTROL | MOD_WIN);
constexpr int MOD_ALT_WIN = (MOD_ALT | MOD_WIN);
constexpr int MOD_SHIFT_WIN = (MOD_SHIFT | MOD_WIN);

// Three-key combinations
constexpr int MOD_CTRL_ALT_SHIFT = (MOD_CONTROL | MOD_ALT | MOD_SHIFT);
constexpr int MOD_CTRL_ALT_WIN = (MOD_CONTROL | MOD_ALT | MOD_WIN);
constexpr int MOD_CTRL_SHIFT_WIN = (MOD_CONTROL | MOD_SHIFT | MOD_WIN);
constexpr int MOD_ALT_SHIFT_WIN = (MOD_ALT | MOD_SHIFT | MOD_WIN);

// Four-key combination (ultimate)
constexpr int MOD_ALL_MODIFIERS = (MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN);

// =============================================================================
// SPECIAL PURPOSE MODIFIER COMBINATIONS
// =============================================================================

// Emergency combinations for critical system functions
constexpr int MOD_EMERGENCY = (MOD_CONTROL | MOD_SHIFT);              // Standard emergency
constexpr int MOD_EMERGENCY_ALT = (MOD_CONTROL | MOD_SHIFT | MOD_ALT); // Enhanced emergency
constexpr int MOD_EMERGENCY_WIN = (MOD_CONTROL | MOD_SHIFT | MOD_WIN); // System emergency
constexpr int MOD_EMERGENCY_ALL = (MOD_CONTROL | MOD_SHIFT | MOD_ALT | MOD_WIN); // Ultimate emergency

// Debug combinations for development
constexpr int MOD_DEBUG = (MOD_CONTROL);                             // Basic debug
constexpr int MOD_DEBUG_ENHANCED = (MOD_CONTROL | MOD_ALT);          // Enhanced debug
constexpr int MOD_DEBUG_VERBOSE = (MOD_CONTROL | MOD_ALT | MOD_SHIFT); // Verbose debug

// Configuration combinations
constexpr int MOD_CONFIG = (MOD_ALT);                                // Basic config
constexpr int MOD_CONFIG_ADVANCED = (MOD_ALT | MOD_SHIFT);          // Advanced config
constexpr int MOD_CONFIG_EXPERT = (MOD_CTRL_ALT_SHIFT);             // Expert config

// Profile management combinations
constexpr int MOD_PROFILE = (MOD_SHIFT);                            // Basic profile
constexpr int MOD_PROFILE_ADVANCED = (MOD_ALT | MOD_SHIFT);         // Advanced profile

// Test and development combinations
constexpr int MOD_TEST = (MOD_CONTROL | MOD_WIN);                   // Test mode
constexpr int MOD_DEV = (MOD_ALT | MOD_WIN);                        // Development mode
constexpr int MOD_ADMIN = (MOD_CTRL_ALT_WIN);                       // Admin mode

#endif // MOD_COMBINATIONS_DEFINED

// =============================================================================
// MODIFIER UTILITY MACROS
// =============================================================================

// Macro to check if a specific modifier combination is pressed
#define IS_MOD_PRESSED(mod) (getCurrentModifierState() == (mod))

// Macro to check if any of the modifier keys in a combination are pressed
#define HAS_MOD_KEYS(mod) ((getCurrentModifierState() & (mod)) != 0)

// Macro to check if all modifier keys in a combination are pressed
#define HAS_ALL_MOD_KEYS(mod) ((getCurrentModifierState() & (mod)) == (mod))

// Macro to check if only specific modifiers are pressed (no extra keys)
#define IS_EXACT_MOD(mod) (getCurrentModifierState() == (mod))

// Utility macros for common modifier checks
#define IS_CTRL_PRESSED() HAS_MOD_KEYS(MOD_CONTROL)
#define IS_ALT_PRESSED() HAS_MOD_KEYS(MOD_ALT)
#define IS_SHIFT_PRESSED() HAS_MOD_KEYS(MOD_SHIFT)
#define IS_WIN_PRESSED() HAS_MOD_KEYS(MOD_WIN)

#define IS_ONLY_CTRL() IS_EXACT_MOD(MOD_CONTROL)
#define IS_ONLY_ALT() IS_EXACT_MOD(MOD_ALT)
#define IS_ONLY_SHIFT() IS_EXACT_MOD(MOD_SHIFT)
#define IS_ONLY_WIN() IS_EXACT_MOD(MOD_WIN)

// Emergency detection macros
#define IS_EMERGENCY() IS_EXACT_MOD(MOD_EMERGENCY)
#define IS_EMERGENCY_ALT() IS_EXACT_MOD(MOD_EMERGENCY_ALT)
#define IS_EMERGENCY_WIN() IS_EXACT_MOD(MOD_EMERGENCY_WIN)
#define IS_ANY_EMERGENCY() (IS_EMERGENCY() || IS_EMERGENCY_ALT() || IS_EMERGENCY_WIN())

// =============================================================================
// MODIFIER VALIDATION CONSTANTS
// =============================================================================

// Valid modifier ranges for validation
constexpr int MOD_MIN_VALUE = MOD_NONE;
constexpr int MOD_MAX_VALUE = MOD_ALL_MODIFIERS;

// Forbidden modifier combinations (system reserved)
constexpr int MOD_FORBIDDEN_1 = (MOD_WIN);                          // Win key alone
constexpr int MOD_FORBIDDEN_2 = (MOD_ALT | MOD_WIN);               // Alt+Win (language switch)
constexpr int MOD_FORBIDDEN_3 = (MOD_CONTROL | MOD_ALT | MOD_WIN);  // Ctrl+Alt+Win (reserved)

// Safe modifier combinations (recommended for user keybindings)
constexpr int MOD_SAFE_COMBINATIONS[] = {
    MOD_NONE,
    MOD_CONTROL,
    MOD_ALT,
    MOD_SHIFT,
    MOD_CTRL_ALT,
    MOD_CTRL_SHIFT,
    MOD_ALT_SHIFT,
    MOD_CTRL_ALT_SHIFT
};

constexpr size_t MOD_SAFE_COMBINATIONS_COUNT = sizeof(MOD_SAFE_COMBINATIONS) / sizeof(MOD_SAFE_COMBINATIONS[0]);

// =============================================================================
// MODIFIER CONSTANTS FOR SPECIFIC PURPOSES
// =============================================================================

namespace ModifierConstants {
    // System control modifiers
    constexpr int SYSTEM_EXIT = MOD_EMERGENCY;
    constexpr int SYSTEM_RESTART = MOD_EMERGENCY_ALT;
    constexpr int SYSTEM_SHUTDOWN = MOD_EMERGENCY_WIN;
    constexpr int SYSTEM_PANIC = MOD_EMERGENCY_ALL;
    
    // Feature toggle modifiers
    constexpr int FEATURE_TOGGLE_BASIC = MOD_NONE;
    constexpr int FEATURE_TOGGLE_ADVANCED = MOD_CONTROL;
    constexpr int FEATURE_TOGGLE_EXPERT = MOD_CTRL_ALT;
    
    // Configuration modifiers
    constexpr int CONFIG_BASIC = MOD_ALT;
    constexpr int CONFIG_ADVANCED = MOD_ALT_SHIFT;
    constexpr int CONFIG_EXPERT = MOD_CTRL_ALT_SHIFT;
    
    // Debug modifiers
    constexpr int DEBUG_BASIC = MOD_CONTROL;
    constexpr int DEBUG_VERBOSE = MOD_CTRL_ALT;
    constexpr int DEBUG_EXTREME = MOD_CTRL_ALT_SHIFT;
    
    // Profile modifiers
    constexpr int PROFILE_SWITCH = MOD_NONE;
    constexpr int PROFILE_QUICK = MOD_SHIFT;
    constexpr int PROFILE_ADVANCED = MOD_ALT_SHIFT;
    
    // Adjustment modifiers
    constexpr int ADJUST_FINE = MOD_CONTROL;
    constexpr int ADJUST_COARSE = MOD_SHIFT;
    constexpr int ADJUST_PRECISE = MOD_CTRL_SHIFT;
}

// =============================================================================
// EXTENDED MODIFIER INFORMATION STRUCTURE
// =============================================================================

struct ModifierInfo {
    int value = MOD_NONE;
    std::string name = "None";
    std::string description = "No modifiers";
    int complexity = 0;
    bool is_safe = true;
    bool is_forbidden = false;
    bool is_emergency = false;
    
    ModifierInfo() = default;
    ModifierInfo(int mod_value);
    
    bool isValid() const;
    bool isSafe() const;
    bool isForbidden() const;
    bool isEmergency() const;
    std::string toString() const;
};

// =============================================================================
// KEYBINDING STRUCTURE DEFINITION
// =============================================================================

struct Keybindings {
    // System control keybindings
    unsigned int exit_vk = VK_F12;
    int exit_mod = MOD_NONE;
    
    // Movement assist keybindings
    unsigned int smart_sprint_left_vk = VK_LSHIFT;
    int smart_sprint_left_mod = MOD_NONE;
    unsigned int smart_sprint_right_vk = VK_RSHIFT;
    int smart_sprint_right_mod = MOD_NONE;
    
    // Aim assist toggle keybindings
    unsigned int aim_assist_toggle_vk = VK_F1;
    int aim_assist_toggle_mod = MOD_NONE;
    unsigned int trigger_bot_toggle_vk = VK_F2;
    int trigger_bot_toggle_mod = MOD_NONE;
    
    // Profile management keybindings
    unsigned int profile_next_vk = VK_F5;
    int profile_next_mod = MOD_NONE;
    unsigned int profile_prev_vk = VK_F6;
    int profile_prev_mod = MOD_NONE;
    
    // Statistics and monitoring keybindings
    unsigned int stats_reset_vk = VK_F3;
    int stats_reset_mod = MOD_NONE;
    unsigned int status_print_vk = VK_F4;
    int status_print_mod = MOD_NONE;
    
    // Debug and testing keybindings
    unsigned int debug_toggle_vk = VK_F7;
    int debug_toggle_mod = MOD_CONTROL;
    unsigned int test_mode_vk = VK_F8;
    int test_mode_mod = MOD_CONTROL;
    
    // Emergency shutdown keybinding
    unsigned int emergency_stop_vk = VK_ESCAPE;
    int emergency_stop_mod = MOD_EMERGENCY;
    
    // Mouse sensitivity adjustment keybindings
    unsigned int sens_increase_vk = VK_OEM_PLUS;  // + key
    int sens_increase_mod = MOD_CONTROL;
    unsigned int sens_decrease_vk = VK_OEM_MINUS; // - key
    int sens_decrease_mod = MOD_CONTROL;
    
    // FOV adjustment keybindings
    unsigned int fov_increase_vk = VK_UP;
    int fov_increase_mod = MOD_CONTROL;
    unsigned int fov_decrease_vk = VK_DOWN;
    int fov_decrease_mod = MOD_CONTROL;
    
    // Smoothing adjustment keybindings
    unsigned int smooth_increase_vk = VK_RIGHT;
    int smooth_increase_mod = MOD_CONTROL;
    unsigned int smooth_decrease_vk = VK_LEFT;
    int smooth_decrease_mod = MOD_CONTROL;
    
    // Advanced features
    unsigned int silent_aim_toggle_vk = VK_F9;
    int silent_aim_toggle_mod = MOD_CTRL_ALT;
    unsigned int prediction_toggle_vk = VK_F10;
    int prediction_toggle_mod = MOD_CTRL_ALT;
    unsigned int recoil_toggle_vk = VK_F11;
    int recoil_toggle_mod = MOD_CTRL_ALT;
    
    // Super emergency shutdown
    unsigned int super_emergency_vk = VK_F12;
    int super_emergency_mod = MOD_CTRL_ALT_SHIFT;
    
    // Default constructor
    Keybindings() = default;
    
    // Copy constructor
    Keybindings(const Keybindings& other) = default;
    
    // Assignment operator
    Keybindings& operator=(const Keybindings& other) = default;
    
    // Utility methods
    std::string toString() const;
    bool isValid() const;
    void resetToDefaults();
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
};

// Global keybindings instance declaration
extern Keybindings g_keybindings;

// Mark that keybindings are defined in globals.h to prevent redefinition
#define GLOBALS_H_KEYBINDINGS_DEFINED

// =============================================================================
// WEAPON PROFILE SYSTEM
// =============================================================================

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
};

// =============================================================================
// TARGET INFORMATION STRUCTURE
// =============================================================================

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

// =============================================================================
// MOVEMENT COMMAND STRUCTURE
// =============================================================================

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
// CORE SYSTEM VARIABLES
// =============================================================================

// System state
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_systemActive;
extern std::atomic<SystemState> g_currentSystemState;
extern std::atomic<bool> g_debugMode;
extern std::atomic<bool> g_verboseLogging;

// Aim assist system
extern std::atomic<bool> g_aimAssistEnabled;
extern std::atomic<bool> g_triggerBotEnabled;
extern std::atomic<bool> g_silentAimEnabled;
extern std::atomic<bool> g_predictionEnabled;
extern std::atomic<bool> g_recoilCompensationEnabled;
extern std::atomic<bool> g_adaptiveSensitivityEnabled;

// Input simulation
extern std::atomic<bool> g_isSimulatingInput;
extern std::atomic<bool> g_inputSystemActive;

// =============================================================================
// ADDITIONAL SYSTEM VARIABLES - CONFIG.CPP COMPATIBILITY
// =============================================================================

// System control variables
extern std::atomic<bool> g_assistEnabled;
extern std::atomic<bool> g_adaptiveSmoothingEnabled;
extern std::atomic<bool> g_audioAlertsEnabled;

// Monitor and display variables
extern std::atomic<int> g_monitorOffsetX;
extern std::atomic<int> g_monitorOffsetY;
extern std::atomic<double> g_dpiScale;
extern std::atomic<int> g_targetMonitor;

// Input control variables
extern std::atomic<unsigned int> g_toggleKey;
extern std::atomic<unsigned int> g_holdKey;
extern std::atomic<unsigned int> g_mouseButton;
extern std::atomic<bool> g_holdMode;

// Performance and optimization variables
extern std::atomic<bool> g_enableSIMD;
extern std::atomic<bool> g_enableMultithreading;
extern std::atomic<bool> g_showMetrics;
extern std::atomic<bool> g_performanceMonitoringEnabled;
extern std::atomic<bool> g_profilingEnabled;
extern std::atomic<bool> g_memoryTrackingEnabled;
extern std::atomic<bool> g_cpuMonitoringEnabled;
extern std::atomic<bool> g_fpsTrackingEnabled;
extern std::atomic<bool> g_fileOutputEnabled;
extern std::atomic<bool> g_simdOptimizationEnabled;
extern std::atomic<int> g_maxFps;

// Aim system variables
extern std::atomic<double> g_sensitivity;
extern std::atomic<double> g_predictionTimeMs;
extern std::atomic<double> g_smoothingFactor;
extern std::atomic<double> g_maxAdjustmentDistance;
extern std::string g_antiDetectionLevel;

// Memory pool variables
extern std::atomic<bool> g_memoryPoolingEnabled;
extern std::atomic<int> g_initialPoolSize;
extern std::atomic<int> g_maxPoolSize;
extern std::atomic<int> g_poolGrowthFactor;
extern std::atomic<bool> g_allowPoolExpansion;

// Input system variables
extern std::atomic<double> g_mouseSensitivity;
extern std::atomic<int> g_pollingRateHz;
extern std::atomic<bool> g_rawInputEnabled;

// GUI system variables
extern std::atomic<int> g_guiUpdateIntervalMs;
extern std::atomic<bool> g_showPerformanceOverlay;
extern std::atomic<double> g_guiTransparency;

// =============================================================================
// DISPLAY AND MONITORING VARIABLES
// =============================================================================

// Monitor settings
extern std::atomic<int> g_monitorWidth;
extern std::atomic<int> g_monitorHeight;
extern std::atomic<int> g_refreshRate;
extern std::atomic<double> g_currentFPS;
extern std::atomic<int> g_monitorCount;

// Mouse and cursor
extern std::atomic<int> g_mouseX;
extern std::atomic<int> g_mouseY;
extern std::atomic<int> g_cursorX;
extern std::atomic<int> g_cursorY;
extern std::atomic<int> g_lastMouseX;
extern std::atomic<int> g_lastMouseY;

// =============================================================================
// AIM ASSIST CONFIGURATION VARIABLES
// =============================================================================

// Targeting parameters
extern std::atomic<double> g_aimFOV;
extern std::atomic<double> g_triggerFOV;
extern std::atomic<double> g_scanFOV;
extern std::atomic<double> g_maxTargetDistance;
extern std::atomic<double> g_minConfidenceThreshold;
extern std::atomic<double> g_headConfidenceThreshold;

// Sensitivity and smoothing
extern std::atomic<double> g_globalSensitivityMultiplier;
extern std::atomic<double> g_aimSmoothingFactor;
extern std::atomic<double> g_movementSmoothingFactor;
extern std::atomic<double> g_adaptiveSensitivityFactor;

// Prediction and timing
extern std::atomic<double> g_predictionTime;
extern std::atomic<double> g_reactionDelay;
extern std::atomic<double> g_triggerDelay;
extern std::atomic<double> g_lockOnTime;

// =============================================================================
// TARGET TRACKING VARIABLES
// =============================================================================

// Current target information
extern std::atomic<bool> g_targetAcquired;
extern std::atomic<bool> g_hasValidTarget;
extern std::atomic<int> g_targetX;
extern std::atomic<int> g_targetY;
extern std::atomic<double> g_targetConfidence;
extern std::atomic<double> g_targetDistance;
extern std::atomic<int> g_targetPriority;

// Target tracking statistics
extern std::atomic<uint64_t> g_targetsDetected;
extern std::atomic<uint64_t> g_targetsAcquired;
extern std::atomic<uint64_t> g_targetsLost;
extern std::atomic<uint64_t> g_headshotsDetected;
extern std::atomic<uint64_t> g_bodyTargetsDetected;

// Target history and prediction
extern std::vector<TargetInfo>* g_targetHistory;
extern std::atomic<size_t> g_targetHistorySize;
extern std::mutex g_targetHistoryMutex;

// =============================================================================
// MOVEMENT AND STATISTICS VARIABLES
// =============================================================================

// Movement counters
extern std::atomic<uint64_t> g_totalMovements;
extern std::atomic<uint64_t> g_smoothedMovements;
extern std::atomic<uint64_t> g_predictedMovements;
extern std::atomic<uint64_t> g_compensatedMovements;
extern std::atomic<uint64_t> g_adaptiveMovements;

// Shooting statistics
extern std::atomic<uint64_t> g_totalShots;
extern std::atomic<uint64_t> g_totalHits;
extern std::atomic<uint64_t> g_headshotCount;
extern std::atomic<uint64_t> g_bodyShots;
extern std::atomic<uint64_t> g_missedShots;

// Performance metrics
extern std::atomic<uint64_t> g_frameCount;
extern std::atomic<double> g_averageFrameTime;
extern std::atomic<double> g_cpuUsage;
extern std::atomic<uint64_t> g_memoryUsage;
extern std::atomic<double> g_systemLoad;

// =============================================================================
// PROFILE AND CONFIGURATION VARIABLES
// =============================================================================

// Weapon profiles
extern std::vector<WeaponProfile>* g_weaponProfiles;
extern std::atomic<int> g_activeProfileIndex;
extern std::atomic<int> g_profileCount;
extern std::mutex g_profileMutex;

// Movement state
extern std::atomic<int> g_currentMovementState;
extern std::atomic<double> g_movementSpeed;
extern std::atomic<bool> g_isMoving;
extern std::atomic<bool> g_isSprinting;
extern std::atomic<bool> g_isCrouching;

// =============================================================================
// TIMING AND SYNCHRONIZATION VARIABLES
// =============================================================================

// System timing
extern std::atomic<uint64_t> g_systemStartTime;
extern std::atomic<uint64_t> g_lastUpdateTime;
extern std::atomic<uint64_t> g_deltaTime;
extern std::atomic<uint64_t> g_sessionTime;

// Frame timing
extern std::atomic<double> g_lastFrameTime;
extern std::atomic<double> g_targetFrameTime;
extern std::atomic<bool> g_vsyncEnabled;
extern std::atomic<double> g_frameTimeVariance;

// Thread synchronization
extern std::mutex g_globalMutex;
extern std::mutex g_configMutex;
extern std::mutex g_logMutex;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

// System initialization and shutdown
bool initializeGlobalSystem();
void shutdownGlobalSystem();
bool isSystemInitialized();
bool restartSystem();

// System state management
void setSystemState(SystemState state);
SystemState getSystemState();
std::string getSystemStateString();
bool isSystemHealthy();

// Movement state management
void setMovementState(PlayerMovementState state);
PlayerMovementState getCurrentMovementState();
std::string playerMovementStateToString(PlayerMovementState state);
bool isMovementStateValid(PlayerMovementState state);

// Profile management
bool loadWeaponProfiles(const std::string& filename);
bool saveWeaponProfiles(const std::string& filename);
WeaponProfile* getCurrentWeaponProfile();
bool setActiveProfile(int index);
int getActiveProfileIndex();
bool addWeaponProfile(const WeaponProfile& profile);
bool removeWeaponProfile(int index);
int getWeaponProfileCount();

// Target management
bool addTarget(const TargetInfo& target);
bool updateTarget(size_t index, const TargetInfo& target);
bool removeTarget(size_t index);
std::vector<TargetInfo> getActiveTargets();
TargetInfo* getBestTarget();
void clearTargetHistory();

// Statistics and monitoring
void updateSystemStatistics();
void resetGlobalStatistics();
std::vector<std::string> getSystemStatistics();
double calculateAccuracy();
double calculateEfficiency();
double calculateAverageReactionTime();

// Configuration management
bool loadGlobalConfiguration(const std::string& filename);
bool saveGlobalConfiguration(const std::string& filename);
void resetConfigurationToDefaults();
bool validateConfiguration();
void applyDefaultConfig();

// Logging and debugging
void logMessage(const std::string& message);
void logError(const std::string& message);
void logWarning(const std::string& message);
void logDebug(const std::string& message);
void logVerbose(const std::string& message);

// Utility functions
uint64_t getCurrentTimeMs();
double getCurrentTimeSec();
std::string getCurrentTimeString();
std::string getSystemStatus();
std::string getPerformanceReport();

// =============================================================================
// KEYBINDING UTILITY FUNCTION DECLARATIONS
// =============================================================================

std::string getKeybindingString(unsigned int vk, int mod);
std::string getVirtualKeyName(unsigned int vk);
std::string getModifierString(int mod);
std::string getModifierCombinationName(int mod);
bool isKeybindingPressed(unsigned int vk, int mod);
bool isModifierPressed(int modifier);
bool isExactModifierPressed(int modifier);
int getCurrentModifierState();
void initializeKeybindings();
void loadKeybindingsFromConfig();
void saveKeybindingsToConfig();
bool validateKeybinding(unsigned int vk, int mod);

// =============================================================================
// MODIFIER HELPER FUNCTIONS DECLARATIONS
// =============================================================================

// Enhanced modifier utility function declarations
bool isModifierCombinationValid(int mod);
bool isModifierCombinationSafe(int mod);
bool isModifierCombinationForbidden(int mod);
std::vector<int> getAllValidModifierCombinations();
std::vector<int> getSafeModifierCombinations();
int getNextSafeModifierCombination(int current_mod);
int getPreviousSafeModifierCombination(int current_mod);

// Modifier conversion functions
std::string modifierToHumanString(int mod);
std::string modifierToConfigString(int mod);
int configStringToModifier(const std::string& config_str);
int humanStringToModifier(const std::string& human_str);

// Modifier comparison functions
bool areModifiersEquivalent(int mod1, int mod2);
int getModifierComplexity(int mod); // Returns complexity score (0-4)
bool isModifierMoreComplex(int mod1, int mod2);

// Modifier information functions
ModifierInfo getModifierInfo(int mod);
std::vector<ModifierInfo> getAllModifierInfo();
std::vector<ModifierInfo> getSafeModifierInfo();

// =============================================================================
// GLOBAL CONSTANTS
// =============================================================================

namespace GlobalConstants {
    // System limits
    constexpr int MAX_WEAPON_PROFILES = 50;
    constexpr int MAX_TARGET_HISTORY = 1000;
    constexpr double MAX_SENSITIVITY = 10.0;
    constexpr double MIN_SENSITIVITY = 0.1;
    constexpr double MAX_SMOOTHING = 1.0;
    constexpr double MIN_SMOOTHING = 0.0;
    
    // Performance thresholds
    constexpr double TARGET_FPS = 60.0;
    constexpr double MAX_FRAME_TIME = 16.67; // milliseconds
    constexpr double MAX_CPU_USAGE = 80.0;   // percentage
    constexpr uint64_t MAX_MEMORY_USAGE = 1024 * 1024 * 1024; // 1GB
    
    // Timing constants
    constexpr double MIN_REACTION_TIME = 10.0;  // ms
    constexpr double MAX_REACTION_TIME = 500.0; // ms
    constexpr double MIN_PREDICTION_TIME = 1.0; // ms
    constexpr double MAX_PREDICTION_TIME = 200.0; // ms
    
    // FOV constants
    constexpr double MIN_FOV = 1.0;
    constexpr double MAX_FOV = 500.0;
    constexpr double DEFAULT_AIM_FOV = 100.0;
    constexpr double DEFAULT_TRIGGER_FOV = 50.0;
    constexpr double MIN_AIM_FOV = 1.0;
    constexpr double MAX_AIM_FOV = 500.0;
    
    // Distance constants
    constexpr double MIN_TARGET_DISTANCE = 10.0;
    constexpr double MAX_TARGET_DISTANCE = 2000.0;
    constexpr double DEFAULT_MAX_DISTANCE = 1000.0;
    
    // Confidence thresholds
    constexpr double MIN_CONFIDENCE = 0.1;
    constexpr double MAX_CONFIDENCE = 1.0;
    constexpr double DEFAULT_MIN_CONFIDENCE = 0.5;
    constexpr double DEFAULT_HEAD_CONFIDENCE = 0.7;
    
    // Default values for config.cpp compatibility
    constexpr double DEFAULT_SENSITIVITY = 1.0;
    constexpr double DEFAULT_SMOOTHING_FACTOR = 0.75;
    
    // File paths
    const std::string CONFIG_DIR = "./config/";
    const std::string PROFILES_FILE = CONFIG_DIR + "weapon_profiles.json";
    const std::string KEYBINDINGS_FILE = CONFIG_DIR + "keybindings.json";
    const std::string MAIN_CONFIG_FILE = CONFIG_DIR + "main_config.json";
    const std::string TARGETS_FILE = CONFIG_DIR + "targets.json";
    const std::string LOG_FILE = "./logs/system.log";
    const std::string DEBUG_LOG_FILE = "./logs/debug.log";
    const std::string PERFORMANCE_LOG_FILE = "./logs/performance.log";
}

// =============================================================================
// UTILITY MACROS
// =============================================================================

#define GLOBALS_H_INCLUDED
#define SAFE_ATOMIC_LOAD(var) (var.load(std::memory_order_acquire))
#define SAFE_ATOMIC_STORE(var, val) (var.store(val, std::memory_order_release))
#define SAFE_ATOMIC_EXCHANGE(var, val) (var.exchange(val, std::memory_order_acq_rel))

#define LOG_SYSTEM(msg) logMessage("[SYSTEM] " + std::string(msg))
#define LOG_ERROR_SYSTEM(msg) logError("[SYSTEM] " + std::string(msg))
#define LOG_WARNING_SYSTEM(msg) logWarning("[SYSTEM] " + std::string(msg))
#define LOG_DEBUG_SYSTEM(msg) if(g_debugMode.load()) logDebug("[SYSTEM] " + std::string(msg))
#define LOG_VERBOSE_SYSTEM(msg) if(g_verboseLogging.load()) logVerbose("[SYSTEM] " + std::string(msg))

#define UPDATE_COUNTER(counter) (counter.fetch_add(1, std::memory_order_relaxed))
#define RESET_COUNTER(counter) (counter.store(0, std::memory_order_relaxed))
#define INCREMENT_BY(counter, value) (counter.fetch_add(value, std::memory_order_relaxed))

#define CLAMP(value, min_val, max_val) (std::max(min_val, std::min(value, max_val)))
#define IS_VALID_RANGE(value, min_val, max_val) ((value) >= (min_val) && (value) <= (max_val))

// Thread-safe getter/setter macros
#define GET_ATOMIC(var) SAFE_ATOMIC_LOAD(var)
#define SET_ATOMIC(var, val) SAFE_ATOMIC_STORE(var, val)

#endif // GLOBALS_H