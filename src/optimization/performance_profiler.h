// performance_profiler.h - CORRECTED VERSION v3.0.5
// description: Performance profiling system with metrics collection
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.5 - Fixed singleton constructor access
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include <memory>
#include "common_defines.h"

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
class PerformanceProfiler;

// =============================================================================
// METRIC STRUCTURES - FIXED ATOMIC COPY/MOVE ISSUES
// =============================================================================
struct TimingMetrics {
    std::atomic<uint64_t> total_calls{0};
    std::atomic<uint64_t> total_time_ns{0};
    std::atomic<uint64_t> min_time_ns{UINT64_MAX};
    std::atomic<uint64_t> max_time_ns{0};
    std::atomic<uint64_t> last_call_time_ns{0};
    
    // Default constructor
    TimingMetrics() = default;
    
    // Copy constructor
    TimingMetrics(const TimingMetrics& other) {
        total_calls.store(other.total_calls.load());
        total_time_ns.store(other.total_time_ns.load());
        min_time_ns.store(other.min_time_ns.load());
        max_time_ns.store(other.max_time_ns.load());
        last_call_time_ns.store(other.last_call_time_ns.load());
    }
    
    // Move constructor
    TimingMetrics(TimingMetrics&& other) noexcept {
        total_calls.store(other.total_calls.load());
        total_time_ns.store(other.total_time_ns.load());
        min_time_ns.store(other.min_time_ns.load());
        max_time_ns.store(other.max_time_ns.load());
        last_call_time_ns.store(other.last_call_time_ns.load());
    }
    
    // Copy assignment
    TimingMetrics& operator=(const TimingMetrics& other) {
        if (this != &other) {
            total_calls.store(other.total_calls.load());
            total_time_ns.store(other.total_time_ns.load());
            min_time_ns.store(other.min_time_ns.load());
            max_time_ns.store(other.max_time_ns.load());
            last_call_time_ns.store(other.last_call_time_ns.load());
        }
        return *this;
    }
    
    // Move assignment
    TimingMetrics& operator=(TimingMetrics&& other) noexcept {
        if (this != &other) {
            total_calls.store(other.total_calls.load());
            total_time_ns.store(other.total_time_ns.load());
            min_time_ns.store(other.min_time_ns.load());
            max_time_ns.store(other.max_time_ns.load());
            last_call_time_ns.store(other.last_call_time_ns.load());
        }
        return *this;
    }
    
    double getAverageTimeMs() const {
        uint64_t calls = total_calls.load();
        return calls > 0 ? static_cast<double>(total_time_ns.load()) / (calls * 1000000.0) : 0.0;
    }
    
    void reset() {
        total_calls.store(0);
        total_time_ns.store(0);
        min_time_ns.store(UINT64_MAX);
        max_time_ns.store(0);
        last_call_time_ns.store(0);
    }
};

struct MemoryMetrics {
    std::atomic<size_t> current_usage{0};
    std::atomic<size_t> peak_usage{0};
    std::atomic<size_t> total_allocations{0};
    std::atomic<size_t> total_deallocations{0};
    std::atomic<size_t> allocation_count{0};
    
    // Default constructor
    MemoryMetrics() = default;
    
    // Copy constructor
    MemoryMetrics(const MemoryMetrics& other) {
        current_usage.store(other.current_usage.load());
        peak_usage.store(other.peak_usage.load());
        total_allocations.store(other.total_allocations.load());
        total_deallocations.store(other.total_deallocations.load());
        allocation_count.store(other.allocation_count.load());
    }
    
    // Move constructor
    MemoryMetrics(MemoryMetrics&& other) noexcept {
        current_usage.store(other.current_usage.load());
        peak_usage.store(other.peak_usage.load());
        total_allocations.store(other.total_allocations.load());
        total_deallocations.store(other.total_deallocations.load());
        allocation_count.store(other.allocation_count.load());
    }
    
    // Copy assignment
    MemoryMetrics& operator=(const MemoryMetrics& other) {
        if (this != &other) {
            current_usage.store(other.current_usage.load());
            peak_usage.store(other.peak_usage.load());
            total_allocations.store(other.total_allocations.load());
            total_deallocations.store(other.total_deallocations.load());
            allocation_count.store(other.allocation_count.load());
        }
        return *this;
    }
    
    // Move assignment
    MemoryMetrics& operator=(MemoryMetrics&& other) noexcept {
        if (this != &other) {
            current_usage.store(other.current_usage.load());
            peak_usage.store(other.peak_usage.load());
            total_allocations.store(other.total_allocations.load());
            total_deallocations.store(other.total_deallocations.load());
            allocation_count.store(other.allocation_count.load());
        }
        return *this;
    }
    
    void reset() {
        current_usage.store(0);
        peak_usage.store(0);
        total_allocations.store(0);
        total_deallocations.store(0);
        allocation_count.store(0);
    }
};

struct CPUMetrics {
    std::atomic<double> usage_percent{0.0};
    std::atomic<uint64_t> total_samples{0};
    std::atomic<uint64_t> high_usage_samples{0};
    std::atomic<uint64_t> last_update_time{0};
    
    // Default constructor
    CPUMetrics() = default;
    
    // Manual copy constructor for atomics
    CPUMetrics(const CPUMetrics& other) {
        usage_percent.store(other.usage_percent.load());
        total_samples.store(other.total_samples.load());
        high_usage_samples.store(other.high_usage_samples.load());
        last_update_time.store(other.last_update_time.load());
    }
    
    // Manual move constructor
    CPUMetrics(CPUMetrics&& other) noexcept {
        usage_percent.store(other.usage_percent.load());
        total_samples.store(other.total_samples.load());
        high_usage_samples.store(other.high_usage_samples.load());
        last_update_time.store(other.last_update_time.load());
    }
    
    // Manual copy assignment
    CPUMetrics& operator=(const CPUMetrics& other) {
        if (this != &other) {
            usage_percent.store(other.usage_percent.load());
            total_samples.store(other.total_samples.load());
            high_usage_samples.store(other.high_usage_samples.load());
            last_update_time.store(other.last_update_time.load());
        }
        return *this;
    }
    
    // Manual move assignment
    CPUMetrics& operator=(CPUMetrics&& other) noexcept {
        if (this != &other) {
            usage_percent.store(other.usage_percent.load());
            total_samples.store(other.total_samples.load());
            high_usage_samples.store(other.high_usage_samples.load());
            last_update_time.store(other.last_update_time.load());
        }
        return *this;
    }
    
    void reset() {
        usage_percent.store(0.0);
        total_samples.store(0);
        high_usage_samples.store(0);
        last_update_time.store(0);
    }
};

struct FPSMetrics {
    std::atomic<uint64_t> frame_count{0};
    std::atomic<uint64_t> total_frame_time_ns{0};
    std::atomic<double> current_fps{0.0};
    std::atomic<double> average_fps{0.0};
    
    // Default constructor
    FPSMetrics() = default;
    
    // Manual copy constructor
    FPSMetrics(const FPSMetrics& other) {
        frame_count.store(other.frame_count.load());
        total_frame_time_ns.store(other.total_frame_time_ns.load());
        current_fps.store(other.current_fps.load());
        average_fps.store(other.average_fps.load());
    }
    
    // Manual move constructor
    FPSMetrics(FPSMetrics&& other) noexcept {
        frame_count.store(other.frame_count.load());
        total_frame_time_ns.store(other.total_frame_time_ns.load());
        current_fps.store(other.current_fps.load());
        average_fps.store(other.average_fps.load());
    }
    
    // Manual copy assignment
    FPSMetrics& operator=(const FPSMetrics& other) {
        if (this != &other) {
            frame_count.store(other.frame_count.load());
            total_frame_time_ns.store(other.total_frame_time_ns.load());
            current_fps.store(other.current_fps.load());
            average_fps.store(other.average_fps.load());
        }
        return *this;
    }
    
    // Manual move assignment
    FPSMetrics& operator=(FPSMetrics&& other) noexcept {
        if (this != &other) {
            frame_count.store(other.frame_count.load());
            total_frame_time_ns.store(other.total_frame_time_ns.load());
            current_fps.store(other.current_fps.load());
            average_fps.store(other.average_fps.load());
        }
        return *this;
    }
    
    void reset() {
        frame_count.store(0);
        total_frame_time_ns.store(0);
        current_fps.store(0.0);
        average_fps.store(0.0);
    }
};

// =============================================================================
// CUSTOM METRIC STRUCTURE
// =============================================================================
struct CustomMetric {
    std::string name;
    double value;
    std::string unit;
    std::chrono::steady_clock::time_point timestamp;
    
    CustomMetric() = default;
    CustomMetric(const std::string& n, double v, const std::string& u)
        : name(n), value(v), unit(u), timestamp(std::chrono::steady_clock::now()) {}
};

// =============================================================================
// PROFILER SCOPE HELPER
// =============================================================================
class ProfilerScope {
public:
    ProfilerScope(const std::string& name);
    ~ProfilerScope();
    
private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_start_time;
};

// =============================================================================
// MAIN PERFORMANCE PROFILER CLASS
// =============================================================================
class PerformanceProfiler {
public:
    // Singleton pattern
    static PerformanceProfiler& getInstance();
    
    // Core profiling functions
    void startTiming(const std::string& name, const std::string& category = "general");
    void endTiming(const std::string& name);
    
    // Memory tracking
    void recordAllocation(const std::string& component, size_t size);
    void recordDeallocation(const std::string& component, size_t size);
    void updateMemoryUsage(const std::string& component, size_t current_usage);
    
    // System metrics
    void updateCPUUsage(double percentage);
    void recordFrameTime(std::chrono::nanoseconds frame_time);
    void updateFPS(double current_fps, double average_fps);
    
    // Custom metrics
    void recordCustomMetric(const std::string& name, double value, const std::string& unit = "");
    void incrementCounter(const std::string& name, uint64_t increment = 1);
    
    // Data retrieval - Return by value to avoid copy issues
    TimingMetrics getTimingMetrics(const std::string& name) const;
    MemoryMetrics getMemoryMetrics(const std::string& component) const;
    CPUMetrics getCPUMetrics() const;
    FPSMetrics getFPSMetrics() const;
    
    // Bulk data retrieval
    std::vector<std::string> getAllTimingNames() const;
    std::vector<std::string> getAllMemoryComponents() const;
    std::vector<CustomMetric> getCustomMetrics() const;
    
    // Reporting
    std::string generateReport(bool detailed = false) const;
    std::string generateTimingReport() const;
    std::string generateMemoryReport() const;
    std::string generateSystemReport() const;
    
    // File export
    void exportCSV(const std::string& filename) const;
    void exportJSON(const std::string& filename) const;
    
    // Configuration
    void enableMonitoring(bool enable);
    void setReportInterval(std::chrono::milliseconds interval);
    void enableFileOutput(bool enable, const std::string& directory = "logs");
    
    // Maintenance
    void resetMetrics();
    void clearHistory();
    
    // Analysis
    std::vector<std::string> getPerformanceBottlenecks() const;
    std::vector<std::string> getOptimizationSuggestions() const;
    double getOverallPerformanceScore() const;
    
    // Thread management
    void startMonitoring();
    void stopMonitoring();
    
    // Destructor
    ~PerformanceProfiler();

private:
    // Private constructor for singleton
    PerformanceProfiler();
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;
    
    // Friend function to allow creation
    friend void initializeProfiler();
    
    // Internal data structures
    std::unordered_map<std::string, TimingMetrics> m_timing_metrics;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_active_timers;
    std::unordered_map<std::string, MemoryMetrics> m_memory_metrics;
    std::vector<CustomMetric> m_custom_metrics;
    std::unordered_map<std::string, uint64_t> m_counters;
    
    // System metrics
    CPUMetrics m_cpu_metrics;
    FPSMetrics m_fps_metrics;
    
    // Thread safety
    mutable std::mutex m_timing_mutex;
    mutable std::mutex m_memory_mutex;
    mutable std::mutex m_cpu_mutex;
    mutable std::mutex m_fps_mutex;
    mutable std::mutex m_custom_mutex;
    mutable std::mutex m_file_mutex;
    
    // Configuration
    std::atomic<bool> m_monitoring_enabled{false};
    std::atomic<bool> m_file_output_enabled{false};
    std::chrono::milliseconds m_report_interval{1000};
    std::string m_output_directory{"logs"};
    
    // Monitoring thread
    std::unique_ptr<std::thread> m_monitoring_thread;
    std::atomic<bool> m_should_stop{false};
    
    // Internal methods
    void monitoringLoop();
    void collectSystemMetrics();
    void writeToFile(const std::string& filename, const std::string& content) const;
    void updateTimingMetric(const std::string& name, std::chrono::nanoseconds duration, const std::string& category);
};

// =============================================================================
// GLOBAL PROFILER INSTANCE - SIMPLIFIED APPROACH
// =============================================================================
extern PerformanceProfiler* g_profiler_instance;

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================
#define PROFILER() PerformanceProfiler::getInstance()
#define START_TIMING(name) PROFILER().startTiming(name)
#define END_TIMING(name) PROFILER().endTiming(name)
#define RECORD_MEMORY(component, size) PROFILER().recordAllocation(component, size)
#define RECORD_CUSTOM(name, value, unit) PROFILER().recordCustomMetric(name, value, unit)

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================
void initializeProfiler();
void shutdownProfiler();
PerformanceProfiler* getProfiler();

// Implementation function for ProfileCustom to avoid macro conflicts
void ProfileCustomImpl(const std::string& name, double value, const std::string& unit);