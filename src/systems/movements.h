// movements.h - REFACTORED AND UPDATED VERSION v1.0.0
// description: Header for the MovementSystem class. This system is responsible
//              for executing complex, multi-step tactical movements in a
//              non-blocking way by using a dedicated worker thread.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 1.0.0 - Initial class-based, event-driven design.
// date: 2025-07-20
// project: Tactical Aim Assist

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

// Forward declarations
class StateManager;
class EventSystem;
struct BaseEvent;

/**
 * @class MovementSystem
 * @brief Manages the execution of complex tactical movements.
 */
class MovementSystem {
public:
    /**
     * @brief Constructs the MovementSystem.
     * @param stateManager A reference to the central state manager.
     * @param eventSystem A reference to the event system for receiving commands.
     */
    MovementSystem(StateManager& stateManager, EventSystem& eventSystem);
    ~MovementSystem();

    // Deleted copy/move constructors to prevent copying.
    MovementSystem(const MovementSystem&) = delete;
    MovementSystem& operator=(const MovementSystem&) = delete;

    /**
     * @brief Initializes the system and starts its worker thread.
     */
    void initialize();

    /**
     * @brief Shuts down the system and joins its worker thread.
     */
    void shutdown();

private:
    /**
     * @brief Sets up subscriptions to relevant events (e.g., MovementActionRequested).
     */
    void subscribeToEvents();

    /**
     * @brief The main function for the dedicated movement worker thread.
     */
    void movementWorkerThread();

    /**
     * @brief Callback function to handle incoming movement requests from the EventSystem.
     * @param event The event containing the requested movement data.
     */
    void handleMovementRequest(const BaseEvent& event);

    /**
     * @brief Dispatches a movement name to the appropriate execution method.
     * @param movementName The name of the movement to execute (e.g., "SmartSprintLeft").
     */
    void dispatchMovement(const std::string& movementName);

    // --- Tactical Movement Implementations ---
    // These methods contain the actual logic for key presses and delays.
    void executeSmartDiagonalSprint(bool leftDirection);
    void executePredictiveSlide();
    void executeAntiDetectionDiveBack();
    void executeSmartCornerBounce(bool leftCorner);
    void executePredictiveCutback();
    void executeDropShotSupineSlide();
    void executeSlideCancelDirectional();
    void executeDiveDirectionalIntelligent();
    void executeOmnidirectionalSlide();
    void executeContextualStrafeJump();
    void executeMovementTest();
    
    // --- Member Variables ---
    StateManager& m_stateManager;
    EventSystem& m_eventSystem;
    
    std::thread m_workerThread;
    std::atomic<bool> m_shutdownFlag{false};

    std::queue<std::string> m_movementQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondVar;
    
    std::atomic<bool> m_isExecutingMovement{false};
};

// =============================================================================
// GLOBAL INSTANCE AND API
// =============================================================================

extern std::unique_ptr<MovementSystem> g_movementSystem;

/**
 * @brief Initializes the global MovementSystem.
 * @return True if successful, false otherwise.
 */
bool initializeMovementSystem();

/**
 * @brief Shuts down the global MovementSystem.
 */
void shutdownMovementSystem();