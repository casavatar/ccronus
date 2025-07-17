// state_manager.cpp - CORRECTED VERSION v3.0.4
// description: Implementation of centralized state management system
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.4 - Fixed PlayerMovementState enum values
// date: 2025-07-16
// project: Tactical Aim Assist

#include "state_manager.h"
#include "globals.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================
std::unique_ptr<StateManager> g_stateManager = nullptr;

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================
StateManager::StateManager() {
    m_movement_state_change_time = std::chrono::steady_clock::now();
    m_last_activity_time = std::chrono::steady_clock::now();
    m_player_stats.session_start = std::chrono::steady_clock::now();
    
    DEBUG_LOG("StateManager constructed");
}

StateManager::~StateManager() {
    shutdown();
    DEBUG_LOG("StateManager destroyed");
}

// =============================================================================
// CORE MANAGEMENT
// =============================================================================
void StateManager::initialize() {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    
    if (m_initialized.load()) {
        logMessage("WARNING: StateManager already initialized");
        return;
    }
    
    // Initialize default values
    m_weapon_context.active_profile_index = 0;
    m_weapon_context.active_weapon_name = "Default";
    m_weapon_context.rounds_in_magazine = 30;
    
    // Set default config values
    m_config_values["smoothing_factor"] = "0.85";
    m_config_values["prediction_aggressiveness"] = "0.75";
    m_config_values["anti_detection_level"] = "Medium";
    
    setSystemState(SystemState::Running);
    m_initialized.store(true);
    
    logMessage("StateManager initialized successfully");
    triggerStateChangeEvent("System", "Initializing", "Running");
}

void StateManager::shutdown() {
    if (!m_initialized.load()) return;
    
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    
    setSystemState(SystemState::Shutting_Down);
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> callback_lock(m_callback_mutex);
        m_callbacks.clear();
    }
    
    m_initialized.store(false);
    logMessage("StateManager shutdown completed");
}

bool StateManager::isInitialized() const {
    return m_initialized.load();
}

// =============================================================================
// SYSTEM STATE MANAGEMENT
// =============================================================================
SystemState StateManager::getSystemState() const {
    return m_system_state.load();
}

void StateManager::setSystemState(SystemState state) {
    SystemState oldState = m_system_state.exchange(state);
    if (oldState != state) {
        updateLastActivity();
        triggerStateChangeEvent("System", systemStateToString(oldState), systemStateToString(state));
        DEBUG_LOG("System state changed: " + systemStateToString(oldState) + " -> " + systemStateToString(state));
    }
}

std::string StateManager::getSystemStateString() const {
    return systemStateToString(getSystemState());
}

// =============================================================================
// PLAYER STATE MANAGEMENT
// =============================================================================
PlayerMovementState StateManager::getPlayerMovementState() const {
    return m_player_movement_state.load();
}

void StateManager::setPlayerMovementState(PlayerMovementState state) {
    PlayerMovementState oldState = m_player_movement_state.exchange(state);
    if (oldState != state) {
        std::unique_lock<std::shared_mutex> lock(m_state_mutex);
        m_movement_state_change_time = std::chrono::steady_clock::now();
        updateLastActivity();
        
        // Use the global function from globals.h
        triggerStateChangeEvent("PlayerMovement", 
                               ::playerMovementStateToString(oldState), 
                               ::playerMovementStateToString(state));
        
        DEBUG_LOG("Player movement state changed: " + 
                 ::playerMovementStateToString(oldState) + " -> " + 
                 ::playerMovementStateToString(state));
    }
}

bool StateManager::isPlayerMoving() const {
    PlayerMovementState state = getPlayerMovementState();
    return state != PlayerMovementState::Stationary;
}

std::chrono::milliseconds StateManager::getMovementDuration() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_movement_state_change_time);
}

// =============================================================================
// AIM ASSIST STATE MANAGEMENT
// =============================================================================
AimAssistMode StateManager::getAimAssistMode() const {
    return m_aim_assist_mode.load();
}

void StateManager::setAimAssistMode(AimAssistMode mode) {
    AimAssistMode oldMode = m_aim_assist_mode.exchange(mode);
    if (oldMode != mode) {
        updateLastActivity();
        triggerStateChangeEvent("AimAssist", 
                               aimAssistModeToString(oldMode), 
                               aimAssistModeToString(mode));
        DEBUG_LOG("Aim assist mode changed: " + aimAssistModeToString(oldMode) + " -> " + aimAssistModeToString(mode));
    }
}

bool StateManager::isAimAssistActive() const {
    return m_aim_assist_active.load() && getSystemState() == SystemState::Running;
}

void StateManager::setAimAssistActive(bool active) {
    bool oldActive = m_aim_assist_active.exchange(active);
    if (oldActive != active) {
        updateLastActivity();
        triggerStateChangeEvent("AimAssist", 
                               oldActive ? "Active" : "Inactive", 
                               active ? "Active" : "Inactive");
        DEBUG_LOG("Aim assist " + std::string(active ? "activated" : "deactivated"));
    }
}

// =============================================================================
// WEAPON CONTEXT MANAGEMENT
// =============================================================================
const WeaponContext& StateManager::getWeaponContext() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_weapon_context;
}

void StateManager::updateWeaponContext(const WeaponContext& context) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    
    bool profileChanged = (m_weapon_context.active_profile_index != context.active_profile_index);
    bool weaponChanged = (m_weapon_context.active_weapon_name != context.active_weapon_name);
    
    m_weapon_context = context;
    updateLastActivity();
    
    if (profileChanged) {
        triggerStateChangeEvent("WeaponProfile", 
                               std::to_string(m_weapon_context.active_profile_index),
                               std::to_string(context.active_profile_index));
    }
    
    if (weaponChanged) {
        triggerStateChangeEvent("Weapon", m_weapon_context.active_weapon_name, context.active_weapon_name);
    }
}

void StateManager::setActiveWeaponProfile(int profileIndex) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    
    if (profileIndex != m_weapon_context.active_profile_index) {
        int oldIndex = m_weapon_context.active_profile_index;
        m_weapon_context.active_profile_index = profileIndex;
        updateLastActivity();
        
        triggerStateChangeEvent("WeaponProfile", std::to_string(oldIndex), std::to_string(profileIndex));
        DEBUG_LOG("Active weapon profile changed: " + std::to_string(oldIndex) + " -> " + std::to_string(profileIndex));
    }
}

// =============================================================================
// STATISTICS AND METRICS
// =============================================================================
const PlayerStats& StateManager::getPlayerStats() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    return m_player_stats;
}

const SystemMetrics& StateManager::getSystemMetrics() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    return m_system_metrics;
}

void StateManager::updatePlayerStats(const PlayerStats& stats) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_player_stats = stats;
    updateLastActivity();
}

void StateManager::updateSystemMetrics(const SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    m_system_metrics = metrics;
}

void StateManager::incrementShotsFired() {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_player_stats.shots_fired++;
    m_weapon_context.last_fire_time = std::chrono::steady_clock::now();
    m_weapon_context.is_firing = true;
    updateLastActivity();
}

void StateManager::incrementHitsDetected() {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_player_stats.hits_detected++;
    
    // Update accuracy
    if (m_player_stats.shots_fired > 0) {
        m_player_stats.average_accuracy = 
            static_cast<double>(m_player_stats.hits_detected) / m_player_stats.shots_fired * 100.0;
    }
    updateLastActivity();
}

void StateManager::incrementHeadshotsDetected() {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    m_player_stats.headshots_detected++;
    updateLastActivity();
}

// =============================================================================
// EVENT SYSTEM
// =============================================================================
void StateManager::registerStateChangeCallback(const std::string& name, StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_callbacks[name] = callback;
    DEBUG_LOG("Registered state change callback: " + name);
}

void StateManager::unregisterStateChangeCallback(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_callbacks.erase(name);
    DEBUG_LOG("Unregistered state change callback: " + name);
}

// =============================================================================
// CONFIGURATION STATE
// =============================================================================
void StateManager::setConfigValue(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    
    std::string oldValue = m_config_values[key];
    m_config_values[key] = value;
    
    if (oldValue != value) {
        triggerStateChangeEvent("Config:" + key, oldValue, value);
        DEBUG_LOG("Config value changed - " + key + ": " + oldValue + " -> " + value);
    }
}

std::string StateManager::getConfigValue(const std::string& key, const std::string& defaultValue) const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    
    auto it = m_config_values.find(key);
    return (it != m_config_values.end()) ? it->second : defaultValue;
}

// =============================================================================
// DEBUG AND DIAGNOSTICS
// =============================================================================
std::vector<std::string> StateManager::getDiagnosticInfo() const {
    std::shared_lock<std::shared_mutex> lock(m_state_mutex);
    
    std::vector<std::string> info;
    
    info.push_back("=== StateManager Diagnostics ===");
    info.push_back("System State: " + getSystemStateString());
    info.push_back("Player Movement: " + ::playerMovementStateToString(getPlayerMovementState()));
    info.push_back("Aim Assist Mode: " + aimAssistModeToString(getAimAssistMode()));
    info.push_back("Aim Assist Active: " + std::string(isAimAssistActive() ? "Yes" : "No"));
    
    info.push_back("--- Weapon Context ---");
    info.push_back("Active Profile: " + std::to_string(m_weapon_context.active_profile_index));
    info.push_back("Weapon Name: " + m_weapon_context.active_weapon_name);
    info.push_back("Is Firing: " + std::string(m_weapon_context.is_firing ? "Yes" : "No"));
    
    info.push_back("--- Player Stats ---");
    info.push_back("Shots Fired: " + std::to_string(m_player_stats.shots_fired));
    info.push_back("Hits Detected: " + std::to_string(m_player_stats.hits_detected));
    info.push_back("Headshots: " + std::to_string(m_player_stats.headshots_detected));
    info.push_back("Average Accuracy: " + std::to_string(m_player_stats.average_accuracy) + "%");
    
    return info;
}

void StateManager::logStateChange(const std::string& component, const std::string& from, const std::string& to) {
    logMessage("[STATE] " + component + ": " + from + " -> " + to);
}

// =============================================================================
// THREAD-SAFE BULK UPDATES
// =============================================================================
void StateManager::performBulkUpdate(std::function<void()> updateFunction) {
    std::unique_lock<std::shared_mutex> lock(m_state_mutex);
    updateFunction();
    updateLastActivity();
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================
void StateManager::triggerStateChangeEvent(const std::string& component, const std::string& from, const std::string& to) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    
    for (const auto& [name, callback] : m_callbacks) {
        // Sin try-catch ya que las excepciones están deshabilitadas
        if (callback) {
            callback(component, from, to);
        } else {
            logMessage("ERROR: Null callback in state change callback '" + name + "'");
        }
    }
}

void StateManager::updateLastActivity() {
    m_last_activity_time = std::chrono::steady_clock::now();
}

std::string StateManager::aimAssistModeToString(AimAssistMode mode) const {
    switch (mode) {
        case AimAssistMode::Disabled: return "Disabled";
        case AimAssistMode::Precision: return "Precision";
        case AimAssistMode::Aggressive: return "Aggressive";
        case AimAssistMode::Stealth: return "Stealth";
        case AimAssistMode::Custom: return "Custom";
        default: return "Unknown";
    }
}

std::string StateManager::systemStateToString(SystemState state) const {
    switch (state) {
        case SystemState::Initializing: return "Initializing";
        case SystemState::Running: return "Running";
        case SystemState::Paused: return "Paused";
        case SystemState::Shutting_Down: return "Shutting_Down";
        case SystemState::Error: return "Error";
        default: return "Unknown";
    }
}

// =============================================================================
// GLOBAL FUNCTIONS
// =============================================================================
bool initializeStateManager() {
    if (!g_stateManager) {
        g_stateManager = std::make_unique<StateManager>();
        g_stateManager->initialize();
        logMessage("✅ StateManager initialized successfully");
        return true;
    }
    return g_stateManager->isInitialized();
}

void shutdownStateManager() {
    if (g_stateManager) {
        g_stateManager->shutdown();
        g_stateManager.reset();
        logMessage("✅ StateManager shutdown completed");
    }
}

StateManager* getStateManager() {
    return g_stateManager.get();
}