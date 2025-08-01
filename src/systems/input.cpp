// input.cpp - REFACTORED AND UPDATED VERSION v4.0.0
// description: Input handling system encapsulated in a class. This system detects raw
//              hardware input, determines player movement state, updates the StateManager,
//              and publishes generic input events to the EventSystem.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 4.0.0 - Refactored into InputSystem class, decoupled from game logic.
// date: 2025-07-20
// project: Tactical Aim Assist

#include "input.h"
#include "state_manager.h"
#include "event_system.h"
#include "globals.h" // For logging and compatibility

#include <iostream>
#include <thread>
#include <chrono>

// =============================================================================
// GLOBAL INPUT SYSTEM INSTANCE
// =============================================================================

std::unique_ptr<InputSystem> g_inputSystem;

// =============================================================================
// InputSystem CLASS IMPLEMENTATION
// =============================================================================

InputSystem::InputSystem(StateManager& stateManager, EventSystem& eventSystem)
    : m_stateManager(stateManager), 
      m_eventSystem(eventSystem)
{
    // Initialize key state arrays to false (released)
    m_lastKeyState.fill(false);
    m_currentKeyState.fill(false);
}

InputSystem::~InputSystem() {
    shutdown();
}

bool InputSystem::initialize() {
    logMessage("InputSystem: Initializing...");
    
    // Set initial mouse position in the state manager from the system
    POINT initial_cursor;
    if (GetCursorPos(&initial_cursor)) {
        m_lastMousePosition = initial_cursor;
        m_stateManager.setCursorPosition(initial_cursor.x, initial_cursor.y);
    }

    m_isRunning.store(true);
    logMessage("InputSystem: Initialized successfully.");
    return true;
}

void InputSystem::shutdown() {
    m_isRunning.store(false);
    logMessage("InputSystem: Shutdown complete.");
}

void InputSystem::runInputLoop() {
    logMessage("InputSystem: Starting input processing loop...");
    
    while (m_isRunning.load() && g_application_running.load()) {
        // Process keyboard and mouse state at a regular interval
        processKeyboardState();
        processMouseState();

        // Control the polling rate to avoid excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    logMessage("InputSystem: Input loop finished.");
}

// --- PRIVATE METHODS ---

void InputSystem::processKeyboardState() {
    // 1. Update the current state of all relevant keys
    // This range covers most standard keys
    for (int vk = 0x01; vk < 0xFF; ++vk) {
        m_currentKeyState[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    // 2. Detect new key presses and publish events
    // This system's job is to REPORT input, not INTERPRET it.
    for (int vk = 0x01; vk < 0xFF; ++vk) {
        // If key was not pressed before but is pressed now
        if (m_currentKeyState[vk] && !m_lastKeyState[vk]) {
            m_eventSystem.publishEventWithData(EventType::KeyPressed, vk);
        }
    }

    // 3. Update the player's movement state based on currently held keys
    updateMovementState();

    // 4. Update the last key state for the next frame's comparison
    m_lastKeyState = m_currentKeyState;
}

void InputSystem::processMouseState() {
    POINT current_cursor;
    if (!GetCursorPos(&current_cursor)) {
        return; // Failed to get cursor position
    }

    // Check if the mouse has moved
    if (current_cursor.x != m_lastMousePosition.x || current_cursor.y != m_lastMousePosition.y) {
        // Update the state in the StateManager
        m_stateManager.setCursorPosition(current_cursor.x, current_cursor.y);

        // Publish a generic mouse movement event
        MouseMovedEventData event_data;
        event_data.new_x = current_cursor.x;
        event_data.new_y = current_cursor.y;
        event_data.delta_x = current_cursor.x - m_lastMousePosition.x;
        event_data.delta_y = current_cursor.y - m_lastMousePosition.y;
        m_eventSystem.publishEventWithData(EventType::MouseMoved, event_data);

        m_lastMousePosition = current_cursor;
    }
}

void InputSystem::updateMovementState() {
    // This logic determines the player's physical state based on keys being held down.
    
    bool isShiftPressed = m_currentKeyState[VK_LSHIFT] || m_currentKeyState[VK_RSHIFT];
    bool isCtrlPressed = m_currentKeyState[VK_LCONTROL] || m_currentKeyState[VK_RCONTROL];
    bool isSpacePressed = m_currentKeyState[VK_SPACE];
    bool isWASDPressed = m_currentKeyState['W'] || m_currentKeyState['A'] ||
                         m_currentKeyState['S'] || m_currentKeyState['D'];

    PlayerMovementState new_state = PlayerMovementState::Stationary;

    if (isSpacePressed) {
        // For simplicity, we'll just call it Jumping. A more complex system
        // could check for time in air to determine if Falling.
        new_state = PlayerMovementState::Jumping;
    } else if (isWASDPressed) {
        if (isShiftPressed && isCtrlPressed) {
            new_state = PlayerMovementState::Sliding; // Example of a combo move
        } else if (isShiftPressed) {
            new_state = PlayerMovementState::Sprinting;
        } else if (isCtrlPressed) {
            new_state = PlayerMovementState::Crouching;
        } else {
            // Check for strafing
            bool isStrafingOnly = (m_currentKeyState['A'] || m_currentKeyState['D']) &&
                                  !(m_currentKeyState['W'] || m_currentKeyState['S']);
            if(isStrafingOnly) {
                new_state = PlayerMovementState::Strafing;
            } else {
                 new_state = PlayerMovementState::Running; // Default non-sprint movement
            }
        }
    } else {
        new_state = PlayerMovementState::Stationary;
    }

    // Only update the StateManager if the state has actually changed
    if (m_stateManager.getPlayerMovementState() != new_state) {
        m_stateManager.setPlayerMovementState(new_state);
    }
}


// =============================================================================
// GLOBAL C-STYLE API IMPLEMENTATION
// =============================================================================

bool initializeInputSystem() {
    if (!getStateManager() || !getEventSystem()) {
        logError("InputSystem Error: StateManager or EventSystem not initialized before InputSystem.");
        return false;
    }
    g_inputSystem = std::make_unique<InputSystem>(*getStateManager(), *getEventSystem());
    return g_inputSystem->initialize();
}

void shutdownInputSystem() {
    if (g_inputSystem) {
        g_inputSystem->shutdown();
        g_inputSystem.reset();
    }
}

void runInputLoop() {
    if (g_inputSystem) {
        g_inputSystem->runInputLoop();
    } else {
        logError("InputSystem Error: Tried to run loop on an uninitialized system.");
    }
}