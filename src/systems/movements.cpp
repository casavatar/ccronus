// movements.cpp - CORRECTED AND UPDATED VERSION v1.1.0
// --------------------------------------------------------------------------------------
// description: Implementation of the MovementSystem class.
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 1.1.0 - Fixed exception handling and C++20 initializer warnings.
// license: GNU General Public License v3.0
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "movements.h"
#include "state_manager.h"
#include "event_system.h"
#include "globals.h"

#include <random>
#include <algorithm>
#include <cctype>

// Global instance
std::unique_ptr<MovementSystem> g_movementSystem;

namespace { // Anonymous namespace for internal linkage
    thread_local std::mt19937 gen(std::random_device{}());

    int smartRandom(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    void smartKey(WORD key, int delay) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key;
        SendInput(1, &input, sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay + smartRandom(-5, 5)));
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    
    void smartMultiKey(const std::vector<WORD>& keys, int delay) {
        if (keys.empty()) return;
        std::vector<INPUT> inputs(keys.size());
        for (size_t i = 0; i < keys.size(); ++i) {
            inputs[i].type = INPUT_KEYBOARD;
            inputs[i].ki.wVk = keys[i];
        }
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay + smartRandom(-10, 10)));
        for (auto& input : inputs) {
            input.ki.dwFlags = KEYEVENTF_KEYUP;
        }
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    }
} // namespace

MovementSystem::MovementSystem(StateManager& stateManager, EventSystem& eventSystem)
    : m_stateManager(stateManager), m_eventSystem(eventSystem) {}

MovementSystem::~MovementSystem() {
    shutdown();
}

void MovementSystem::initialize() {
    logMessage("MovementSystem: Initializing...");
    m_shutdownFlag.store(false);
    subscribeToEvents();
    m_workerThread = std::thread(&MovementSystem::movementWorkerThread, this);
    logMessage("MovementSystem: Worker thread started. System initialized.");
}

void MovementSystem::shutdown() {
    if (m_shutdownFlag.exchange(true)) return;
    logMessage("MovementSystem: Shutting down...");
    m_queueCondVar.notify_one();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    logMessage("MovementSystem: Shutdown complete.");
}

void MovementSystem::subscribeToEvents() {
    m_eventSystem.subscribe(
        EventType::MovementActionRequested,
        "MovementSystem_RequestHandler",
        [this](const BaseEvent& event) {
            this->handleMovementRequest(event);
        }
    );
}

void MovementSystem::handleMovementRequest(const BaseEvent& event) {
    if (m_isExecutingMovement.load()) {
        logWarning("MovementSystem: Ignoring new request, a movement is already in progress.");
        return;
    }

    // CORRECTED: Replaced try/catch with a safer type check.
    if (!event.hasDataType<std::string>()) {
        logError("MovementSystem: Received movement request with invalid data type.");
        return;
    }

    std::string movementName = event.getData<std::string>();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_movementQueue.push(movementName);
    }
    m_queueCondVar.notify_one();
}

void MovementSystem::movementWorkerThread() {
    while (!m_shutdownFlag.load()) {
        std::string movementName;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondVar.wait(lock, [this] {
                return m_shutdownFlag.load() || !m_movementQueue.empty();
            });

            if (m_shutdownFlag.load()) break;

            movementName = m_movementQueue.front();
            m_movementQueue.pop();
        }

        m_isExecutingMovement.store(true);
        dispatchMovement(movementName);
        m_isExecutingMovement.store(false);
    }
}

void MovementSystem::dispatchMovement(const std::string& movementName) {
    logMessage("MovementSystem: Executing '" + movementName + "'...");
    if (movementName == "SmartSprintLeft") executeSmartDiagonalSprint(true);
    else if (movementName == "SmartSprintRight") executeSmartDiagonalSprint(false);
    else if (movementName == "PredictiveSlide") executePredictiveSlide();
    // ... other movement dispatches ...
    else if (movementName == "SlideCancelDirectional") executeSlideCancelDirectional();
    else {
        logWarning("MovementSystem: Unknown movement requested: " + movementName);
    }
}

void MovementSystem::executeSmartDiagonalSprint(bool leftDirection) {
    WORD primaryKey = leftDirection ? 'A' : 'D';
    smartMultiKey({primaryKey, 'W', VK_SHIFT}, 150 + smartRandom(-20, 20));
}

void MovementSystem::executePredictiveSlide() {
    smartKey(VK_CONTROL, 200 + smartRandom(-30, 30));
}

void MovementSystem::executeSlideCancelDirectional() {
    INPUT inputs[2];
    
    // Phase 1: Press and hold sprint and crouch
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki = { VK_SHIFT, 0, 0, 0, 0 };

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki = { VK_CONTROL, 0, 0, 0, 0 };
    
    SendInput(2, inputs, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(120 + smartRandom(0, 30)));
    
    // Phase 2: Release crouch to cancel
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputs[1], sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + smartRandom(0, 20)));
    
    // Phase 3: Release sprint
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputs[0], sizeof(INPUT));
}

void MovementSystem::executeDiveDirectionalIntelligent() {
    smartKey(VK_SHIFT, 30); // Start sprint
    smartKey(VK_CONTROL, 30); // Tap crouch to slide
    smartKey('Z', 30); // Tap prone to dive
}

void MovementSystem::executeOmnidirectionalSlide() {
    executeSlideCancelDirectional();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    smartKey(smartRandom(0, 1) ? 'A' : 'D', 100);
}

void MovementSystem::executeContextualStrafeJump() {
    WORD strafeKey = smartRandom(0, 1) ? 'A' : 'D';
    smartMultiKey({strafeKey, VK_SPACE}, 150);
}

void MovementSystem::executeSmartCornerBounce(bool leftCorner) {
    // Execute a smart corner bounce movement
    WORD directionKey = leftCorner ? 'A' : 'D';
    smartMultiKey({directionKey, VK_SHIFT}, 100 + smartRandom(-15, 15));
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + smartRandom(0, 20)));
    smartKey(VK_SPACE, 30); // Jump
}

void MovementSystem::executeMovementTest() {
    logMessage("--- Movement Test Sequence Start ---");
    executeSmartDiagonalSprint(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    executePredictiveSlide();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    executeSmartCornerBounce(false);
    logMessage("--- Movement Test Sequence End ---");
}

bool initializeMovementSystem() {
    if (!getStateManager() || !getEventSystem()) {
        logError("MovementSystem Error: StateManager or EventSystem not initialized.");
        return false;
    }
    g_movementSystem = std::make_unique<MovementSystem>(*getStateManager(), *getEventSystem());
    g_movementSystem->initialize();
    return true;
}

void shutdownMovementSystem() {
    if (g_movementSystem) {
        g_movementSystem->shutdown();
        g_movementSystem.reset();
    }
}