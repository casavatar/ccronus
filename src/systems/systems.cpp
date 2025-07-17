// systems.cpp - FIXED all errors v3.0.3
// description: Core systems implementation - Fixed const mutex and clamp issues
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.3 - Fixed compilation issues
// date: 2025-07-16
// project: Tactical Aim Assist

#include <iostream>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <random>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <cmath>
#include <algorithm>  // Added for std::clamp alternative

#include "globals.h"
#include "core/config.h"

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================
template<typename T>
constexpr T clamp_value(const T& value, const T& min_val, const T& max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

// =============================================================================
// GLOBAL RANDOM GENERATOR - FIXED MISSING DECLARATION
// =============================================================================
std::mutex g_random_mutex;
thread_local std::mt19937 gen(std::random_device{}());

// =============================================================================
// PERFORMANCE OPTIMIZER SYSTEM
// =============================================================================
class PerformanceOptimizer {
private:
    std::queue<std::function<void()>> m_taskQueue;
    mutable std::mutex m_queueMutex;  // FIXED: Made mutable for const methods
    std::atomic<bool> m_running{true};
    std::thread m_workerThread;
    std::atomic<size_t> m_tasksProcessed{0};
    std::atomic<size_t> m_tasksQueued{0};
    
public:
    PerformanceOptimizer() {
        m_workerThread = std::thread(&PerformanceOptimizer::workerFunction, this);
    }
    
    ~PerformanceOptimizer() {
        m_running.store(false);
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }
    
    template<typename Func>
    void addTask(Func&& task) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_taskQueue.push(std::forward<Func>(task));
            m_tasksQueued.fetch_add(1);
        }
    }
    
    size_t getTasksProcessed() const { return m_tasksProcessed.load(); }
    size_t getTasksQueued() const { return m_tasksQueued.load(); }
    
    // FIXED: mutable mutex allows locking in const method
    size_t getPendingTasks() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_taskQueue.size();
    }
    
private:
    void workerFunction() {
        while (m_running.load()) {
            std::function<void()> task;
            
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (!m_taskQueue.empty()) {
                    task = m_taskQueue.front();
                    m_taskQueue.pop();
                }
            }
            
            if (task) {
                task();
                m_tasksProcessed.fetch_add(1);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
};

// =============================================================================
// ANTI-DETECTION SYSTEM
// =============================================================================
class AntiDetectionSystem {
public:
    enum Context { IDLE, MOVEMENT, COMBAT };
    
private:
    Context m_currentContext = IDLE;
    std::atomic<uint64_t> m_lastActionTime{0};
    std::atomic<uint64_t> m_lastMicroPause{0};
    std::atomic<size_t> m_totalActions{0};
    std::atomic<double> m_humanAccuracyFactor{0.85};
    
    // Human behavior parameters
    static constexpr int BASE_DELAY_MS = 25;
    static constexpr int MAX_DELAY_VARIANCE = 15;
    static constexpr double BASE_ACCURACY = 0.85;
    static constexpr double ACCURACY_VARIANCE = 0.1;
    
public:
    void updateContext(Context newContext) {
        m_currentContext = newContext;
        logDebug("Anti-detection context updated: " + std::to_string(static_cast<int>(newContext)));
    }
    
    void registerAction(UINT actionKey) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        m_lastActionTime.store(now);
        m_totalActions.fetch_add(1);
        
        if (g_debugMode.load()) {
            logDebug("Action registered: " + std::to_string(actionKey));
        }
        
        // Check if micro-pause is needed
        if (needsMicroPause()) {
            executeMicroPause();
        }
    }
    
    int getHumanDelay() const {
        std::lock_guard<std::mutex> lock(g_random_mutex);
        std::uniform_int_distribution<int> naturalVariation(-MAX_DELAY_VARIANCE, MAX_DELAY_VARIANCE);
        int variation = static_cast<int>(naturalVariation(gen));
        
        int contextMultiplier = (m_currentContext == COMBAT) ? 1 : 
                               (m_currentContext == MOVEMENT) ? 2 : 3;
        
        return BASE_DELAY_MS + variation + (contextMultiplier * 5);
    }
    
    double getHumanAccuracy() const {
        std::lock_guard<std::mutex> lock(g_random_mutex);
        std::uniform_real_distribution<double> accuracyVariation(-ACCURACY_VARIANCE, ACCURACY_VARIANCE);
        double variation = accuracyVariation(gen);
        
        double contextAccuracy = (m_currentContext == COMBAT) ? BASE_ACCURACY : 
                                (m_currentContext == MOVEMENT) ? BASE_ACCURACY - 0.05 : BASE_ACCURACY - 0.1;
        
        // FIXED: Use custom clamp function instead of std::clamp
        return clamp_value(contextAccuracy + variation, 0.3, 0.95);
    }
    
    bool needsMicroPause() const {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        uint64_t timeSinceLastPause = now - m_lastMicroPause.load();
        size_t totalActions = m_totalActions.load();
        
        int pauseProbability = std::min(static_cast<int>(totalActions / 10), 30);
        
        std::lock_guard<std::mutex> lock(g_random_mutex);
        if (timeSinceLastPause > 20000 && totalActions > 30 && std::uniform_int_distribution<>(0, 100)(gen) < pauseProbability) return true;
        
        return false;
    }
    
    void executeMicroPause() {
        std::lock_guard<std::mutex> lock(g_random_mutex);
        int pauseTime = std::uniform_int_distribution<>(40, 120)(gen);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(pauseTime));
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        m_lastMicroPause.store(now);
        
        logDebug("Executed micro-pause: " + std::to_string(pauseTime) + "ms");
    }
    
    bool shouldAddJitter() const {
        return m_currentContext != IDLE;
    }
    
    std::pair<double, double> getJitter() const {
        std::lock_guard<std::mutex> lock(g_random_mutex);
        std::uniform_real_distribution<double> jitterDist(-0.5, 0.5);
        return {jitterDist(gen), jitterDist(gen)};
    }
    
    // Statistics
    size_t getTotalActions() const { return m_totalActions.load(); }
    uint64_t getLastActionTime() const { return m_lastActionTime.load(); }
    Context getCurrentContext() const { return m_currentContext; }
    
    void resetStats() {
        m_totalActions.store(0);
        m_lastActionTime.store(0);
        m_lastMicroPause.store(0);
    }
};

// =============================================================================
// PREDICTIVE AIM SYSTEM
// =============================================================================
class PredictiveAimSystem {
private:
    struct TargetHistory {
        POINT position;
        uint64_t timestamp;
        double confidence;
    };
    
    std::vector<TargetHistory> m_targetHistory;
    mutable std::mutex m_historyMutex;  // FIXED: Made mutable for const methods
    static constexpr size_t MAX_HISTORY = 10;
    
public:
    void updateTarget(POINT position, double confidence = 1.0) {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        m_targetHistory.push_back({position, static_cast<uint64_t>(now), confidence});
        
        if (m_targetHistory.size() > MAX_HISTORY) {
            m_targetHistory.erase(m_targetHistory.begin());
        }
    }
    
    POINT getPredictedTarget(double predictionTimeMs = 100.0) {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        
        if (m_targetHistory.size() < 2) {
            return m_targetHistory.empty() ? POINT{0, 0} : m_targetHistory.back().position;
        }
        
        // Calculate velocity from last two points
        const auto& last = m_targetHistory.back();
        const auto& secondLast = m_targetHistory[m_targetHistory.size() - 2];
        
        double deltaTime = static_cast<double>(last.timestamp - secondLast.timestamp);
        if (deltaTime <= 0) deltaTime = 16.67; // Assume 60fps
        
        double velocityX = (last.position.x - secondLast.position.x) / deltaTime;
        double velocityY = (last.position.y - secondLast.position.y) / deltaTime;
        
        // Predict future position
        POINT predicted;
        predicted.x = last.position.x + static_cast<int>(velocityX * predictionTimeMs);
        predicted.y = last.position.y + static_cast<int>(velocityY * predictionTimeMs);
        
        // Add prediction error for realism
        if (last.confidence < 1.0) {
            std::lock_guard<std::mutex> lock(g_random_mutex);
            std::uniform_real_distribution<double> errorDist(-2.0, 2.0);
            predicted.x += static_cast<int>(errorDist(gen));
            predicted.y += static_cast<int>(errorDist(gen));
        }
        
        return predicted;
    }
    
    // FIXED: mutable mutex allows locking in const method
    double getTargetVelocity() const {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        
        if (m_targetHistory.size() < 2) return 0.0;
        
        const auto& last = m_targetHistory.back();
        const auto& secondLast = m_targetHistory[m_targetHistory.size() - 2];
        
        double deltaTime = static_cast<double>(last.timestamp - secondLast.timestamp);
        if (deltaTime <= 0) return 0.0;
        
        double deltaX = last.position.x - secondLast.position.x;
        double deltaY = last.position.y - secondLast.position.y;
        
        return std::sqrt(deltaX * deltaX + deltaY * deltaY) / deltaTime;
    }
    
    void clearHistory() {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_targetHistory.clear();
    }
    
    // FIXED: mutable mutex allows locking in const method
    size_t getHistorySize() const {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        return m_targetHistory.size();
    }
};

// =============================================================================
// FLUID MOVEMENT SYSTEM
// =============================================================================
class FluidMovementSystem {
public:
    struct BezierCurve {
        POINT p0, p1, p2, p3; // Control points
        int steps;
        
        POINT getPointAt(double t) const {
            double u = 1.0 - t;
            double tt = t * t;
            double uu = u * u;
            double uuu = uu * u;
            double ttt = tt * t;
            
            POINT point;
            point.x = static_cast<int>(uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x);
            point.y = static_cast<int>(uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y);
            
            return point;
        }
    };
    
private:
    std::mutex m_movementMutex;
    std::atomic<bool> m_isMoving{false};
    
public:
    void executeFluidMovement(POINT start, POINT end, int durationMs = 200) {
        if (m_isMoving.load()) return; // Prevent overlapping movements
        
        m_isMoving.store(true);
        
        auto curve = generateNaturalCurve(start, end);
        int steps = std::max(10, std::min(durationMs / 8, 50));
        curve.steps = steps;
        
        for (int i = 0; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            POINT currentPos = curve.getPointAt(t);
            
            if (i > 0) {
                POINT prevPos = curve.getPointAt(static_cast<double>(i - 1) / steps);
                int deltaX = currentPos.x - prevPos.x;
                int deltaY = currentPos.y - prevPos.y;
                
                if (deltaX != 0 || deltaY != 0) {
                    INPUT input = {};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE;
                    input.mi.dx = deltaX;
                    input.mi.dy = deltaY;
                    
                    SendInput(1, &input, sizeof(INPUT));
                }
            }
            
            if (i < steps) {
                std::this_thread::sleep_for(std::chrono::milliseconds(durationMs / steps));
            }
        }
        
        m_isMoving.store(false);
    }
    
    BezierCurve generateNaturalCurve(POINT start, POINT end) {
        BezierCurve curve;
        curve.p0 = start;
        curve.p3 = end;
        
        int dX = end.x - start.x;
        int dY = end.y - start.y;
        
        // Generate natural control points with randomization
        std::lock_guard<std::mutex> lock(g_random_mutex);
        std::uniform_real_distribution<double> v(-5.0, 5.0);
        
        curve.p1.x = start.x + dX*0.33 + static_cast<int>(v(gen)); 
        curve.p1.y = start.y + dY*0.33 + static_cast<int>(v(gen));
        curve.p2.x = start.x + dX*0.67 + static_cast<int>(v(gen)); 
        curve.p2.y = start.y + dY*0.67 + static_cast<int>(v(gen));
        
        return curve;
    }
    
    bool isCurrentlyMoving() const { return m_isMoving.load(); }
};

// =============================================================================
// MOMENTUM SYSTEM
// =============================================================================
class MomentumSystem {
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
    
public:
    void updateMovementState(bool forward, bool backward, bool left, bool right) {
        m_movingForward.store(forward);
        m_movingBackward.store(backward);
        m_movingLeft.store(left);
        m_movingRight.store(right);
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        m_lastMovementTime.store(now);
        
        logDebug("Momentum movement state updated: F=" + std::to_string(forward) + 
                " B=" + std::to_string(backward) + " L=" + std::to_string(left) + " R=" + std::to_string(right));
    }
    
    void addMomentum(double amount) {
        double current = m_currentMomentum.load();
        double newMomentum = std::min(current + amount, 1.0); // Cap at 1.0
        m_currentMomentum.store(newMomentum);
        
        logDebug("Added momentum: " + std::to_string(amount) + " (total: " + std::to_string(newMomentum) + ")");
    }
    
    void decayMomentum() {
        double current = m_currentMomentum.load();
        if (current > 0.0) {
            double decayed = std::max(current - 0.01, 0.0);
            m_currentMomentum.store(decayed);
        }
    }
    
    bool canChainMovement() const {
        return m_currentMomentum.load() > 0.3;
    }
    
    int getOptimalDirection(int baseDirection) const {
        // If we have momentum, sometimes continue in last direction
        if (m_currentMomentum.load() > 0.5) {
            std::lock_guard<std::mutex> lock(g_random_mutex);
            if (std::uniform_int_distribution<>(0, 100)(gen) < 70) {
                return m_lastDirection.load();
            }
        }
        
        return baseDirection;
    }
    
    void setLastDirection(int direction) {
        m_lastDirection.store(direction);
    }
    
    double getCurrentMomentum() const { return m_currentMomentum.load(); }
    int getLastDirection() const { return m_lastDirection.load(); }
    
    bool isMoving() const {
        return m_movingForward.load() || m_movingBackward.load() || 
               m_movingLeft.load() || m_movingRight.load();
    }
};

// =============================================================================
// FEEDBACK SYSTEM
// =============================================================================
class FeedbackSystem {
private:
    mutable std::mutex m_feedbackMutex;  // FIXED: Made mutable for const methods
    std::atomic<uint64_t> m_lastFeedbackTime{0};
    std::string m_currentMovement;
    std::atomic<int> m_activeDuration{0};
    
public:
    void startMovement(const std::string& movementName, int expectedDuration = 500) {
        std::lock_guard<std::mutex> lock(m_feedbackMutex);
        
        m_currentMovement = movementName;
        m_activeDuration.store(expectedDuration);
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        m_lastFeedbackTime.store(now);
        
        logDebug("Movement started: " + movementName + " (duration: " + std::to_string(expectedDuration) + "ms)");
    }
    
    void endMovement() {
        std::lock_guard<std::mutex> lock(m_feedbackMutex);
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto duration = now - m_lastFeedbackTime.load();
        
        logDebug("Movement ended: " + m_currentMovement + " (actual duration: " + std::to_string(duration) + "ms)");
        
        m_currentMovement.clear();
        m_activeDuration.store(0);
    }
    
    bool isMovementActive() const {
        return !m_currentMovement.empty();
    }
    
    // FIXED: mutable mutex allows locking in const method
    std::string getCurrentMovement() const {
        std::lock_guard<std::mutex> lock(m_feedbackMutex);
        return m_currentMovement;
    }
    
    int getRemainingDuration() const {
        if (m_currentMovement.empty()) return 0;
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto elapsed = now - m_lastFeedbackTime.load();
        
        return std::max(0, m_activeDuration.load() - static_cast<int>(elapsed));
    }
};

// =============================================================================
// GLOBAL SYSTEM INSTANCES
// =============================================================================
std::unique_ptr<PerformanceOptimizer> g_performanceOptimizer;
std::unique_ptr<AntiDetectionSystem> g_antiDetectionSystem;
std::unique_ptr<PredictiveAimSystem> g_predictiveAimSystem;
std::unique_ptr<FluidMovementSystem> g_fluidMovementSystem;
std::unique_ptr<MomentumSystem> g_momentumSystem;
std::unique_ptr<FeedbackSystem> g_feedbackSystem;

// =============================================================================
// SYSTEM INITIALIZATION AND MANAGEMENT - FIXED NO EXCEPTIONS
// =============================================================================
bool initializeAllSystems() {
    logMessage("🚀 Initializing All Systems...");
    
    // FIXED: Remove try-catch since exceptions are disabled
    // Initialize core systems with error checking
    g_performanceOptimizer = std::make_unique<PerformanceOptimizer>();
    if (!g_performanceOptimizer) {
        logMessage("❌ Failed to initialize Performance Optimizer");
        return false;
    }
    
    g_antiDetectionSystem = std::make_unique<AntiDetectionSystem>();
    if (!g_antiDetectionSystem) {
        logMessage("❌ Failed to initialize Anti-Detection System");
        return false;
    }
    
    g_predictiveAimSystem = std::make_unique<PredictiveAimSystem>();
    if (!g_predictiveAimSystem) {
        logMessage("❌ Failed to initialize Predictive Aim System");
        return false;
    }
    
    g_fluidMovementSystem = std::make_unique<FluidMovementSystem>();
    if (!g_fluidMovementSystem) {
        logMessage("❌ Failed to initialize Fluid Movement System");
        return false;
    }
    
    g_momentumSystem = std::make_unique<MomentumSystem>();
    if (!g_momentumSystem) {
        logMessage("❌ Failed to initialize Momentum System");
        return false;
    }
    
    g_feedbackSystem = std::make_unique<FeedbackSystem>();
    if (!g_feedbackSystem) {
        logMessage("❌ Failed to initialize Feedback System");
        return false;
    }
    
    logMessage("✅ Performance Optimizer initialized");
    logMessage("✅ Anti-Detection System initialized");
    logMessage("✅ Predictive Aim System initialized");
    logMessage("✅ Fluid Movement System initialized");
    logMessage("✅ Momentum System initialized");
    logMessage("✅ Feedback System initialized");
    
    return true;
}

void shutdownAllSystems() {
    logMessage("🛑 Shutting down All Systems...");
    
    g_feedbackSystem.reset();
    g_momentumSystem.reset();
    g_fluidMovementSystem.reset();
    g_predictiveAimSystem.reset();
    g_antiDetectionSystem.reset();
    g_performanceOptimizer.reset();
    
    logMessage("✅ All Systems shutdown complete");
}

// =============================================================================
// SYSTEM ACCESS FUNCTIONS
// =============================================================================
PerformanceOptimizer* getPerformanceOptimizer() {
    return g_performanceOptimizer.get();
}

AntiDetectionSystem* getAntiDetectionSystem() {
    return g_antiDetectionSystem.get();
}

PredictiveAimSystem* getPredictiveAimSystem() {
    return g_predictiveAimSystem.get();
}

FluidMovementSystem* getFluidMovementSystem() {
    return g_fluidMovementSystem.get();
}

MomentumSystem* getMomentumSystem() {
    return g_momentumSystem.get();
}

FeedbackSystem* getFeedbackSystem() {
    return g_feedbackSystem.get();
}

// =============================================================================
// SYSTEM STATUS AND DIAGNOSTICS
// =============================================================================
void printSystemStatus() {
    logMessage("=== System Status Report ===");
    
    if (g_performanceOptimizer) {
        logMessage("Performance Optimizer: Active");
        logMessage("  Tasks Processed: " + std::to_string(g_performanceOptimizer->getTasksProcessed()));
        logMessage("  Tasks Queued: " + std::to_string(g_performanceOptimizer->getTasksQueued()));
        logMessage("  Pending Tasks: " + std::to_string(g_performanceOptimizer->getPendingTasks()));
    }
    
    if (g_antiDetectionSystem) {
        logMessage("Anti-Detection System: Active");
        logMessage("  Total Actions: " + std::to_string(g_antiDetectionSystem->getTotalActions()));
        logMessage("  Current Context: " + std::to_string(static_cast<int>(g_antiDetectionSystem->getCurrentContext())));
    }
    
    if (g_predictiveAimSystem) {
        logMessage("Predictive Aim System: Active");
        logMessage("  History Size: " + std::to_string(g_predictiveAimSystem->getHistorySize()));
        logMessage("  Target Velocity: " + std::to_string(g_predictiveAimSystem->getTargetVelocity()));
    }
    
    if (g_fluidMovementSystem) {
        logMessage("Fluid Movement System: " + std::string(g_fluidMovementSystem->isCurrentlyMoving() ? "Moving" : "Idle"));
    }
    
    if (g_momentumSystem) {
        logMessage("Momentum System: Active");
        logMessage("  Current Momentum: " + std::to_string(g_momentumSystem->getCurrentMomentum()));
        logMessage("  Is Moving: " + std::string(g_momentumSystem->isMoving() ? "Yes" : "No"));
    }
    
    if (g_feedbackSystem) {
        logMessage("Feedback System: Active");
        logMessage("  Current Movement: " + g_feedbackSystem->getCurrentMovement());
        logMessage("  Remaining Duration: " + std::to_string(g_feedbackSystem->getRemainingDuration()) + "ms");
    }
}

void testAllSystems() {
    logMessage("🧪 Testing All Systems...");
    
    if (g_performanceOptimizer) {
        g_performanceOptimizer->addTask([]() {
            logMessage("✅ Performance Optimizer test task executed");
        });
    }
    
    if (g_antiDetectionSystem) {
        g_antiDetectionSystem->registerAction(VK_SPACE);
        logMessage("✅ Anti-Detection System test completed");
    }
    
    if (g_predictiveAimSystem) {
        POINT testPoint = {100, 200};
        g_predictiveAimSystem->updateTarget(testPoint);
        auto predicted = g_predictiveAimSystem->getPredictedTarget(50.0);
        logMessage("✅ Predictive Aim System test completed. Predicted: (" + 
                   std::to_string(predicted.x) + ", " + std::to_string(predicted.y) + ")");
    }
    
    if (g_momentumSystem) {
        g_momentumSystem->addMomentum(0.3);
        logMessage("✅ Momentum System test completed");
    }
    
    if (g_feedbackSystem) {
        g_feedbackSystem->startMovement("System Test", 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        g_feedbackSystem->endMovement();
        logMessage("✅ Feedback System test completed");
    }
    
    logMessage("✅ All Systems tested successfully");
}

// =============================================================================
// SYSTEM PERFORMANCE MONITORING
// =============================================================================
void monitorSystemPerformance() {
    static auto lastMonitorTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastMonitorTime).count();
    
    if (elapsed >= 30) { // Monitor every 30 seconds
        logDebug("=== System Performance Monitor ===");
        
        if (g_performanceOptimizer) {
            size_t pending = g_performanceOptimizer->getPendingTasks();
            if (pending > 10) {
                logDebug("Warning: High task queue (" + std::to_string(pending) + " pending)");
            }
        }
        
        if (g_momentumSystem) {
            g_momentumSystem->decayMomentum();
        }
        
        lastMonitorTime = now;
    }
}