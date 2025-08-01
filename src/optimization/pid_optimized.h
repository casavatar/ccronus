// pid_optimized.h - CORRECTED VERSION v3.0.6
// --------------------------------------------------------------------------------------
// description: Optimized PID controller system with SIMD and memory pooling
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 3.0.6 - Fixed ObjectPool template conflicts
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <thread>
#include <algorithm>
#include <cctype>
#include "common_defines.h"

// Forward declarations - CORRECTED
template<typename T> class ObjectPool;
class PoolManager;
struct PoolConfig;
struct PoolStatistics;

// =============================================================================
// PID CONFIGURATION STRUCTURES
// =============================================================================
struct PIDOptimizationConfig {
    bool enable_simd = true;
    bool enable_adaptive_tuning = true;
    bool enable_windup_protection = true;
    bool enable_derivative_filtering = true;
    bool batch_processing = true;
    
    double max_integral = 100.0;
    double derivative_filter_alpha = 0.1;
    double adaptive_gain = 0.01;
    double windup_threshold = 0.8;
    
    double performance_threshold = 0.95;
    double stability_margin = 0.1;
    double response_time_target = 50.0; // ms
    
    // Constructor declaration
    PIDOptimizationConfig();
    
    // Method declarations
    bool validate() const;
    void reset();
};

struct PIDGains {
    double kp = 1.0;
    double ki = 0.1;
    double kd = 0.05;
    
    PIDGains() = default;
    PIDGains(double p, double i, double d) : kp(p), ki(i), kd(d) {}
};

// =============================================================================
// PERFORMANCE METRICS
// =============================================================================
struct PIDPerformanceMetrics {
    std::atomic<uint64_t> total_calculations{0};
    std::atomic<uint64_t> avg_calculation_time_ns{0};
    std::atomic<uint64_t> peak_calculation_time_ns{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    std::atomic<uint64_t> simd_operations{0};
    std::atomic<uint64_t> scalar_operations{0};
    std::atomic<uint64_t> adaptive_adjustments{0};
    std::atomic<uint64_t> stability_violations{0};
    std::atomic<uint64_t> windup_events{0};
    std::chrono::steady_clock::time_point last_reset_time;
    
    // Constructor declaration
    PIDPerformanceMetrics();
    
    // Copy constructor for atomics
    PIDPerformanceMetrics(const PIDPerformanceMetrics& other);
    PIDPerformanceMetrics(PIDPerformanceMetrics&& other) noexcept;
    PIDPerformanceMetrics& operator=(const PIDPerformanceMetrics& other);
    PIDPerformanceMetrics& operator=(PIDPerformanceMetrics&& other) noexcept;
    
    // Method declarations
    void reset();
    double getCacheHitRatio() const;
    double getSIMDUtilization() const;
    std::chrono::milliseconds getUptime() const;
};

// =============================================================================
// OUTPUT LIMITS STRUCTURE
// =============================================================================
struct OutputLimits {
    bool enabled = false;
    double min_output = -100.0;
    double max_output = 100.0;
};

// =============================================================================
// OPTIMIZED PID CONTROLLER
// =============================================================================
class OptimizedPIDController {
public:
    // Constructor
    OptimizedPIDController(double kp, double ki, double kd, 
                          const PIDOptimizationConfig& config = PIDOptimizationConfig{});
    
    // Destructor
    ~OptimizedPIDController();
    
    // Core calculation methods
    double calculate(double error, double dt);
    
    void calculateBatch(const std::vector<double>& errors,
                       const std::vector<double>& dts,
                       std::vector<double>& outputs);
    
    // Configuration methods
    void setGains(double kp, double ki, double kd);
    void getGains(double& kp, double& ki, double& kd) const;
    void setOutputLimits(double min_output, double max_output);
    void enableOutputLimits(bool enable);
    
    // State management
    void reset();
    
    // Metrics and diagnostics
    PIDPerformanceMetrics getMetrics() const;
    std::vector<std::string> getDiagnosticInfo() const;
    
private:
    // PID gains
    double m_kp, m_ki, m_kd;
    
    // State variables
    double m_integral = 0.0;
    double m_last_error = 0.0;
    double m_last_derivative = 0.0;
    double m_last_output = 0.0;
    std::chrono::steady_clock::time_point m_last_time;
    
    // Configuration
    PIDOptimizationConfig m_config;
    OutputLimits m_output_limits;
    
    // History for analysis
    std::vector<double> m_error_history;
    std::vector<double> m_output_history;
    
    // Performance metrics
    PIDPerformanceMetrics m_metrics;
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // Private methods
    double calculateInternal(double error, double dt);
    void updateHistory(double error, double output);
    void updateMetrics(std::chrono::nanoseconds calculation_time);
    void performAdaptiveTuning(double error, double output, double dt);
};

// =============================================================================
// MULTI-AXIS PID CONTROLLER
// =============================================================================
class MultiAxisPIDController {
public:
    // Constructor
    MultiAxisPIDController(size_t num_axes, 
                          const std::vector<PIDGains>& gains,
                          const PIDOptimizationConfig& config = PIDOptimizationConfig{});
    
    // Destructor
    ~MultiAxisPIDController();
    
    // Core calculation method
    std::vector<double> calculate(const std::vector<double>& errors,
                                 const std::vector<double>& dts);
    
    // Configuration methods
    void setGains(size_t axis, double kp, double ki, double kd);
    void setOutputLimits(size_t axis, double min_output, double max_output);
    
    // State management
    void reset();
    
    // Metrics and access
    std::vector<PIDPerformanceMetrics> getAllMetrics() const;
    OptimizedPIDController* getController(size_t axis);
    
private:
    size_t m_num_axes;
    PIDOptimizationConfig m_config;
    std::vector<std::unique_ptr<OptimizedPIDController>> m_controllers;
    
    // Private methods
    void initializeControllers(const std::vector<PIDGains>& gains);
};

// =============================================================================
// PID POOL MANAGER - SIMPLIFIED WITHOUT TEMPLATES
// =============================================================================
class PIDPoolManager {
public:
    // Singleton pattern
    static PIDPoolManager& getInstance();
    
    // Direct creation methods (no pools for now due to template conflicts)
    std::unique_ptr<OptimizedPIDController> acquirePIDController(double kp, double ki, double kd,
                                                               const PIDOptimizationConfig& config = PIDOptimizationConfig{});
    
    std::unique_ptr<MultiAxisPIDController> acquireMultiAxisController(size_t num_axes,
                                                                      const std::vector<PIDGains>& gains,
                                                                      const PIDOptimizationConfig& config = PIDOptimizationConfig{});
    
    // Statistics and management
    std::vector<std::string> getPoolStatistics() const;
    void clearPools();
    
    // Constructor and destructor
    PIDPoolManager();
    ~PIDPoolManager();
    
private:
    // Deleted copy operations
    PIDPoolManager(const PIDPoolManager&) = delete;
    PIDPoolManager& operator=(const PIDPoolManager&) = delete;
    
    // Pool initialization
    void initializePools();
    
    // Statistics tracking
    mutable std::mutex m_stats_mutex;
    std::atomic<size_t> m_pid_controllers_created{0};
    std::atomic<size_t> m_multi_axis_controllers_created{0};
    std::chrono::steady_clock::time_point m_start_time;
};

// =============================================================================
// GLOBAL FUNCTIONS
// =============================================================================
bool initializePIDSystem();
void shutdownPIDSystem();

std::unique_ptr<OptimizedPIDController> createOptimizedPID(double kp, double ki, double kd,
                                                          const PIDOptimizationConfig& config = PIDOptimizationConfig{});

std::unique_ptr<MultiAxisPIDController> createMultiAxisPID(size_t num_axes,
                                                          const std::vector<PIDGains>& gains,
                                                          const PIDOptimizationConfig& config = PIDOptimizationConfig{});

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================
#define CREATE_PID(kp, ki, kd) createOptimizedPID(kp, ki, kd)
#define CREATE_MULTI_PID(axes, gains) createMultiAxisPID(axes, gains)
#define PID_POOL() PIDPoolManager::getInstance()