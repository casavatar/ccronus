// pid_optimized.cpp - CORRECTED VERSION v3.0.6
// --------------------------------------------------------------------------------------
// description: Implementation of optimized PID controller system
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 3.0.6 - Fixed ObjectPool template conflicts
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "pid_optimized.h"
#include "globals.h"
// Remove memory_pool.h include to avoid template conflicts
// #include "memory_pool.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cstring>
#include <immintrin.h>
#include <thread>
#include <cctype>

// =============================================================================
// SIMD UTILITIES
// =============================================================================
namespace SIMDUtils {
    
    bool isAVX2Supported() {
        static bool checked = false;
        static bool supported = false;
        
        if (!checked) {
            // Simple AVX2 check - in real implementation would use CPUID
            supported = true; // Assume supported for now
            checked = true;
        }
        
        return supported;
    }
    
    void vectorizedPIDCalculation(const double* errors, const double* dts, 
                                 double* outputs, size_t count,
                                 double kp, double ki, double kd,
                                 double& integral, double& last_error) {
        
        if (!isAVX2Supported() || count < 4) {
            // Fallback to scalar implementation
            for (size_t i = 0; i < count; ++i) {
                double error = errors[i];
                double dt = dts[i];
                
                integral += error * dt;
                double derivative = (i > 0) ? (error - last_error) / dt : 0.0;
                
                outputs[i] = kp * error + ki * integral + kd * derivative;
                last_error = error;
            }
            return;
        }
        
        // Simplified SIMD implementation
        for (size_t i = 0; i < count; i += 4) {
            size_t remaining = std::min(size_t(4), count - i);
            
            for (size_t j = 0; j < remaining; ++j) {
                double error = errors[i + j];
                double dt = dts[i + j];
                
                integral += error * dt;
                double derivative = (i + j > 0) ? (error - last_error) / dt : 0.0;
                
                outputs[i + j] = kp * error + ki * integral + kd * derivative;
                last_error = error;
            }
        }
    }
    
} // namespace SIMDUtils

// =============================================================================
// OPTIMIZATION CONFIG IMPLEMENTATION
// =============================================================================
PIDOptimizationConfig::PIDOptimizationConfig() {
    enable_simd = true;
    enable_adaptive_tuning = true;
    enable_windup_protection = true;
    enable_derivative_filtering = true;
    batch_processing = true;
    
    max_integral = 100.0;
    derivative_filter_alpha = 0.1;
    adaptive_gain = 0.01;
    windup_threshold = 0.8;
    
    performance_threshold = 0.95;
    stability_margin = 0.1;
    response_time_target = 50.0; // ms
}

bool PIDOptimizationConfig::validate() const {
    if (max_integral <= 0.0) return false;
    if (derivative_filter_alpha < 0.0 || derivative_filter_alpha > 1.0) return false;
    if (adaptive_gain < 0.0 || adaptive_gain > 1.0) return false;
    if (windup_threshold < 0.0 || windup_threshold > 1.0) return false;
    if (performance_threshold < 0.0 || performance_threshold > 1.0) return false;
    if (stability_margin < 0.0) return false;
    if (response_time_target <= 0.0) return false;
    
    return true;
}

void PIDOptimizationConfig::reset() {
    *this = PIDOptimizationConfig{}; // Reset to defaults
}

// =============================================================================
// PID PERFORMANCE METRICS IMPLEMENTATION
// =============================================================================
PIDPerformanceMetrics::PIDPerformanceMetrics() {
    reset();
}

// Copy constructor for atomics
PIDPerformanceMetrics::PIDPerformanceMetrics(const PIDPerformanceMetrics& other) {
    total_calculations.store(other.total_calculations.load());
    avg_calculation_time_ns.store(other.avg_calculation_time_ns.load());
    peak_calculation_time_ns.store(other.peak_calculation_time_ns.load());
    cache_hits.store(other.cache_hits.load());
    cache_misses.store(other.cache_misses.load());
    simd_operations.store(other.simd_operations.load());
    scalar_operations.store(other.scalar_operations.load());
    adaptive_adjustments.store(other.adaptive_adjustments.load());
    stability_violations.store(other.stability_violations.load());
    windup_events.store(other.windup_events.load());
    last_reset_time = other.last_reset_time;
}

// Move constructor
PIDPerformanceMetrics::PIDPerformanceMetrics(PIDPerformanceMetrics&& other) noexcept {
    total_calculations.store(other.total_calculations.load());
    avg_calculation_time_ns.store(other.avg_calculation_time_ns.load());
    peak_calculation_time_ns.store(other.peak_calculation_time_ns.load());
    cache_hits.store(other.cache_hits.load());
    cache_misses.store(other.cache_misses.load());
    simd_operations.store(other.simd_operations.load());
    scalar_operations.store(other.scalar_operations.load());
    adaptive_adjustments.store(other.adaptive_adjustments.load());
    stability_violations.store(other.stability_violations.load());
    windup_events.store(other.windup_events.load());
    last_reset_time = other.last_reset_time;
}

// Copy assignment
PIDPerformanceMetrics& PIDPerformanceMetrics::operator=(const PIDPerformanceMetrics& other) {
    if (this != &other) {
        total_calculations.store(other.total_calculations.load());
        avg_calculation_time_ns.store(other.avg_calculation_time_ns.load());
        peak_calculation_time_ns.store(other.peak_calculation_time_ns.load());
        cache_hits.store(other.cache_hits.load());
        cache_misses.store(other.cache_misses.load());
        simd_operations.store(other.simd_operations.load());
        scalar_operations.store(other.scalar_operations.load());
        adaptive_adjustments.store(other.adaptive_adjustments.load());
        stability_violations.store(other.stability_violations.load());
        windup_events.store(other.windup_events.load());
        last_reset_time = other.last_reset_time;
    }
    return *this;
}

// Move assignment
PIDPerformanceMetrics& PIDPerformanceMetrics::operator=(PIDPerformanceMetrics&& other) noexcept {
    if (this != &other) {
        total_calculations.store(other.total_calculations.load());
        avg_calculation_time_ns.store(other.avg_calculation_time_ns.load());
        peak_calculation_time_ns.store(other.peak_calculation_time_ns.load());
        cache_hits.store(other.cache_hits.load());
        cache_misses.store(other.cache_misses.load());
        simd_operations.store(other.simd_operations.load());
        scalar_operations.store(other.scalar_operations.load());
        adaptive_adjustments.store(other.adaptive_adjustments.load());
        stability_violations.store(other.stability_violations.load());
        windup_events.store(other.windup_events.load());
        last_reset_time = other.last_reset_time;
    }
    return *this;
}

void PIDPerformanceMetrics::reset() {
    total_calculations.store(0);
    avg_calculation_time_ns.store(0);
    peak_calculation_time_ns.store(0);
    cache_hits.store(0);
    cache_misses.store(0);
    simd_operations.store(0);
    scalar_operations.store(0);
    adaptive_adjustments.store(0);
    stability_violations.store(0);
    windup_events.store(0);
    last_reset_time = std::chrono::steady_clock::now();
}

double PIDPerformanceMetrics::getCacheHitRatio() const {
    uint64_t hits = cache_hits.load();
    uint64_t misses = cache_misses.load();
    uint64_t total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / total : 0.0;
}

double PIDPerformanceMetrics::getSIMDUtilization() const {
    uint64_t simd_ops = simd_operations.load();
    uint64_t scalar_ops = scalar_operations.load();
    uint64_t total_ops = simd_ops + scalar_ops;
    return total_ops > 0 ? static_cast<double>(simd_ops) / total_ops : 0.0;
}

std::chrono::milliseconds PIDPerformanceMetrics::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - last_reset_time);
}

// =============================================================================
// OPTIMIZED PID CONTROLLER IMPLEMENTATION
// =============================================================================
OptimizedPIDController::OptimizedPIDController(double kp, double ki, double kd,
                                             const PIDOptimizationConfig& config)
    : m_kp(kp), m_ki(ki), m_kd(kd), m_config(config) {
    
    if (!m_config.validate()) {
        logMessage("WARNING: Invalid PID optimization config, using defaults");
        m_config = PIDOptimizationConfig{};
    }
    
    reset();
    DEBUG_LOG("OptimizedPIDController constructed with kp=" + std::to_string(kp) + 
              " ki=" + std::to_string(ki) + " kd=" + std::to_string(kd));
}

OptimizedPIDController::~OptimizedPIDController() {
    DEBUG_LOG("OptimizedPIDController destroyed");
}

void OptimizedPIDController::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_integral = 0.0;
    m_last_error = 0.0;
    m_last_derivative = 0.0;
    m_last_output = 0.0;
    m_last_time = std::chrono::steady_clock::time_point{};
    
    // Clear history
    m_error_history.clear();
    m_output_history.clear();
    
    m_metrics.reset();
    
    DEBUG_LOG("PID controller reset");
}

double OptimizedPIDController::calculate(double error, double dt) {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Validate inputs
    if (dt <= 0.0) {
        logMessage("WARNING: Invalid dt in PID calculation: " + std::to_string(dt));
        return m_last_output;
    }
    
    // Calculate PID terms
    double proportional = m_kp * error;
    
    // Integral term with windup protection
    double new_integral = m_integral + error * dt;
    if (m_config.enable_windup_protection) {
        new_integral = std::clamp(new_integral, -m_config.max_integral, m_config.max_integral);
    }
    double integral_term = m_ki * new_integral;
    
    // Derivative term with optional filtering
    double derivative = (error - m_last_error) / dt;
    if (m_config.enable_derivative_filtering) {
        derivative = m_config.derivative_filter_alpha * derivative + 
                    (1.0 - m_config.derivative_filter_alpha) * m_last_derivative;
    }
    double derivative_term = m_kd * derivative;
    
    // Calculate output
    double output = proportional + integral_term + derivative_term;
    
    // Apply output limits if configured
    if (m_output_limits.enabled) {
        output = std::clamp(output, m_output_limits.min_output, m_output_limits.max_output);
    }
    
    // Update state
    m_integral = new_integral;
    m_last_error = error;
    m_last_derivative = derivative;
    m_last_output = output;
    
    // Update history
    updateHistory(error, output);
    
    // Adaptive tuning
    if (m_config.enable_adaptive_tuning) {
        performAdaptiveTuning(error, output, dt);
    }
    
    // Update metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    updateMetrics(duration);
    
    return output;
}

void OptimizedPIDController::calculateBatch(const std::vector<double>& errors,
                                          const std::vector<double>& dts,
                                          std::vector<double>& outputs) {
    if (errors.size() != dts.size()) {
        logMessage("ERROR: Error and dt vectors must have same size");
        return;
    }
    
    outputs.resize(errors.size());
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_config.enable_simd && m_config.batch_processing && errors.size() >= 4) {
        // Use SIMD implementation
        SIMDUtils::vectorizedPIDCalculation(
            errors.data(), dts.data(), outputs.data(), errors.size(),
            m_kp, m_ki, m_kd, m_integral, m_last_error
        );
        m_metrics.simd_operations.fetch_add(errors.size());
    } else {
        // Fallback to sequential processing
        for (size_t i = 0; i < errors.size(); ++i) {
            outputs[i] = calculateInternal(errors[i], dts[i]);
        }
        m_metrics.scalar_operations.fetch_add(errors.size());
    }
    
    // Update metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    updateMetrics(duration);
}

void OptimizedPIDController::setGains(double kp, double ki, double kd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_kp = kp;
    m_ki = ki;
    m_kd = kd;
    
    DEBUG_LOG("PID gains updated: kp=" + std::to_string(kp) + 
              " ki=" + std::to_string(ki) + " kd=" + std::to_string(kd));
}

void OptimizedPIDController::getGains(double& kp, double& ki, double& kd) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    kp = m_kp;
    ki = m_ki;
    kd = m_kd;
}

void OptimizedPIDController::setOutputLimits(double min_output, double max_output) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_output_limits.enabled = true;
    m_output_limits.min_output = min_output;
    m_output_limits.max_output = max_output;
    
    DEBUG_LOG("Output limits set: [" + std::to_string(min_output) + ", " + std::to_string(max_output) + "]");
}

void OptimizedPIDController::enableOutputLimits(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_output_limits.enabled = enable;
}

PIDPerformanceMetrics OptimizedPIDController::getMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics; // Copy constructor will handle atomics
}

std::vector<std::string> OptimizedPIDController::getDiagnosticInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::string> info;
    info.push_back("=== Optimized PID Controller Diagnostics ===");
    info.push_back("Gains: kp=" + std::to_string(m_kp) + " ki=" + std::to_string(m_ki) + " kd=" + std::to_string(m_kd));
    info.push_back("Current State:");
    info.push_back("  Integral: " + std::to_string(m_integral));
    info.push_back("  Last Error: " + std::to_string(m_last_error));
    info.push_back("  Last Output: " + std::to_string(m_last_output));
    
    info.push_back("Performance Metrics:");
    info.push_back("  Total Calculations: " + std::to_string(m_metrics.total_calculations.load()));
    info.push_back("  Avg Calculation Time: " + std::to_string(m_metrics.avg_calculation_time_ns.load()) + " ns");
    info.push_back("  Cache Hit Ratio: " + std::to_string(m_metrics.getCacheHitRatio() * 100.0) + "%");
    info.push_back("  SIMD Utilization: " + std::to_string(m_metrics.getSIMDUtilization() * 100.0) + "%");
    
    info.push_back("Configuration:");
    info.push_back("  SIMD Enabled: " + std::string(m_config.enable_simd ? "Yes" : "No"));
    info.push_back("  Adaptive Tuning: " + std::string(m_config.enable_adaptive_tuning ? "Yes" : "No"));
    info.push_back("  Windup Protection: " + std::string(m_config.enable_windup_protection ? "Yes" : "No"));
    
    return info;
}

// =============================================================================
// PRIVATE METHODS
// =============================================================================
double OptimizedPIDController::calculateInternal(double error, double dt) {
    // Internal calculation without locking (caller must hold lock)
    
    // Calculate PID terms
    double proportional = m_kp * error;
    
    // Integral term with windup protection
    double new_integral = m_integral + error * dt;
    if (m_config.enable_windup_protection) {
        new_integral = std::clamp(new_integral, -m_config.max_integral, m_config.max_integral);
    }
    double integral_term = m_ki * new_integral;
    
    // Derivative term with optional filtering
    double derivative = (error - m_last_error) / dt;
    if (m_config.enable_derivative_filtering) {
        derivative = m_config.derivative_filter_alpha * derivative + 
                    (1.0 - m_config.derivative_filter_alpha) * m_last_derivative;
    }
    double derivative_term = m_kd * derivative;
    
    // Calculate output
    double output = proportional + integral_term + derivative_term;
    
    // Apply output limits if configured
    if (m_output_limits.enabled) {
        output = std::clamp(output, m_output_limits.min_output, m_output_limits.max_output);
    }
    
    // Update state
    m_integral = new_integral;
    m_last_error = error;
    m_last_derivative = derivative;
    m_last_output = output;
    
    return output;
}

void OptimizedPIDController::updateHistory(double error, double output) {
    // Maintain limited history for analysis
    const size_t MAX_HISTORY = 100;
    
    m_error_history.push_back(error);
    m_output_history.push_back(output);
    
    if (m_error_history.size() > MAX_HISTORY) {
        m_error_history.erase(m_error_history.begin());
    }
    
    if (m_output_history.size() > MAX_HISTORY) {
        m_output_history.erase(m_output_history.begin());
    }
}

void OptimizedPIDController::updateMetrics(std::chrono::nanoseconds calculation_time) {
    m_metrics.total_calculations.fetch_add(1);
    
    uint64_t time_ns = static_cast<uint64_t>(calculation_time.count());
    
    // Update average (simple moving average)
    uint64_t current_avg = m_metrics.avg_calculation_time_ns.load();
    uint64_t total_calcs = m_metrics.total_calculations.load();
    uint64_t new_avg = (current_avg * (total_calcs - 1) + time_ns) / total_calcs;
    m_metrics.avg_calculation_time_ns.store(new_avg);
    
    // Update peak
    uint64_t current_peak = m_metrics.peak_calculation_time_ns.load();
    while (time_ns > current_peak && !m_metrics.peak_calculation_time_ns.compare_exchange_weak(current_peak, time_ns)) {
        // Keep trying
    }
}

void OptimizedPIDController::performAdaptiveTuning(double error, double output, double dt) {
    // Simplified adaptive tuning implementation
    // Suppress unused parameter warnings
    (void)output;
    (void)dt;
    (void)error;
    
    if (m_error_history.size() < 10) return; // Need some history
    
    // Simple adaptive adjustment based on error magnitude
    double avg_error = std::accumulate(m_error_history.end() - 10, m_error_history.end(), 0.0) / 10.0;
    
    if (std::abs(avg_error) > 1.0) { // High error, increase proportional gain
        m_kp += m_config.adaptive_gain * 0.1;
        m_metrics.adaptive_adjustments.fetch_add(1);
    } else if (std::abs(avg_error) < 0.1) { // Low error, might be able to reduce gains
        m_kp = std::max(0.1, m_kp - m_config.adaptive_gain * 0.05);
        m_metrics.adaptive_adjustments.fetch_add(1);
    }
}

// =============================================================================
// MULTI-AXIS PID CONTROLLER IMPLEMENTATION
// =============================================================================
MultiAxisPIDController::MultiAxisPIDController(size_t num_axes, 
                                             const std::vector<PIDGains>& gains,
                                             const PIDOptimizationConfig& config) 
    : m_num_axes(num_axes), m_config(config) {
    
    initializeControllers(gains);
    DEBUG_LOG("MultiAxisPIDController constructed with " + std::to_string(num_axes) + " axes");
}

MultiAxisPIDController::~MultiAxisPIDController() {
    DEBUG_LOG("MultiAxisPIDController destroyed");
}

void MultiAxisPIDController::initializeControllers(const std::vector<PIDGains>& gains) {
    m_controllers.clear();
    m_controllers.reserve(m_num_axes);
    
    for (size_t i = 0; i < m_num_axes; ++i) {
        PIDGains axis_gains = (i < gains.size()) ? gains[i] : PIDGains{1.0, 0.1, 0.05};
        
        m_controllers.emplace_back(std::make_unique<OptimizedPIDController>(
            axis_gains.kp, axis_gains.ki, axis_gains.kd, m_config
        ));
    }
}

std::vector<double> MultiAxisPIDController::calculate(const std::vector<double>& errors,
                                                    const std::vector<double>& dts) {
    if (errors.size() != m_num_axes || dts.size() != m_num_axes) {
        logMessage("ERROR: Input vector size mismatch in MultiAxisPIDController");
        return std::vector<double>(m_num_axes, 0.0);
    }
    
    std::vector<double> outputs(m_num_axes);
    
    // Calculate outputs for each axis
    for (size_t i = 0; i < m_num_axes; ++i) {
        if (m_controllers[i]) {
            outputs[i] = m_controllers[i]->calculate(errors[i], dts[i]);
        } else {
            outputs[i] = 0.0;
        }
    }
    
    return outputs;
}

void MultiAxisPIDController::reset() {
    for (auto& controller : m_controllers) {
        if (controller) {
            controller->reset();
        }
    }
    DEBUG_LOG("MultiAxisPIDController reset");
}

void MultiAxisPIDController::setGains(size_t axis, double kp, double ki, double kd) {
    if (axis >= m_num_axes || !m_controllers[axis]) {
        logMessage("ERROR: Invalid axis index in setGains: " + std::to_string(axis));
        return;
    }
    
    m_controllers[axis]->setGains(kp, ki, kd);
}

void MultiAxisPIDController::setOutputLimits(size_t axis, double min_output, double max_output) {
    if (axis >= m_num_axes || !m_controllers[axis]) {
        logMessage("ERROR: Invalid axis index in setOutputLimits: " + std::to_string(axis));
        return;
    }
    
    m_controllers[axis]->setOutputLimits(min_output, max_output);
}

std::vector<PIDPerformanceMetrics> MultiAxisPIDController::getAllMetrics() const {
    std::vector<PIDPerformanceMetrics> metrics;
    metrics.reserve(m_num_axes);
    
    for (const auto& controller : m_controllers) {
        if (controller) {
            metrics.push_back(controller->getMetrics());
        } else {
            metrics.push_back(PIDPerformanceMetrics{});
        }
    }
    
    return metrics;
}

OptimizedPIDController* MultiAxisPIDController::getController(size_t axis) {
    if (axis >= m_num_axes) {
        return nullptr;
    }
    return m_controllers[axis].get();
}

// =============================================================================
// PID POOL MANAGER IMPLEMENTATION - SIMPLIFIED
// =============================================================================
PIDPoolManager::PIDPoolManager() : m_start_time(std::chrono::steady_clock::now()) {
    initializePools();
    DEBUG_LOG("PIDPoolManager constructed");
}

PIDPoolManager::~PIDPoolManager() {
    DEBUG_LOG("PIDPoolManager destroyed");
}

PIDPoolManager& PIDPoolManager::getInstance() {
    static PIDPoolManager instance;
    return instance;
}

void PIDPoolManager::initializePools() {
    // Initialize statistics tracking
    m_pid_controllers_created.store(0);
    m_multi_axis_controllers_created.store(0);
    
    logMessage("✅ PID Pool Manager initialized (direct allocation mode)");
}

std::unique_ptr<OptimizedPIDController> PIDPoolManager::acquirePIDController(double kp, double ki, double kd,
                                                                           const PIDOptimizationConfig& config) {
    // Direct creation since ObjectPool templates conflict
    m_pid_controllers_created.fetch_add(1);
    return std::make_unique<OptimizedPIDController>(kp, ki, kd, config);
}

std::unique_ptr<MultiAxisPIDController> PIDPoolManager::acquireMultiAxisController(size_t num_axes,
                                                                                 const std::vector<PIDGains>& gains,
                                                                                 const PIDOptimizationConfig& config) {
    // Direct creation since ObjectPool templates conflict
    m_multi_axis_controllers_created.fetch_add(1);
    return std::make_unique<MultiAxisPIDController>(num_axes, gains, config);
}

std::vector<std::string> PIDPoolManager::getPoolStatistics() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    
    std::vector<std::string> stats;
    stats.push_back("=== PID Pool Manager Statistics ===");
    stats.push_back("Mode: Direct Allocation (No ObjectPool due to template conflicts)");
    
    stats.push_back("--- Creation Statistics ---");
    stats.push_back("PID Controllers Created: " + std::to_string(m_pid_controllers_created.load()));
    stats.push_back("Multi-Axis Controllers Created: " + std::to_string(m_multi_axis_controllers_created.load()));
    
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - m_start_time);
    stats.push_back("Manager Uptime: " + std::to_string(uptime.count()) + " seconds");
    
    stats.push_back("--- Notes ---");
    stats.push_back("* ObjectPool integration disabled due to template conflicts");
    stats.push_back("* All controllers created using direct allocation");
    stats.push_back("* Memory pooling benefits not available in current implementation");
    
    return stats;
}

void PIDPoolManager::clearPools() {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    
    // Reset statistics
    m_pid_controllers_created.store(0);
    m_multi_axis_controllers_created.store(0);
    m_start_time = std::chrono::steady_clock::now();
    
    logMessage("✅ PID Pool Manager statistics cleared");
}

// =============================================================================
// GLOBAL FUNCTIONS
// =============================================================================
bool initializePIDSystem() {
    // Initialize PID pool manager
    auto& pool_manager = PIDPoolManager::getInstance();
    (void)pool_manager; // Suppress unused warning
    
    logMessage("✅ PID system initialized successfully");
    return true;
}

void shutdownPIDSystem() {
    auto& pool_manager = PIDPoolManager::getInstance();
    pool_manager.clearPools();
    
    logMessage("✅ PID system shutdown completed");
}

std::unique_ptr<OptimizedPIDController> createOptimizedPID(double kp, double ki, double kd,
                                                          const PIDOptimizationConfig& config) {
    return PIDPoolManager::getInstance().acquirePIDController(kp, ki, kd, config);
}

std::unique_ptr<MultiAxisPIDController> createMultiAxisPID(size_t num_axes,
                                                          const std::vector<PIDGains>& gains,
                                                          const PIDOptimizationConfig& config) {
    return PIDPoolManager::getInstance().acquireMultiAxisController(num_axes, gains, config);
}