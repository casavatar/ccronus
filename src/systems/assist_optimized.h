// assist_optimized.h - CORRECTED VERSION v3.1.3
// description: Optimized aim assist system header - FIXED
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.3 - Fixed atomic copy issues and redefinitions
// date: 2025-07-17
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

// =============================================================================
// AIM ASSIST CONFIGURATION STRUCTURES
// =============================================================================

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
    double micro_movement_chance = 0.3;
    
    AimAssistConfig() = default;
};

struct TargetPriorityConfig {
    bool prefer_head = true;
    bool prefer_chest = false;
    bool prefer_closest = true;
    bool prefer_center_screen = false;
    
    double head_priority_weight = 1.5;
    double chest_priority_weight = 1.0;
    double distance_weight = 0.8;
    double screen_center_weight = 0.6;
    
    TargetPriorityConfig() = default;
};

// =============================================================================
// TARGET INFORMATION STRUCTURES
// =============================================================================

enum class TargetType {
    UNKNOWN,
    PLAYER_HEAD,
    PLAYER_CHEST,
    PLAYER_BODY,
    VEHICLE,
    DESTRUCTIBLE
};

enum class TargetTeam {
    UNKNOWN,
    FRIENDLY,
    ENEMY,
    NEUTRAL
};

struct TargetInfo {
    bool valid = false;
    TargetType type = TargetType::UNKNOWN;
    TargetTeam team = TargetTeam::UNKNOWN;
    
    // Position information
    double screen_x = 0.0;
    double screen_y = 0.0;
    double world_x = 0.0;
    double world_y = 0.0;
    double world_z = 0.0;
    
    // Target properties
    double distance = 0.0;
    double health = 100.0;
    double confidence = 0.0;
    double visibility = 1.0;
    
    // Movement information
    double velocity_x = 0.0;
    double velocity_y = 0.0;
    double velocity_z = 0.0;
    double angular_velocity = 0.0;
    
    // Prediction data
    double predicted_x = 0.0;
    double predicted_y = 0.0;
    double prediction_accuracy = 0.0;
    
    // Timing information
    std::chrono::steady_clock::time_point first_detected;
    std::chrono::steady_clock::time_point last_updated;
    uint64_t detection_count = 0;
    
    TargetInfo() : first_detected(std::chrono::steady_clock::now()), 
                   last_updated(std::chrono::steady_clock::now()) {}
    
    // Utility methods
    double getAge() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - first_detected);
        return static_cast<double>(duration.count());
    }
    
    double getTimeSinceUpdate() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_updated);
        return static_cast<double>(duration.count());
    }
    
    bool isStale(double max_age_ms = 500.0) const {
        return getTimeSinceUpdate() > max_age_ms;
    }
    
    void updatePosition(double x, double y, double conf = 1.0) {
        screen_x = x;
        screen_y = y;
        confidence = conf;
        last_updated = std::chrono::steady_clock::now();
        detection_count++;
    }
};

// ✅ REMOVED OptimizedTargetInfo - avoiding redefinition

// =============================================================================
// AIM CALCULATION STRUCTURES
// =============================================================================

struct AimCalculation {
    double delta_x = 0.0;
    double delta_y = 0.0;
    double smoothed_x = 0.0;
    double smoothed_y = 0.0;
    double predicted_x = 0.0;
    double predicted_y = 0.0;
    
    double confidence = 0.0;
    double smoothing_applied = 0.0;
    double prediction_applied = 0.0;
    
    bool should_aim = false;
    bool should_shoot = false;
    bool requires_prediction = false;
    
    std::chrono::steady_clock::time_point calculation_time;
    
    AimCalculation() : calculation_time(std::chrono::steady_clock::now()) {}
};

// ✅ REMOVED PIDResponse - avoiding redefinition

// ✅ REMOVED MovementCommand - avoiding redefinition (defined in globals.h)

// =============================================================================
// PERFORMANCE MONITORING STRUCTURES - FIXED ATOMIC COPY ISSUE
// =============================================================================

struct AimAssistMetrics {
    // Performance counters
    std::atomic<uint64_t> targets_detected{0};
    std::atomic<uint64_t> targets_acquired{0};
    std::atomic<uint64_t> shots_taken{0};
    std::atomic<uint64_t> shots_hit{0};
    std::atomic<uint64_t> movements_executed{0};
    
    // Timing metrics
    std::atomic<double> avg_detection_time{0.0};
    std::atomic<double> avg_calculation_time{0.0};
    std::atomic<double> avg_movement_time{0.0};
    std::atomic<double> avg_frame_time{0.0};
    
    // Accuracy metrics
    std::atomic<double> hit_ratio{0.0};
    std::atomic<double> avg_confidence{0.0};
    std::atomic<double> prediction_accuracy{0.0};
    
    // System health
    std::atomic<bool> system_healthy{true};
    std::atomic<double> cpu_usage{0.0};
    std::atomic<uint64_t> memory_usage{0};
    std::atomic<double> fps{60.0};
    
    // ✅ FIXED: Delete copy constructor and assignment operator
    AimAssistMetrics() = default;
    AimAssistMetrics(const AimAssistMetrics&) = delete;
    AimAssistMetrics& operator=(const AimAssistMetrics&) = delete;
    
    // ✅ FIXED: Move constructor and assignment
    AimAssistMetrics(AimAssistMetrics&&) = default;
    AimAssistMetrics& operator=(AimAssistMetrics&&) = default;
    
    void reset() {
        targets_detected.store(0);
        targets_acquired.store(0);
        shots_taken.store(0);
        shots_hit.store(0);
        movements_executed.store(0);
        
        avg_detection_time.store(0.0);
        avg_calculation_time.store(0.0);
        avg_movement_time.store(0.0);
        avg_frame_time.store(0.0);
        
        hit_ratio.store(0.0);
        avg_confidence.store(0.0);
        prediction_accuracy.store(0.0);
        
        system_healthy.store(true);
        cpu_usage.store(0.0);
        memory_usage.store(0);
        fps.store(60.0);
    }
    
    double calculateAccuracy() const {
        uint64_t total = shots_taken.load();
        uint64_t hits = shots_hit.load();
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
    
    double calculateDetectionRate() const {
        uint64_t detected = targets_detected.load();
        uint64_t acquired = targets_acquired.load();
        return detected > 0 ? static_cast<double>(acquired) / detected : 0.0;
    }
    
    // ✅ FIXED: Create snapshot for returning metrics
    struct MetricsSnapshot {
        uint64_t targets_detected;
        uint64_t targets_acquired;
        uint64_t shots_taken;
        uint64_t shots_hit;
        uint64_t movements_executed;
        
        double avg_detection_time;
        double avg_calculation_time;
        double avg_movement_time;
        double avg_frame_time;
        
        double hit_ratio;
        double avg_confidence;
        double prediction_accuracy;
        
        bool system_healthy;
        double cpu_usage;
        uint64_t memory_usage;
        double fps;
    };
    
    MetricsSnapshot getSnapshot() const {
        MetricsSnapshot snapshot;
        snapshot.targets_detected = targets_detected.load();
        snapshot.targets_acquired = targets_acquired.load();
        snapshot.shots_taken = shots_taken.load();
        snapshot.shots_hit = shots_hit.load();
        snapshot.movements_executed = movements_executed.load();
        
        snapshot.avg_detection_time = avg_detection_time.load();
        snapshot.avg_calculation_time = avg_calculation_time.load();
        snapshot.avg_movement_time = avg_movement_time.load();
        snapshot.avg_frame_time = avg_frame_time.load();
        
        snapshot.hit_ratio = hit_ratio.load();
        snapshot.avg_confidence = avg_confidence.load();
        snapshot.prediction_accuracy = prediction_accuracy.load();
        
        snapshot.system_healthy = system_healthy.load();
        snapshot.cpu_usage = cpu_usage.load();
        snapshot.memory_usage = memory_usage.load();
        snapshot.fps = fps.load();
        
        return snapshot;
    }
};

// =============================================================================
// AIM ASSIST SYSTEM CLASS INTERFACE
// =============================================================================

class OptimizedAimAssistSystem {
private:
    AimAssistConfig config;
    TargetPriorityConfig priority_config;
    AimAssistMetrics metrics;
    
    std::atomic<bool> active{false};
    std::atomic<bool> enabled{false};
    std::atomic<bool> processing{false};
    
    std::vector<TargetInfo> detected_targets;
    TargetInfo current_target;
    
    // Internal state
    std::chrono::steady_clock::time_point last_update;
    uint64_t frame_counter = 0;
    
public:
    OptimizedAimAssistSystem() = default;
    ~OptimizedAimAssistSystem() = default;
    
    // ✅ FIXED: Delete copy operations
    OptimizedAimAssistSystem(const OptimizedAimAssistSystem&) = delete;
    OptimizedAimAssistSystem& operator=(const OptimizedAimAssistSystem&) = delete;
    
    // System control
    bool initialize();
    void shutdown();
    bool isActive() const { return active.load(); }
    bool isEnabled() const { return enabled.load(); }
    
    // Configuration
    void setConfig(const AimAssistConfig& cfg) { config = cfg; }
    AimAssistConfig getConfig() const { return config; }
    void setPriorityConfig(const TargetPriorityConfig& cfg) { priority_config = cfg; }
    TargetPriorityConfig getPriorityConfig() const { return priority_config; }
    
    // Main processing
    void update();
    void processFrame();
    bool acquireTarget();
    AimCalculation calculateAim();
    
    // Target management
    std::vector<TargetInfo> detectTargets();
    TargetInfo selectBestTarget(const std::vector<TargetInfo>& targets);
    double calculateTargetPriority(const TargetInfo& target);
    bool validateTarget(const TargetInfo& target);
    
    // Prediction and smoothing
    TargetInfo predictTargetMovement(const TargetInfo& target, double time_ms);
    
    // ✅ FIXED: Return metrics snapshot instead of copy
    AimAssistMetrics::MetricsSnapshot getMetrics() const { return metrics.getSnapshot(); }
    void resetMetrics() { metrics.reset(); }
    std::vector<std::string> getPerformanceStats();
    void updatePerformanceMetrics();
    
    // Weapon profile integration
    void updateWeaponSettings(const WeaponProfile& profile);
    void optimizeForWeapon(const WeaponProfile& profile);
    
    // Safety and detection avoidance
    bool isSafeToOperate();
    void applyDetectionAvoidance();
    bool passesSecurityChecks();
};

// =============================================================================
// GLOBAL FUNCTION DECLARATIONS
// =============================================================================

// Core aim assist functions
void enhancedHeadshotAimAssist();

// Target detection and analysis
std::vector<TargetInfo> scanForTargets();
TargetInfo analyzeTarget(double x, double y);
double calculateTargetConfidence(double x, double y);
bool isValidTarget(const TargetInfo& target);

// Weapon and profile management
void updateAimAssistSettings(const WeaponProfile& profile);
WeaponProfile getOptimalProfileForWeapon(const std::string& weapon_name);
void calibrateAimAssistForProfile(const WeaponProfile& profile);

// Performance and monitoring
std::vector<std::string> getAimAssistPerformanceStats();
void resetAimAssistStats();
void updateAimAssistMetrics();
double getAimAssistAccuracy();
double getAimAssistEfficiency();

// System control
bool initializeAimAssistSystem();
void shutdownAimAssistSystem();
void runOptimizedAimAssistLoop();
bool isAimAssistSystemActive();
void setAimAssistEnabled(bool enabled);

// Configuration and calibration
void loadAimAssistConfig(const std::string& filename);
void saveAimAssistConfig(const std::string& filename);
void calibrateAimAssistSystem();
void optimizeAimAssistPerformance();

// Security and anti-detection
bool performSecurityChecks();
void enableAntiDetectionMeasures();
void randomizeAimPatterns();
bool isOperationSafe();

// Advanced features
void enablePredictiveAiming(bool enable);
void enableRecoilCompensation(bool enable);
void enableTriggerBot(bool enable);
void setSilentAimMode(bool enable);

// Utility functions
double calculateDistance(double x1, double y1, double x2, double y2);
double calculateAngle(double x1, double y1, double x2, double y2);
bool isPointInFOV(double x, double y, double fov_radius);
double normalizeAngle(double angle);

// Debug and testing
void enableAimAssistDebugMode(bool enable);
void logAimAssistEvent(const std::string& event);
void dumpAimAssistState();
std::string getAimAssistSystemStatus();

// =============================================================================
// GLOBAL VARIABLES DECLARATIONS
// =============================================================================

// System state
extern std::atomic<bool> g_aimAssistSystemActive;
extern std::atomic<bool> g_aimAssistEnabled;
extern std::atomic<bool> g_isSimulatingInput;
extern std::atomic<bool> g_triggerBotEnabled;
extern std::atomic<bool> g_silentAimEnabled;

// Current target information
extern std::atomic<double> g_currentTargetX;
extern std::atomic<double> g_currentTargetY;
extern std::atomic<double> g_currentTargetConfidence;
extern std::atomic<bool> g_hasValidTarget;

// Performance metrics
extern std::atomic<uint64_t> g_aimAssistCalculations;
extern std::atomic<uint64_t> g_successfulAims;
extern std::atomic<double> g_averageAimTime;
extern std::atomic<double> g_systemEfficiency;

// =============================================================================
// CONSTANTS AND LIMITS
// =============================================================================

namespace AimAssistConstants {
    // System limits
    constexpr double MAX_AIM_DISTANCE = 2000.0;
    constexpr double MIN_AIM_DISTANCE = 5.0;
    constexpr double MAX_FOV_RADIUS = 500.0;
    constexpr double MIN_FOV_RADIUS = 10.0;
    
    // Performance thresholds
    constexpr double MIN_CONFIDENCE_THRESHOLD = 0.3;
    constexpr double MAX_CALCULATION_TIME_MS = 5.0;
    constexpr int MAX_TARGETS_PER_FRAME = 50;
    constexpr int TARGET_HISTORY_SIZE = 100;
    
    // Movement limits
    constexpr double MAX_MOVEMENT_PER_FRAME = 100.0;
    constexpr double MIN_MOVEMENT_THRESHOLD = 0.1;
    constexpr double MAX_SMOOTHING_FACTOR = 1.0;
    constexpr double MIN_SMOOTHING_FACTOR = 0.01;
    
    // Timing constraints
    constexpr double MAX_PREDICTION_TIME_MS = 500.0;
    constexpr double MIN_REACTION_TIME_MS = 10.0;
    constexpr double MAX_REACTION_TIME_MS = 200.0;
    
    // Detection avoidance
    constexpr double MAX_RANDOMIZATION = 0.5;
    constexpr double MIN_HUMANIZATION_DELAY = 5.0;
    constexpr double MAX_HUMANIZATION_DELAY = 50.0;
    
    // File paths
    const std::string CONFIG_FILE = "./config/aim_assist.json";
    const std::string PROFILES_FILE = "./config/aim_profiles.json";
    const std::string METRICS_FILE = "./logs/aim_metrics.log";
}

// =============================================================================
// UTILITY MACROS
// =============================================================================

#define AIM_ASSIST_PROFILE_SCOPE(name) \
    auto start_time = std::chrono::high_resolution_clock::now(); \
    auto end_time_guard = [&]() { \
        auto end_time = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time); \
        logAimAssistEvent(name + " took " + std::to_string(duration.count()) + "μs"); \
    }; \
    std::unique_ptr<void, decltype(end_time_guard)> profile_guard(nullptr, end_time_guard);

#define SAFE_AIM_ASSIST_CALL(func) \
    do { \
        if (g_aimAssistSystemActive.load() && isOperationSafe()) { \
            func; \
        } \
    } while(0)

#define UPDATE_AIM_METRIC(metric, value) \
    do { \
        g_##metric.store(value, std::memory_order_relaxed); \
    } while(0)

#define INCREMENT_AIM_COUNTER(counter) \
    do { \
        g_##counter.fetch_add(1, std::memory_order_relaxed); \
    } while(0)

#define AIM_ASSIST_LOG(level, message) \
    do { \
        if (g_debugMode.load()) { \
            logAimAssistEvent("[" + std::string(level) + "] " + message); \
        } \
    } while(0)

// =============================================================================
// TEMPLATE UTILITIES
// =============================================================================

template<typename T>
constexpr T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

template<typename T>
constexpr T lerp(T a, T b, double t) {
    return a + static_cast<T>((b - a) * t);
}

template<typename T>
constexpr double distance2D(T x1, T y1, T x2, T y2) {
    T dx = x2 - x1;
    T dy = y2 - y1;
    return std::sqrt(static_cast<double>(dx * dx + dy * dy)); // ✅ FIXED: std::sqrt
}

template<typename T>
constexpr bool inRange(T value, T min_val, T max_val) {
    return value >= min_val && value <= max_val;
}

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

inline double degToRad(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

inline double radToDeg(double radians) {
    return radians * 180.0 / 3.14159265358979323846;
}

inline bool isZero(double value, double epsilon = 1e-9) {
    return std::abs(value) < epsilon;
}

inline double smoothstep(double edge0, double edge1, double x) {
    double t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

inline uint64_t getCurrentTimeMs() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}