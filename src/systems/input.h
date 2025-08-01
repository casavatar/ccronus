// input.h - REFACTORED AND UPDATED VERSION v2.0.0
// description: Header for the InputSystem class. This system detects raw hardware
//              input, determines player movement state, updates the StateManager,
//              and publishes generic input events to the EventSystem.
//              Enhanced with Z key slide movement support.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 2.0.0 - Enhanced with Z key slide movement and improved movement detection.
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
 *        Enhanced with Z key slide movement support and improved movement detection.
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

    /**
     * @brief Checks if the Z key is currently pressed for slide movement.
     * @return True if Z key is pressed, false otherwise.
     */
    bool isSlideKeyPressed() const;

    /**
     * @brief Gets the current slide movement state.
     * @return True if sliding is active, false otherwise.
     */
    bool isSliding() const;

    /**
     * @brief Enables or disables the movement system.
     * @param enabled True to enable, false to disable.
     */
    void setMovementSystemEnabled(bool enabled);

    /**
     * @brief Checks if the movement system is enabled.
     * @return True if enabled, false otherwise.
     */
    bool isMovementSystemEnabled() const;

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
     *        Enhanced with Z key slide movement detection.
     */
    void updateMovementState();

    /**
     * @brief Handles Z key slide movement logic.
     */
    void processSlideMovement();

    /**
     * @brief Checks for movement key combinations and determines the appropriate movement state.
     * @return The calculated movement state.
     */
    PlayerMovementState calculateMovementState();

    // --- Member Variables ---
    StateManager& m_stateManager;
    EventSystem& m_eventSystem;
    
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_movementSystemEnabled{true};
    std::atomic<bool> m_isSliding{false};

    // For tracking key press/release transitions
    std::array<bool, 256> m_lastKeyState;
    std::array<bool, 256> m_currentKeyState;

    // For tracking mouse movement
    POINT m_lastMousePosition = {0, 0};

    // Slide movement timing
    std::chrono::steady_clock::time_point m_slideStartTime;
    static constexpr auto SLIDE_DURATION_MS = std::chrono::milliseconds(500);
    static constexpr auto SLIDE_COOLDOWN_MS = std::chrono::milliseconds(1000);
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

/**
 * @brief Global function to check if Z key slide movement is active.
 * @return True if sliding, false otherwise.
 */
bool isSlideMovementActive();

/**
 * @brief Global function to enable/disable the movement system.
 * @param enabled True to enable, false to disable.
 */
void setMovementSystemEnabled(bool enabled);