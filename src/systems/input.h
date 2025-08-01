// input.h - REFACTORED AND UPDATED VERSION v1.0.0
// description: Header for the InputSystem class. This system detects raw hardware
//              input, determines player movement state, updates the StateManager,
//              and publishes generic input events to the EventSystem.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 1.0.0 - Refactored into a class-based, event-driven system.
// date: 2025-07-21
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <memory>
#include <thread>
#include <atomic>
#include <array>

// Forward declarations
class StateManager;
class EventSystem;

/**
 * @class InputSystem
 * @brief Manages raw hardware input detection and player movement state.
 */
class InputSystem {
public:
    /**
     * @brief Constructs the InputSystem.
     * @param stateManager A reference to the central state manager.
     * @param eventSystem A reference to the event system for publishing input events.
     */
    InputSystem(StateManager& stateManager, EventSystem& eventSystem);
    ~InputSystem();

    // Deleted copy/move constructors to prevent copying.
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    /**
     * @brief Initializes the input system.
     * @return True if successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Shuts down the input system.
     */
    void shutdown();

    /**
     * @brief The main loop for the input processing thread. Polls hardware state.
     */
    void runInputLoop();

private:
    /**
     * @brief Processes the current state of the keyboard, detects new presses, and updates movement state.
     */
    void processKeyboardState();

    /**
     * @brief Processes the current state of the mouse, detects movement, and updates cursor state.
     */
    void processMouseState();

    /**
     * @brief Determines the player's movement state based on currently held keys and updates the StateManager.
     */
    void updateMovementState();

    // --- Member Variables ---
    StateManager& m_stateManager;
    EventSystem& m_eventSystem;
    
    std::atomic<bool> m_isRunning{false};

    // For tracking key press/release transitions
    std::array<bool, 256> m_lastKeyState;
    std::array<bool, 256> m_currentKeyState;

    // For tracking mouse movement
    POINT m_lastMousePosition = {0, 0};
};

// =============================================================================
// GLOBAL INSTANCE AND API
// =============================================================================

// Global unique pointer to the InputSystem instance.
extern std::unique_ptr<InputSystem> g_inputSystem;

/**
 * @brief Initializes the global InputSystem.
 * @return True if successful, false otherwise.
 */
bool initializeInputSystem();

/**
 * @brief Shuts down the global InputSystem.
 */
void shutdownInputSystem();

/**
 * @brief Global function to be used as the entry point for the input thread.
 */
void runInputLoop();