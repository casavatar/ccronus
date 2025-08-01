// action_handler.h - VERSION 1.0.0
// description: Header for the ActionHandler class. This system is responsible for
//              interpreting raw input events (e.g., KeyPressed) and translating
//              them into semantic game actions (e.g., ToggleAimAssist) based on
//              the current keybindings stored in the StateManager.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 1.0.0 - Initial creation
// date: 2025-07-20
// project: Tactical Aim Assist

#pragma once

#include <memory>

// Forward declarations
class StateManager;
class EventSystem;
struct BaseEvent;

/**
 * @class ActionHandler
 * @brief Interprets raw input events and converts them into gameplay actions.
 */
class ActionHandler {
public:
    /**
     * @brief Constructs the ActionHandler.
     * @param stateManager A reference to the central state manager for querying keybindings.
     * @param eventSystem A reference to the event system for subscribing to input and publishing actions.
     */
    ActionHandler(StateManager& stateManager, EventSystem& eventSystem);

    /**
     * @brief Initializes the system by subscribing to necessary input events.
     */
    void initialize();

    /**
     * @brief Shuts down the system, unsubscribing from events.
     */
    void shutdown();

private:
    /**
     * @brief Sets up all event subscriptions for this system.
     */
    void subscribeToEvents();

    /**
     * @brief Handles raw key press events from the InputSystem.
     * @param event The event data, containing the virtual key code.
     */
    void handleKeyPressed(const BaseEvent& event);

    // --- Member Variables ---
    StateManager& m_stateManager;
    EventSystem& m_eventSystem;
    bool m_isInitialized = false;
};

// =============================================================================
// GLOBAL INSTANCE AND API
// =============================================================================

// Global unique pointer to the ActionHandler instance.
extern std::unique_ptr<ActionHandler> g_actionHandler;

/**
 * @brief Initializes the global ActionHandler system.
 * @return True if initialization is successful, false otherwise.
 */
bool initializeActionHandler();

/**
 * @brief Shuts down the global ActionHandler system.
 */
void shutdownActionHandler();