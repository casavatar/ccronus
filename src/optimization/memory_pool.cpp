// memory_pool.cpp - CORRECTED VERSION v3.0.5
// --------------------------------------------------------------------------------------
// description: Implementation of memory pool management
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 3.0.5 - Fixed atomic and type issues
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "memory_pool.h"
#include "globals.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <cctype>

// =============================================================================
// EXPLICIT TEMPLATE INSTANTIATIONS
// =============================================================================
template class ObjectPool<InputEvent>;
template class ObjectPool<PIDState>;
template class ObjectPool<MovementCommand>;
template class ObjectPool<std::vector<float>>;
template class ObjectPool<std::string>;

// =============================================================================
// GLOBAL POOL INITIALIZER
// =============================================================================
class GlobalPoolInitializer {
public:
    GlobalPoolInitializer() {
        auto& manager = PoolManager::getInstance();
        
        PoolConfig fast_config;
        fast_config.initial_size = 32;
        fast_config.max_size = 256;
        fast_config.growth_factor = 2;
        
        PoolConfig large_config;
        large_config.initial_size = 8;
        large_config.max_size = 64;
        large_config.growth_factor = 2;
        
        manager.registerPool<InputEvent>("InputEvents", fast_config);
        manager.registerPool<PIDState>("PIDStates", fast_config);
        manager.registerPool<MovementCommand>("MovementCommands", fast_config);
        manager.registerPool<std::vector<float>>("FloatVectors", large_config);
        manager.registerPool<std::string>("Strings", fast_config);
        
        logMessage("✅ Memory pools initialized successfully");
    }
    
    ~GlobalPoolInitializer() {
        PoolManager::getInstance().clearAllPools();
        logMessage("✅ Memory pools cleaned up");
    }
};

static GlobalPoolInitializer g_pool_initializer;

// =============================================================================
// POOL HELPER IMPLEMENTATIONS
// =============================================================================
namespace PoolHelpers {
    MemoryReport generateMemoryReport() {
        MemoryReport report;
        auto& manager = PoolManager::getInstance();
        auto stats = manager.getGlobalStatistics();
        
        report.detailed_stats = stats;
        report.total_pools = 1; // Simplified
        
        return report;
    }
    
    std::unique_ptr<InputEvent, std::function<void(InputEvent*)>> acquireInputEvent() {
        return PoolManager::getInstance().getPool<InputEvent>()->acquire();
    }
    
    std::unique_ptr<PIDState, std::function<void(PIDState*)>> acquirePIDState() {
        return PoolManager::getInstance().getPool<PIDState>()->acquire();
    }
    
    std::unique_ptr<MovementCommand, std::function<void(MovementCommand*)>> acquireMovementCommand() {
        return PoolManager::getInstance().getPool<MovementCommand>()->acquire();
    }
    
    std::unique_ptr<std::vector<float>, std::function<void(std::vector<float>*)>> acquireFloatVector() {
        return PoolManager::getInstance().getPool<std::vector<float>>()->acquire();
    }
    
    std::unique_ptr<std::string, std::function<void(std::string*)>> acquireString() {
        return PoolManager::getInstance().getPool<std::string>()->acquire();
    }
    
    std::vector<std::string> getPoolStatistics() {
        return PoolManager::getInstance().getGlobalStatistics();
    }
}