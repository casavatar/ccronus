// state_manager.cpp - FINAL CORRECTED VERSION v5.2.0
// --------------------------------------------------------------------------------------
// description: Implementation of the centralized state management system.
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 5.2.0 - Aligned with updated globals.h data structures.
// license: GNU General Public License v3.0
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "state_manager.h"
#include <sstream>
#include <thread>
#include <algorithm>
#include <cctype>

// Global instance definition
std::unique_ptr<StateManager> g_stateManager = nullptr;

StateManager::StateManager() = default;
StateManager::~StateManager() { shutdown(); }

void StateManager::initialize() {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    if (m_initialized.load()) return;
    
    m_appSettings = AppSettings{};
    m_current_target = TargetInfo{};
    m_player_movement_state = PlayerMovementState::Stationary;
    m_cursor_position = { m_appSettings.screen_width / 2, m_appSettings.screen_height / 2 };

    m_initialized.store(true);
    setSystemState(SystemState::ACTIVE);
    logMessage("StateManager initialized successfully.");
}

void StateManager::shutdown() {
    if (!m_initialized.load()) return;
    setSystemState(SystemState::SHUTTING_DOWN);
    m_initialized.store(false);
    logMessage("StateManager shutdown complete.");
}

bool StateManager::isInitialized() const { return m_initialized.load(); }
SystemState StateManager::getSystemState() const { return m_system_state.load(); }
bool StateManager::isRunning() const { return getSystemState() == SystemState::ACTIVE; }

void StateManager::setSystemState(SystemState state) {
    SystemState old_state = m_system_state.exchange(state);
    if (old_state != state) {
        triggerStateChangeEvent("System", systemStateToString(old_state), systemStateToString(state));
    }
}

const AppSettings& StateManager::getAppSettings() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_appSettings;
}

void StateManager::setAppSettings(const AppSettings& settings) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_appSettings = settings;
}

bool StateManager::isAimAssistEnabled() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_appSettings.assist_enabled;
}

void StateManager::setAimAssistEnabled(bool enabled) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    if (m_appSettings.assist_enabled != enabled) {
        bool old_state = m_appSettings.assist_enabled;
        m_appSettings.assist_enabled = enabled;
        lock.unlock();
        triggerStateChangeEvent("AimAssist", old_state ? "Enabled" : "Disabled", enabled ? "Enabled" : "Disabled");
    }
}

const Keybindings& StateManager::getKeybindings() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_appSettings.keybindings;
}

PlayerMovementState StateManager::getPlayerMovementState() const {
    // This is atomic, no lock needed
    return m_player_movement_state.load();
}

void StateManager::setPlayerMovementState(PlayerMovementState state) {
    PlayerMovementState old_state = m_player_movement_state.exchange(state);
    if (old_state != state) {
        triggerStateChangeEvent("PlayerMovement", playerMovementStateToString(old_state), playerMovementStateToString(state));
    }
}

const TargetInfo& StateManager::getCurrentTarget() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_current_target;
}

void StateManager::setCurrentTarget(const TargetInfo& target) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_current_target = target;
}

bool StateManager::hasValidTarget() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_current_target.isValid();
}

POINT StateManager::getCursorPosition() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_cursor_position;
}

void StateManager::setCursorPosition(int x, int y) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_cursor_position.x = x;
    m_cursor_position.y = y;
}

POINT StateManager::getScreenCenter() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return { m_appSettings.screen_width / 2, m_appSettings.screen_height / 2 };
}

const std::vector<WeaponProfile>& StateManager::getWeaponProfiles() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_appSettings.weapon_profiles; // Assumes profiles are stored in AppSettings
}

const WeaponProfile* StateManager::getActiveWeaponProfile() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    const auto& profiles = m_appSettings.weapon_profiles;
    int index = m_appSettings.active_profile_index;

    if (index >= 0 && static_cast<size_t>(index) < profiles.size()) {
        return &profiles[index];
    }
    return nullptr;
}

int StateManager::getActiveWeaponProfileIndex() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_appSettings.active_profile_index;
}

void StateManager::setActiveWeaponProfileByIndex(int profileIndex) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    if (static_cast<size_t>(profileIndex) < m_appSettings.weapon_profiles.size()) {
        m_appSettings.active_profile_index = profileIndex;
    }
}

bool StateManager::loadWeaponProfiles(const std::vector<WeaponProfile>& profiles) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_appSettings.weapon_profiles = profiles;
    if (static_cast<size_t>(m_appSettings.active_profile_index) >= profiles.size()){
        m_appSettings.active_profile_index = -1; // Invalidate index if out of bounds
    }
    return true;
}

void StateManager::registerStateChangeCallback(const std::string& name, StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_callbacks[name] = callback;
}

void StateManager::unregisterStateChangeCallback(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_callbacks.erase(name);
}

void StateManager::triggerStateChangeEvent(const std::string& component, const std::string& from, const std::string& to) {
    std::stringstream ss;
    ss << "[STATE CHANGE] " << component << ": '" << from << "' -> '" << to << "'";
    logMessage(ss.str());

    std::lock_guard<std::mutex> lock(m_callback_mutex);
    for (const auto& pair : m_callbacks) {
        if (pair.second) {
            pair.second(component, from, to);
        }
    }
}

std::string StateManager::systemStateToString(SystemState state) const {
    switch(state) {
        case SystemState::INACTIVE: return "Inactive";
        case SystemState::INITIALIZING: return "Initializing";
        case SystemState::ACTIVE: return "Active";
        case SystemState::PAUSED: return "Paused";
        case SystemState::SHUTTING_DOWN: return "Shutting Down";
        case SystemState::ERROR_STATE: return "Error";
        default: return "Unknown";
    }
}

// Global function implementations
bool initializeStateManager() {
    if (!g_stateManager) {
        g_stateManager = std::make_unique<StateManager>();
        g_stateManager->initialize();
        return true;
    }
    return g_stateManager->isInitialized();
}

void shutdownStateManager() {
    if (g_stateManager) {
        g_stateManager->shutdown();
        g_stateManager.reset();
    }
}

StateManager* getStateManager() {
    return g_stateManager.get();
}