// state_manager.h
// description: Centralized state management system with thread-safe operations
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - New Module
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include "common_defines.h"
#include "globals.h"

// =============================================================================
// STATE ENUMS AND STRUCTS
// =============================================================================
enum class SystemState {
    Initializing,
    Running,
    Paused,
    Shutting_Down,
    Error
};

enum class AimAssistMode {
    Disabled,
    Precision,
    Aggressive,
    Stealth,
    Custom
};

struct PlayerStats {
    std::chrono::steady_clock::time_point session_start;
    uint32_t shots_fired = 0;
    uint32_t hits_detected = 0;
    uint32_t headshots_detected = 0;
    double average_accuracy = 0.0;
    std::chrono::milliseconds total_active_time{0};
    PlayerMovementState dominant_movement_state = PlayerMovementState::Stationary;
};

struct SystemMetrics {
    double cpu_usage_percent = 0.0;
    size_t memory_usage_mb = 0;
    uint32_t fps_current = 0;
    uint32_t input_events_per_second = 0;
    double aim_assist_accuracy = 0.0;
    std::chrono::milliseconds avg_response_time{0};
};

struct WeaponContext {
    int active_profile_index = 0;
    std::string active_weapon_name;
    bool is_firing = false;
    std::chrono::steady_clock::time_point last_fire_time;
    uint32_t rounds_in_magazine = 30;
    double recoil_buildup = 0.0;
};

// =============================================================================
// STATE CHANGE EVENT SYSTEM
// =============================================================================
class StateManager {
public:
    // Event callback type
    using StateChangeCallback = std::function<void(const std::string&, const std::string&, const std::string&)>;
    
    // Constructor/Destructor
    StateManager();
    ~StateManager();
    
    // Core state management
    void initialize();
    void shutdown();
    bool isInitialized() const;
    
    // System state management
    SystemState getSystemState() const;
    void setSystemState(SystemState state);
    std::string getSystemStateString() const;
    
    // Player state management
    PlayerMovementState getPlayerMovementState() const;
    void setPlayerMovementState(PlayerMovementState state);
    bool isPlayerMoving() const;
    std::chrono::milliseconds getMovementDuration() const;
    
    // Aim assist state management
    AimAssistMode getAimAssistMode() const;
    void setAimAssistMode(AimAssistMode mode);
    bool isAimAssistActive() const;
    void setAimAssistActive(bool active);
    
    // Weapon context management
    const WeaponContext& getWeaponContext() const;
    void updateWeaponContext(const WeaponContext& context);
    void setActiveWeaponProfile(int profileIndex);
    
    // Statistics and metrics
    const PlayerStats& getPlayerStats() const;
    const SystemMetrics& getSystemMetrics() const;
    void updatePlayerStats(const PlayerStats& stats);
    void updateSystemMetrics(const SystemMetrics& metrics);
    void incrementShotsFired();
    void incrementHitsDetected();
    void incrementHeadshotsDetected();
    
    // Event system
    void registerStateChangeCallback(const std::string& name, StateChangeCallback callback);
    void unregisterStateChangeCallback(const std::string& name);
    
    // Configuration state
    void setConfigValue(const std::string& key, const std::string& value);
    std::string getConfigValue(const std::string& key, const std::string& defaultValue = "") const;
    
    // Debug and diagnostics
    std::vector<std::string> getDiagnosticInfo() const;
    void logStateChange(const std::string& component, const std::string& from, const std::string& to);
    
    // Thread-safe bulk updates
    void performBulkUpdate(std::function<void()> updateFunction);
    
private:
    // Thread safety
    mutable std::shared_mutex m_state_mutex;
    mutable std::mutex m_callback_mutex;
    mutable std::mutex m_metrics_mutex;
    
    // Core state
    std::atomic<SystemState> m_system_state{SystemState::Initializing};
    std::atomic<PlayerMovementState> m_player_movement_state{PlayerMovementState::Stationary};
    std::atomic<AimAssistMode> m_aim_assist_mode{AimAssistMode::Precision};
    std::atomic<bool> m_aim_assist_active{true};
    std::atomic<bool> m_initialized{false};
    
    // State tracking
    std::chrono::steady_clock::time_point m_movement_state_change_time;
    std::chrono::steady_clock::time_point m_last_activity_time;
    
    // Complex state objects (protected by mutex)
    WeaponContext m_weapon_context;
    PlayerStats m_player_stats;
    SystemMetrics m_system_metrics;
    std::unordered_map<std::string, std::string> m_config_values;
    
    // Event system
    std::unordered_map<std::string, StateChangeCallback> m_callbacks;
    
    // Helper methods
    void triggerStateChangeEvent(const std::string& component, const std::string& from, const std::string& to);
    void updateLastActivity();
    std::string aimAssistModeToString(AimAssistMode mode) const;
    std::string systemStateToString(SystemState state) const;
};

// =============================================================================
// GLOBAL STATE MANAGER INSTANCE
// =============================================================================
extern std::unique_ptr<StateManager> g_stateManager;

// =============================================================================
// CONVENIENCE MACROS FOR COMMON OPERATIONS
// =============================================================================
#define STATE_MGR() (g_stateManager.get())
#define GET_PLAYER_STATE() (STATE_MGR() ? STATE_MGR()->getPlayerMovementState() : PlayerMovementState::Stationary)
#define SET_PLAYER_STATE(state) if(STATE_MGR()) STATE_MGR()->setPlayerMovementState(state)
#define IS_ASSIST_ACTIVE() (STATE_MGR() ? STATE_MGR()->isAimAssistActive() : false)
#define GET_WEAPON_CONTEXT() (STATE_MGR() ? STATE_MGR()->getWeaponContext() : WeaponContext{})