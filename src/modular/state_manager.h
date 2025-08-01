// state_manager.h - FINAL CORRECTED VERSION v5.2.0
// description: Centralized state management system.
// version: 5.2.0 - Aligned with updated globals.h data structures.
// date: 2025-07-21
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
#include <memory>

// Include necessary type definitions from other modules
#include "globals.h"
#include "assist_optimized.h"
#include "config.h"

class StateManager;
extern std::unique_ptr<StateManager> g_stateManager;
bool initializeStateManager();
void shutdownStateManager();
StateManager* getStateManager();
#define STATE_MGR() (getStateManager())

class StateManager {
public:
    using StateChangeCallback = std::function<void(const std::string&, const std::string&, const std::string&)>;

    StateManager();
    ~StateManager();
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

    // --- Core Management ---
    void initialize();
    void shutdown();
    bool isInitialized() const;

    // --- System State ---
    SystemState getSystemState() const;
    void setSystemState(SystemState state);
    bool isRunning() const;

    // --- AppSettings State ---
    const AppSettings& getAppSettings() const;
    void setAppSettings(const AppSettings& settings);

    // --- Convenience Getters/Setters ---
    bool isAimAssistEnabled() const;
    void setAimAssistEnabled(bool enabled);
    const Keybindings& getKeybindings() const;
    PlayerMovementState getPlayerMovementState() const;
    void setPlayerMovementState(PlayerMovementState state);
    const TargetInfo& getCurrentTarget() const;
    void setCurrentTarget(const TargetInfo& target);
    bool hasValidTarget() const;
    POINT getCursorPosition() const;
    void setCursorPosition(int x, int y);
    POINT getScreenCenter() const;

    // --- Weapon Profile Management ---
    const std::vector<WeaponProfile>& getWeaponProfiles() const;
    const WeaponProfile* getActiveWeaponProfile() const;
    int getActiveWeaponProfileIndex() const;
    void setActiveWeaponProfileByIndex(int profileIndex);
    bool loadWeaponProfiles(const std::vector<WeaponProfile>& profiles);

    // --- Event Callback System ---
    void registerStateChangeCallback(const std::string& name, StateChangeCallback callback);
    void unregisterStateChangeCallback(const std::string& name);

private:
    void triggerStateChangeEvent(const std::string& component, const std::string& from, const std::string& to);
    std::string systemStateToString(SystemState state) const;
    
    mutable std::shared_mutex m_state_mutex;
    mutable std::mutex m_callback_mutex;

    std::atomic<bool> m_initialized{false};
    std::atomic<SystemState> m_system_state{SystemState::INACTIVE};

    AppSettings m_appSettings;
    
    // Non-config state
    std::atomic<PlayerMovementState> m_player_movement_state{PlayerMovementState::Stationary};
    TargetInfo m_current_target;
    POINT m_cursor_position{0, 0};
    
    std::unordered_map<std::string, StateChangeCallback> m_callbacks;
};