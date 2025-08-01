// memory_pool.h - CORRECTED VERSION v3.0.5
// --------------------------------------------------------------------------------------
// description: Memory pool management with thread-safe operations
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 3.0.5 - Fixed atomic and incomplete type issues
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
#include <shared_mutex>
#include <unordered_map>
#include <functional>
#include <stack>
#include <typeinfo>
#include <cstddef>
#include <thread>
#include <algorithm>
#include <cctype>
#include "common_defines.h"
#include "globals.h"

// =============================================================================
// CONCRETE TYPE DEFINITIONS (NOT FORWARD DECLARATIONS)
// =============================================================================
struct InputEvent {
    int type = 0;
    int x = 0;
    int y = 0;
    uint64_t timestamp = 0;
    
    InputEvent() = default;
    InputEvent(int t, int x_pos, int y_pos) : type(t), x(x_pos), y(y_pos) {}
};

struct PIDState {
    double kp = 0.8;
    double ki = 0.3;
    double kd = 0.25;
    double max_output = 50.0;
    double integral_limit = 100.0;
    double derivative_limit = 25.0;
    
    PIDState() = default;
    PIDState(double p, double i, double d, double max_out = 50.0) 
        : kp(p), ki(i), kd(d), max_output(max_out) {}
};

// MovementCommand is defined in globals.h - removed duplicate definition

// =============================================================================
// POOL CONFIGURATION
// =============================================================================
struct PoolConfig {
    size_t initial_size = 16;
    size_t max_size = 1024;
    size_t growth_factor = 2;
    bool enable_statistics = true;
    bool thread_safe = true;
    
    PoolConfig() = default;
    PoolConfig(size_t init, size_t max, size_t growth = 2) 
        : initial_size(init), max_size(max), growth_factor(growth) {}
};

// Adaptive pool configuration based on usage patterns
struct AdaptivePoolConfig {
    size_t min_size = 10;
    size_t max_size = 1000;
    size_t growth_factor = 2;
    size_t shrink_threshold = 0.1; // 10% usage triggers shrink
    size_t expand_threshold = 0.8; // 80% usage triggers expand
    size_t max_fragmentation = 0.3; // 30% fragmentation threshold
    
    // Usage tracking
    size_t total_requests = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t peak_usage = 0;
    size_t current_usage = 0;
    
    // Performance metrics
    double average_allocation_time_ns = 0.0;
    double average_deallocation_time_ns = 0.0;
    double fragmentation_ratio = 0.0;
    
    void updateMetrics(size_t requests, size_t hits, size_t misses, size_t usage) {
        total_requests += requests;
        cache_hits += hits;
        cache_misses += misses;
        current_usage = usage;
        peak_usage = std::max(peak_usage, usage);
        
        // Calculate fragmentation
        if (current_usage > 0) {
            fragmentation_ratio = static_cast<double>(cache_misses) / total_requests;
        }
    }
    
    double getHitRatio() const {
        return total_requests > 0 ? static_cast<double>(cache_hits) / total_requests : 0.0;
    }
    
    bool shouldExpand() const {
        return getHitRatio() < expand_threshold && current_usage > 0;
    }
    
    bool shouldShrink() const {
        return getHitRatio() > (1.0 - shrink_threshold) && current_usage < peak_usage * 0.5;
    }
    
    size_t calculateOptimalSize() const {
        if (shouldExpand()) {
            return std::min(max_size, current_usage * growth_factor);
        } else if (shouldShrink()) {
            return std::max(min_size, current_usage / growth_factor);
        }
        return current_usage;
    }
};

// =============================================================================
// POOL STATISTICS - FIXED ATOMIC COPY/MOVE ISSUES
// =============================================================================
struct PoolStatistics {
    std::atomic<size_t> pool_hits{0};
    std::atomic<size_t> pool_misses{0};
    std::atomic<size_t> objects_in_use{0};
    std::atomic<size_t> current_pool_size{0};
    std::atomic<size_t> peak_usage{0};
    std::atomic<size_t> total_allocations{0};
    
    // Default constructor
    PoolStatistics() = default;
    
    // Copy constructor - manual implementation for atomics
    PoolStatistics(const PoolStatistics& other) {
        pool_hits.store(other.pool_hits.load());
        pool_misses.store(other.pool_misses.load());
        objects_in_use.store(other.objects_in_use.load());
        current_pool_size.store(other.current_pool_size.load());
        peak_usage.store(other.peak_usage.load());
        total_allocations.store(other.total_allocations.load());
    }
    
    // Move constructor
    PoolStatistics(PoolStatistics&& other) noexcept {
        pool_hits.store(other.pool_hits.load());
        pool_misses.store(other.pool_misses.load());
        objects_in_use.store(other.objects_in_use.load());
        current_pool_size.store(other.current_pool_size.load());
        peak_usage.store(other.peak_usage.load());
        total_allocations.store(other.total_allocations.load());
    }
    
    // Copy assignment
    PoolStatistics& operator=(const PoolStatistics& other) {
        if (this != &other) {
            pool_hits.store(other.pool_hits.load());
            pool_misses.store(other.pool_misses.load());
            objects_in_use.store(other.objects_in_use.load());
            current_pool_size.store(other.current_pool_size.load());
            peak_usage.store(other.peak_usage.load());
            total_allocations.store(other.total_allocations.load());
        }
        return *this;
    }
    
    // Move assignment
    PoolStatistics& operator=(PoolStatistics&& other) noexcept {
        if (this != &other) {
            pool_hits.store(other.pool_hits.load());
            pool_misses.store(other.pool_misses.load());
            objects_in_use.store(other.objects_in_use.load());
            current_pool_size.store(other.current_pool_size.load());
            peak_usage.store(other.peak_usage.load());
            total_allocations.store(other.total_allocations.load());
        }
        return *this;
    }
    
    double getCacheHitRatio() const { 
        size_t hits = pool_hits.load();
        size_t misses = pool_misses.load();
        size_t total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
    
    double getMemoryEfficiency() const { 
        size_t in_use = objects_in_use.load();
        size_t pool_size = current_pool_size.load();
        return pool_size > 0 ? static_cast<double>(in_use) / pool_size : 0.0;
    }
    
    void reset() {
        pool_hits.store(0);
        pool_misses.store(0);
        objects_in_use.store(0);
        current_pool_size.store(0);
        peak_usage.store(0);
        total_allocations.store(0);
    }
};

// =============================================================================
// OBJECT POOL TEMPLATE
// =============================================================================
template<typename T>
class ObjectPool {
public:
    using DeleterType = std::function<void(T*)>;
    using UniquePtr = std::unique_ptr<T, DeleterType>;
    
    explicit ObjectPool(const PoolConfig& config = PoolConfig{}) : m_config(config) {
        reserve(m_config.initial_size);
    }
    
    ~ObjectPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_available_objects.empty()) {
            m_available_objects.pop();
        }
    }
    
    template<typename... Args>
    UniquePtr acquire(Args&&... args) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Try lock-free acquisition first
        std::unique_ptr<T> obj = tryAcquireLockFree();
        if (obj) {
            m_stats.pool_hits.fetch_add(1);
            m_stats.objects_in_use.fetch_add(1);
            updateAdaptiveMetrics(true, start_time);
            
            auto deleter = [this](T* ptr) {
                this->deallocateToPool(ptr);
            };
            
            return UniquePtr(obj.release(), deleter);
        }
        
        // Fall back to locked acquisition
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Create deleter that returns object to pool
        auto deleter = [this](T* ptr) {
            this->deallocateToPool(ptr);
        };
        
        if (!m_available_objects.empty()) {
            auto obj = std::move(m_available_objects.top());
            m_available_objects.pop();
            m_stats.pool_hits.fetch_add(1);
            m_stats.objects_in_use.fetch_add(1);
            updateAdaptiveMetrics(true, start_time);
            
            return UniquePtr(obj.release(), deleter);
        } else {
            m_stats.pool_misses.fetch_add(1);
            m_stats.objects_in_use.fetch_add(1);
            updateAdaptiveMetrics(false, start_time);
            
            return UniquePtr(new T(std::forward<Args>(args)...), deleter);
        }
    }
    
    void reserve(size_t count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (m_available_objects.size() < count) {
            m_available_objects.push(std::make_unique<T>());
        }
        m_stats.current_pool_size.store(m_available_objects.size());
    }
    
    void expandPool(size_t additional_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < additional_count; ++i) {
            auto obj = std::make_unique<T>();
            if (obj) {
                m_available_objects.push(std::move(obj));
            } else {
                break;
            }
        }
        m_stats.current_pool_size.store(m_available_objects.size());
    }
    
    // Adaptive tuning methods
    void performAdaptiveTuning() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t current_size = m_available_objects.size();
        size_t optimal_size = m_adaptive_config.calculateOptimalSize();
        
        if (optimal_size > current_size) {
            // Expand pool
            size_t expand_by = optimal_size - current_size;
            expandPool(expand_by);
            logMessage("Pool expanded by " + std::to_string(expand_by) + " objects");
        } else if (optimal_size < current_size && current_size > m_config.initial_size) {
            // Shrink pool
            size_t shrink_by = current_size - optimal_size;
            for (size_t i = 0; i < shrink_by && !m_available_objects.empty(); ++i) {
                m_available_objects.pop();
            }
            m_stats.current_pool_size.store(m_available_objects.size());
            logMessage("Pool shrunk by " + std::to_string(shrink_by) + " objects");
        }
    }
    
    // Return statistics by value to avoid copy/move issues
    PoolStatistics getStatistics() const {
        PoolStatistics result;
        result.pool_hits.store(m_stats.pool_hits.load());
        result.pool_misses.store(m_stats.pool_misses.load());
        result.objects_in_use.store(m_stats.objects_in_use.load());
        result.current_pool_size.store(m_stats.current_pool_size.load());
        result.peak_usage.store(m_stats.peak_usage.load());
        result.total_allocations.store(m_stats.total_allocations.load());
        return result;
    }
    
    AdaptivePoolConfig getAdaptiveConfig() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_adaptive_config;
    }
    
    void clearPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_available_objects.empty()) {
            m_available_objects.pop();
        }
        m_stats.current_pool_size.store(0);
    }
    
private:
    // Lock-free acquisition attempt
    std::unique_ptr<T> tryAcquireLockFree() {
        // This is a simplified lock-free attempt
        // In a real implementation, you'd use atomic operations
        return nullptr; // For now, always fall back to locked version
    }
    
    void updateAdaptiveMetrics(bool hit, std::chrono::high_resolution_clock::time_point start_time) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        m_adaptive_config.total_requests++;
        if (hit) {
            m_adaptive_config.cache_hits++;
        } else {
            m_adaptive_config.cache_misses++;
        }
        
        m_adaptive_config.current_usage = m_stats.objects_in_use.load();
        m_adaptive_config.average_allocation_time_ns = 
            (m_adaptive_config.average_allocation_time_ns + duration.count()) / 2.0;
    }
    
    void deallocateToPool(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_available_objects.size() < m_config.max_size) {
            // Reset object to default state
            *obj = T{};
            m_available_objects.push(std::unique_ptr<T>(obj));
        } else {
            delete obj;
        }
        m_stats.objects_in_use.fetch_sub(1);
    }
    
    PoolConfig m_config;
    AdaptivePoolConfig m_adaptive_config;
    mutable std::mutex m_mutex;
    mutable PoolStatistics m_stats;
    std::stack<std::unique_ptr<T>> m_available_objects;
};

// =============================================================================
// POOL ALLOCATOR TEMPLATE
// =============================================================================
template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };
    
    PoolAllocator() = default;
    
    template<typename U>
    PoolAllocator(const PoolAllocator<U>&) noexcept {}
    
    pointer allocate(size_type n) {
        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }
    
    void deallocate(pointer p, size_type n) {
        (void)n; // Suppress unused parameter warning
        ::operator delete(p);
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    bool operator==(const PoolAllocator&) const noexcept { return true; }
    bool operator!=(const PoolAllocator&) const noexcept { return false; }
};

// =============================================================================
// POOL MANAGER
// =============================================================================
class PoolManager {
public:
    static PoolManager& getInstance() {
        static PoolManager instance;
        return instance;
    }
    
    template<typename T>
    ObjectPool<T>* getPool(const std::string& name = "") {
        std::shared_lock<std::shared_mutex> lock(m_pools_mutex);
        
        std::string pool_name = name.empty() ? generatePoolName(typeid(T)) : name;
        
        auto it = m_pools.find(pool_name);
        if (it != m_pools.end()) {
            return static_cast<ObjectPool<T>*>(it->second.get());
        }
        
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(m_pools_mutex);
        
        it = m_pools.find(pool_name);
        if (it != m_pools.end()) {
            return static_cast<ObjectPool<T>*>(it->second.get());
        }
        
        auto pool = std::make_unique<ObjectPool<T>>(m_global_config);
        ObjectPool<T>* pool_ptr = pool.get();
        
        m_pools[pool_name] = std::unique_ptr<void, std::function<void(void*)>>(
            pool.release(),
            [](void* ptr) {
                delete static_cast<ObjectPool<T>*>(ptr);
            }
        );
        
        return pool_ptr;
    }
    
    template<typename T>
    void registerPool(const std::string& name, const PoolConfig& config) {
        std::unique_lock<std::shared_mutex> lock(m_pools_mutex);
        
        std::string pool_name = name.empty() ? generatePoolName(typeid(T)) : name;
        
        if (m_pools.find(pool_name) != m_pools.end()) {
            return;
        }
        
        auto pool = std::make_unique<ObjectPool<T>>(config);
        
        m_pools[pool_name] = std::unique_ptr<void, std::function<void(void*)>>(
            pool.release(),
            [](void* ptr) {
                delete static_cast<ObjectPool<T>*>(ptr);
            }
        );
    }
    
    void unregisterPool(const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(m_pools_mutex);
        auto it = m_pools.find(name);
        if (it != m_pools.end()) {
            m_pools.erase(it);
        }
    }
    
    void performGlobalMaintenance() {
        std::shared_lock<std::shared_mutex> lock(m_pools_mutex);
        // Maintenance implementation
    }
    
    void clearAllPools() {
        std::unique_lock<std::shared_mutex> lock(m_pools_mutex);
        m_pools.clear();
    }
    
    // Return statistics summary as vector of strings
    std::vector<std::string> getGlobalStatistics() const {
        std::shared_lock<std::shared_mutex> lock(m_pools_mutex);
        
        std::vector<std::string> stats;
        stats.push_back("=== Pool Manager Statistics ===");
        stats.push_back("Total Pools: " + std::to_string(m_pools.size()));
        
        size_t total_objects = 0;
        size_t total_pool_size = 0;
        
        for (const auto& [name, pool_ptr] : m_pools) {
            stats.push_back("Pool: " + name);
            // Individual pool stats would require type information
            (void)pool_ptr; // Suppress unused warning
        }
        
        stats.push_back("Total Objects in Use: " + std::to_string(total_objects));
        stats.push_back("Total Pool Size: " + std::to_string(total_pool_size));
        
        return stats;
    }
    
    void setGlobalConfig(const PoolConfig& config) {
        m_global_config = config;
    }
    
    void enableGlobalThreadSafety(bool enable) {
        (void)enable;
    }
    
private:
    PoolManager() = default;
    ~PoolManager() = default;
    PoolManager(const PoolManager&) = delete;
    PoolManager& operator=(const PoolManager&) = delete;
    
    std::string generatePoolName(const std::type_info& type_info) {
        return std::string("Pool_") + type_info.name();
    }
    
    mutable std::shared_mutex m_pools_mutex;
    std::unordered_map<std::string, std::unique_ptr<void, std::function<void(void*)>>> m_pools;
    PoolConfig m_global_config;
};

// =============================================================================
// HELPER FUNCTIONS - DECLARATIONS ONLY
// =============================================================================
namespace PoolHelpers {
    struct MemoryReport {
        size_t total_pools = 0;
        size_t total_objects_in_use = 0;
        size_t total_pool_size = 0;
        double average_efficiency = 0.0;
        std::vector<std::string> detailed_stats;
    };
    
    MemoryReport generateMemoryReport();
    
    // Function declarations - implementations in .cpp
    std::unique_ptr<InputEvent, std::function<void(InputEvent*)>> acquireInputEvent();
    std::unique_ptr<PIDState, std::function<void(PIDState*)>> acquirePIDState();
    std::unique_ptr<MovementCommand, std::function<void(MovementCommand*)>> acquireMovementCommand();
    std::unique_ptr<std::vector<float>, std::function<void(std::vector<float>*)>> acquireFloatVector();
    std::unique_ptr<std::string, std::function<void(std::string*)>> acquireString();
    std::vector<std::string> getPoolStatistics();
}

// =============================================================================
// EXPLICIT TEMPLATE INSTANTIATIONS
// =============================================================================
extern template class ObjectPool<InputEvent>;
extern template class ObjectPool<PIDState>;
extern template class ObjectPool<MovementCommand>;
extern template class ObjectPool<std::vector<float>>;
extern template class ObjectPool<std::string>;

// =============================================================================
// GLOBAL CONVENIENCE MACROS
// =============================================================================
#define ACQUIRE_INPUT_EVENT() PoolHelpers::acquireInputEvent()
#define ACQUIRE_PID_STATE() PoolHelpers::acquirePIDState()
#define ACQUIRE_MOVEMENT_COMMAND() PoolHelpers::acquireMovementCommand()
#define ACQUIRE_FLOAT_VECTOR() PoolHelpers::acquireFloatVector()
#define ACQUIRE_STRING() PoolHelpers::acquireString()

#define POOL_MANAGER() PoolManager::getInstance()
#define GET_POOL_STATS() PoolHelpers::getPoolStatistics()