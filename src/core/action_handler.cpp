// action_handler.cpp - CORRECTED AND UPDATED VERSION v1.2.1
// description: Implementation for the ActionHandler class. This system listens for
//              raw input events and translates them into semantic game actions.
// version: 1.2.1 - Corrected signed/unsigned warnings and missing includes.
// date: 2025-07-21
// project: Tactical Aim Assist

#include "action_handler.h" // For ActionHandler class
#include "state_manager.h" // For state manager
#include "event_system.h" // For event system
#include "globals.h" // For Keybindings struct, logging, and getCurrentModifierState()

// =============================================================================
// GLOBAL INSTANCE DEFINITION
// =============================================================================

std::unique_ptr<ActionHandler> g_actionHandler;

// =============================================================================
// ActionHandler CLASS IMPLEMENTATION
// =============================================================================

ActionHandler::ActionHandler(StateManager& stateManager, EventSystem& eventSystem)
    : m_stateManager(stateManager), m_eventSystem(eventSystem) {}

void ActionHandler::initialize() {
    if (m_isInitialized) {
        return;
    }
    logMessage("ActionHandler: Initializing and subscribing to events...");
    subscribeToEvents();
    m_isInitialized = true;
    logMessage("ActionHandler: Initialized successfully.");
}

void ActionHandler::shutdown() {
    if (!m_isInitialized) {
        return;
    }
    logMessage("ActionHandler: Shutting down and unsubscribing from events...");
    m_eventSystem.unsubscribeAll("ActionHandler_KeyListener");
    m_isInitialized = false;
}

void ActionHandler::subscribeToEvents() {
    m_eventSystem.subscribe(
        EventType::KeyPressed, 
        "ActionHandler_KeyListener",
        [this](const BaseEvent& event) {
            this->handleKeyPressed(event);
        }
    );
}

void ActionHandler::handleKeyPressed(const BaseEvent& event) {
    if (!m_stateManager.isRunning()) {
        return;
    }

    if (!event.hasDataType<int>()) {
        logError("ActionHandler: Received KeyPressed event with invalid data type.");
        return;
    }
    
    const int vk_code = event.getData<int>();
    const int current_mod = getCurrentModifierState();
    const Keybindings& bindings = m_stateManager.getKeybindings(); 

    // --- Check against all known actions ---
    // CORRECTED: Added static_cast to resolve signed/unsigned comparison warnings.
    if (static_cast<unsigned int>(vk_code) == bindings.emergency_stop_vk && current_mod == bindings.emergency_stop_mod) {
        m_eventSystem.publishEvent(EventType::SystemEmergencyStop);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.super_emergency_vk && current_mod == bindings.super_emergency_mod) {
        m_eventSystem.publishEvent(EventType::SystemEmergencyStop);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.exit_vk && current_mod == bindings.exit_mod) {
        m_eventSystem.publishEvent(EventType::SystemShutdownRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.aim_assist_toggle_vk && current_mod == bindings.aim_assist_toggle_mod) {
        m_eventSystem.publishEvent(EventType::ToggleAimAssistRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.trigger_bot_toggle_vk && current_mod == bindings.trigger_bot_toggle_mod) {
        m_eventSystem.publishEvent(EventType::ToggleTriggerBotRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.silent_aim_toggle_vk && current_mod == bindings.silent_aim_toggle_mod) {
        m_eventSystem.publishEvent(EventType::ToggleSilentAimRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.prediction_toggle_vk && current_mod == bindings.prediction_toggle_mod) {
        m_eventSystem.publishEvent(EventType::TogglePredictionRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.recoil_toggle_vk && current_mod == bindings.recoil_toggle_mod) {
        m_eventSystem.publishEvent(EventType::ToggleRecoilCompRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.profile_next_vk && current_mod == bindings.profile_next_mod) {
        m_eventSystem.publishEvent(EventType::CycleProfileNextRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.profile_prev_vk && current_mod == bindings.profile_prev_mod) {
        m_eventSystem.publishEvent(EventType::CycleProfilePrevRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.sens_increase_vk && current_mod == bindings.sens_increase_mod) {
        m_eventSystem.publishEvent(EventType::IncreaseSensitivityRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.sens_decrease_vk && current_mod == bindings.sens_decrease_mod) {
        m_eventSystem.publishEvent(EventType::DecreaseSensitivityRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.fov_increase_vk && current_mod == bindings.fov_increase_mod) {
        m_eventSystem.publishEvent(EventType::IncreaseFovRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.fov_decrease_vk && current_mod == bindings.fov_decrease_mod) {
        m_eventSystem.publishEvent(EventType::DecreaseFovRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.smooth_increase_vk && current_mod == bindings.smooth_increase_mod) {
        m_eventSystem.publishEvent(EventType::IncreaseSmoothingRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.smooth_decrease_vk && current_mod == bindings.smooth_decrease_mod) {
        m_eventSystem.publishEvent(EventType::DecreaseSmoothingRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.smart_sprint_left_vk && current_mod == bindings.smart_sprint_left_mod) {
        m_eventSystem.publishEventWithData(EventType::MovementActionRequested, std::string("SmartSprintLeft"));
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.smart_sprint_right_vk && current_mod == bindings.smart_sprint_right_mod) {
        m_eventSystem.publishEventWithData(EventType::MovementActionRequested, std::string("SmartSprintRight"));
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.stats_reset_vk && current_mod == bindings.stats_reset_mod) {
        m_eventSystem.publishEvent(EventType::ResetStatsRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.status_print_vk && current_mod == bindings.status_print_mod) {
        m_eventSystem.publishEvent(EventType::PrintStatusRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.debug_toggle_vk && current_mod == bindings.debug_toggle_mod) {
        m_eventSystem.publishEvent(EventType::ToggleDebugModeRequested);
    }
    else if (static_cast<unsigned int>(vk_code) == bindings.test_mode_vk && current_mod == bindings.test_mode_mod) {
        m_eventSystem.publishEvent(EventType::ToggleTestModeRequested);
    }
}


// =============================================================================
// GLOBAL C-STYLE API IMPLEMENTATION
// =============================================================================

bool initializeActionHandler() {
    if (!getStateManager() || !getEventSystem()) {
        logError("ActionHandler Error: StateManager or EventSystem not initialized.");
        return false;
    }
    g_actionHandler = std::make_unique<ActionHandler>(*getStateManager(), *getEventSystem());
    g_actionHandler->initialize();
    return true;
}

void shutdownActionHandler() {
    if (g_actionHandler) {
        g_actionHandler->shutdown();
        g_actionHandler.reset();
    }
}