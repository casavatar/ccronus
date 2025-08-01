// systems.h - CORRECTED VERSION v1.1.0
// description: Header for core gameplay systems.
// version: 1.1.0 - Integrated systems with StateManager.
// date: 2025-07-21
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <deque>
#include <atomic>
#include <chrono>
#include <cmath>
#include "common_defines.h"

// Forward declarations
class StateManager;

// =============================================================================
// ANTI-DETECTION SYSTEM
// =============================================================================
class AntiDetectionSystem {
public:
    enum Context { IDLE, MOVEMENT, COMBAT };

    AntiDetectionSystem(StateManager& stateManager);

    void updateContext(Context newContext);
    void registerAction(UINT actionKey);
    int getHumanDelay() const;
    double getHumanAccuracy() const;
    bool needsMicroPause() const;
    void executeMicroPause();
    bool shouldAddJitter() const;
    std::pair<double, double> getJitter() const;
    size_t getTotalActions() const;
    uint64_t getLastActionTime() const;
    Context getCurrentContext() const;
    void resetStats();

private:
    StateManager& m_stateManager;
    Context m_currentContext;
    std::atomic<uint64_t> m_lastActionTime{0};
    std::atomic<uint64_t> m_lastMicroPause{0};
    std::atomic<size_t> m_totalActions{0};
    std::atomic<double> m_humanAccuracyFactor{0.85};
};

// =============================================================================
// PERFORMANCE OPTIMIZER
// =============================================================================
class PerformanceOptimizer {
public:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    
    void addTask(std::function<void()> task);
    size_t getTasksProcessed() const;
    size_t getTasksQueued() const;
    size_t getPendingTasks() const;

private:
    std::queue<std::function<void()>> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::atomic<bool> m_running{true};
    std::thread m_workerThread;
    std::atomic<size_t> m_tasksProcessed{0};
    std::atomic<size_t> m_tasksQueued{0};
    
    void workerFunction();
};

// =============================================================================
// PREDICTIVE AIM SYSTEM
// =============================================================================
class PredictiveAimSystem {
public:
    void updateTarget(POINT position, double confidence = 1.0);
    POINT getPredictedTarget(double predictionTimeMs = 100.0);
    double getTargetVelocity() const;
    void clearHistory();
    size_t getHistorySize() const;

private:
    struct TargetHistory {
        POINT position;
        uint64_t timestamp;
        double confidence;
    };
    
    std::vector<TargetHistory> m_targetHistory;
    mutable std::mutex m_historyMutex;
    static constexpr size_t MAX_HISTORY = 10;
};

// =============================================================================
// FLUID MOVEMENT SYSTEM
// =============================================================================
class FluidMovementSystem {
public:
    struct BezierCurve {
        POINT p0, p1, p2, p3; // Control points
        int steps;
        
        POINT getPointAt(double t) const;
    };
    
    void executeFluidMovement(POINT start, POINT end, int durationMs = 200);
    BezierCurve generateNaturalCurve(POINT start, POINT end);
    bool isCurrentlyMoving() const;

private:
    std::mutex m_movementMutex;
    std::atomic<bool> m_isMoving{false};
};

// =============================================================================
// MOMENTUM SYSTEM
// =============================================================================
class MomentumSystem {
public:
    void updateMovementState(bool forward, bool backward, bool left, bool right);
    void addMomentum(double amount);
    void decayMomentum();
    bool canChainMovement() const;
    int getOptimalDirection(int baseDirection) const;
    void setLastDirection(int direction);
    double getCurrentMomentum() const;
    int getLastDirection() const;
    bool isMoving() const;

private:
    std::atomic<double> m_currentMomentum{0.0};
    std::atomic<int> m_lastDirection{0}; // 0=W, 1=A, 2=S, 3=D
    std::atomic<bool> m_isAccelerating{false};
    std::atomic<uint64_t> m_lastMovementTime{0};
    
    // Movement state
    std::atomic<bool> m_movingForward{false};
    std::atomic<bool> m_movingBackward{false};
    std::atomic<bool> m_movingLeft{false};
    std::atomic<bool> m_movingRight{false};
};

// =============================================================================
// FEEDBACK SYSTEM
// =============================================================================
class FeedbackSystem {
public:
    void startMovement(const std::string& movementName, int expectedDuration = 500);
    void endMovement();
    bool isMovementActive() const;
    std::string getCurrentMovement() const;
    int getRemainingDuration() const;

private:
    mutable std::mutex m_feedbackMutex;
    std::atomic<uint64_t> m_lastFeedbackTime{0};
    std::string m_currentMovement;
    std::atomic<int> m_activeDuration{0};
};

// =============================================================================
// GLOBAL SYSTEM FUNCTIONS
// =============================================================================
bool initializeAllSystems();
void shutdownAllSystems();

// System accessors
PerformanceOptimizer* getPerformanceOptimizer();
AntiDetectionSystem* getAntiDetectionSystem();
PredictiveAimSystem* getPredictiveAimSystem();
FluidMovementSystem* getFluidMovementSystem();
MomentumSystem* getMomentumSystem();
FeedbackSystem* getFeedbackSystem();

// System diagnostics
void printSystemStatus();
void testAllSystems();
void monitorSystemPerformance();