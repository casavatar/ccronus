// --------------------------------------------------------------------------------------
// description: Core systems implementation - Fixed all compilation issues
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------
// version: 3.0.4 - Fixed all compilation errors
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "systems.h"
#include "state_manager.h"
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
#include <algorithm>
#include <cctype>

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
// GLOBAL RANDOM GENERATOR
// =============================================================================
std::mutex g_random_mutex;
thread_local std::mt19937 gen(std::random_device{}());

// =============================================================================
// PERFORMANCE OPTIMIZER IMPLEMENTATION
// =============================================================================

PerformanceOptimizer::PerformanceOptimizer() {
    m_workerThread = std::thread(&PerformanceOptimizer::workerFunction, this);
}

PerformanceOptimizer::~PerformanceOptimizer() {
    m_running.store(false);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void PerformanceOptimizer::addTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(task));
        m_tasksQueued.fetch_add(1);
    }
}

size_t PerformanceOptimizer::getTasksProcessed() const { 
    return m_tasksProcessed.load(); 
}

size_t PerformanceOptimizer::getTasksQueued() const { 
    return m_tasksQueued.load(); 
}

size_t PerformanceOptimizer::getPendingTasks() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

void PerformanceOptimizer::workerFunction() {
    while (m_running.load()) {
        std::function<void()> task;
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_taskQueue.empty()) {
                task = std::move(m_taskQueue.front());
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

// =============================================================================
// ANTI-DETECTION SYSTEM IMPLEMENTATION
// =============================================================================

AntiDetectionSystem::AntiDetectionSystem(StateManager& stateManager) 
    : m_stateManager(stateManager), m_currentContext(IDLE) {
    // Initialize anti-detection system
}

void AntiDetectionSystem::updateContext(Context newContext) {
    m_currentContext = newContext;
    logDebug("Anti-detection context updated: " + std::to_string(static_cast<int>(newContext)));
}

void AntiDetectionSystem::registerAction(UINT actionKey) {
    (void)actionKey; // Suppress unused parameter warning
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    m_lastActionTime.store(now);
    m_totalActions.fetch_add(1);
    
    // Check if micro-pause is needed
    if (needsMicroPause()) {
        executeMicroPause();
    }
}

int AntiDetectionSystem::getHumanDelay() const {
    std::lock_guard<std::mutex> lock(g_random_mutex);
    std::uniform_int_distribution<int> naturalVariation(-15, 15);
    int variation = naturalVariation(gen);
    
    int contextMultiplier = (m_currentContext == COMBAT) ? 1 : 
                           (m_currentContext == MOVEMENT) ? 2 : 3;
    
    return 25 + variation + (contextMultiplier * 5);
}

double AntiDetectionSystem::getHumanAccuracy() const {
    std::lock_guard<std::mutex> lock(g_random_mutex);
    std::uniform_real_distribution<double> accuracyVariation(-0.1, 0.1);
    double variation = accuracyVariation(gen);
    
    double contextAccuracy = (m_currentContext == COMBAT) ? 0.85 : 
                            (m_currentContext == MOVEMENT) ? 0.80 : 0.75;
    
    return clamp_value(contextAccuracy + variation, 0.3, 0.95);
}

bool AntiDetectionSystem::needsMicroPause() const {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    uint64_t timeSinceLastPause = now - m_lastMicroPause.load();
    size_t totalActions = m_totalActions.load();
    
    int pauseProbability = std::min(static_cast<int>(totalActions / 10), 30);
    
    std::lock_guard<std::mutex> lock(g_random_mutex);
    if (timeSinceLastPause > 20000 && totalActions > 30 && std::uniform_int_distribution<>(0, 100)(gen) < pauseProbability) return true;
    
    return false;
}

void AntiDetectionSystem::executeMicroPause() {
    std::lock_guard<std::mutex> lock(g_random_mutex);
    int pauseTime = std::uniform_int_distribution<>(40, 120)(gen);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(pauseTime));
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    m_lastMicroPause.store(now);
    
    logDebug("Executed micro-pause: " + std::to_string(pauseTime) + "ms");
}

bool AntiDetectionSystem::shouldAddJitter() const {
    return m_currentContext != IDLE;
}

std::pair<double, double> AntiDetectionSystem::getJitter() const {
    std::lock_guard<std::mutex> lock(g_random_mutex);
    std::uniform_real_distribution<double> jitterDist(-0.5, 0.5);
    return {jitterDist(gen), jitterDist(gen)};
}

size_t AntiDetectionSystem::getTotalActions() const { 
    return m_totalActions.load(); 
}

uint64_t AntiDetectionSystem::getLastActionTime() const { 
    return m_lastActionTime.load(); 
}

AntiDetectionSystem::Context AntiDetectionSystem::getCurrentContext() const { 
    return m_currentContext; 
}

void AntiDetectionSystem::resetStats() {
    m_totalActions.store(0);
    m_lastActionTime.store(0);
    m_lastMicroPause.store(0);
}

// =============================================================================
// PREDICTIVE AIM SYSTEM IMPLEMENTATION
// =============================================================================

void PredictiveAimSystem::updateTarget(POINT position, double confidence) {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    m_targetHistory.push_back({position, static_cast<uint64_t>(now), confidence});
    
    if (m_targetHistory.size() > MAX_HISTORY) {
        m_targetHistory.erase(m_targetHistory.begin());
    }
}

POINT PredictiveAimSystem::getPredictedTarget(double predictionTimeMs) {
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

double PredictiveAimSystem::getTargetVelocity() const {
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

void PredictiveAimSystem::clearHistory() {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    m_targetHistory.clear();
}

size_t PredictiveAimSystem::getHistorySize() const {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    return m_targetHistory.size();
}

// =============================================================================
// FLUID MOVEMENT SYSTEM IMPLEMENTATION
// =============================================================================

void FluidMovementSystem::executeFluidMovement(POINT start, POINT end, int durationMs) {
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

POINT FluidMovementSystem::BezierCurve::getPointAt(double t) const {
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

FluidMovementSystem::BezierCurve FluidMovementSystem::generateNaturalCurve(POINT start, POINT end) {
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

bool FluidMovementSystem::isCurrentlyMoving() const { 
    return m_isMoving.load(); 
}

// =============================================================================
// MOMENTUM SYSTEM IMPLEMENTATION
// =============================================================================

void MomentumSystem::updateMovementState(bool forward, bool backward, bool left, bool right) {
    m_movingForward.store(forward);
    m_movingBackward.store(backward);
    m_movingLeft.store(left);
    m_movingRight.store(right);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    m_lastMovementTime.store(now);
}

void MomentumSystem::addMomentum(double amount) {
    double current = m_currentMomentum.load();
    double newMomentum = std::min(current + amount, 1.0); // Cap at 1.0
    m_currentMomentum.store(newMomentum);
}

void MomentumSystem::decayMomentum() {
    double current = m_currentMomentum.load();
    if (current > 0.0) {
        double decayed = std::max(current - 0.01, 0.0);
        m_currentMomentum.store(decayed);
    }
}

bool MomentumSystem::canChainMovement() const {
    return m_currentMomentum.load() > 0.3;
}

int MomentumSystem::getOptimalDirection(int baseDirection) const {
    // If we have momentum, sometimes continue in last direction
    if (m_currentMomentum.load() > 0.5) {
        std::lock_guard<std::mutex> lock(g_random_mutex);
        if (std::uniform_int_distribution<>(0, 100)(gen) < 70) {
            return m_lastDirection.load();
        }
    }
    
    return baseDirection;
}

void MomentumSystem::setLastDirection(int direction) {
    m_lastDirection.store(direction);
}

double MomentumSystem::getCurrentMomentum() const { 
    return m_currentMomentum.load(); 
}

int MomentumSystem::getLastDirection() const { 
    return m_lastDirection.load(); 
}

bool MomentumSystem::isMoving() const {
    return m_movingForward.load() || m_movingBackward.load() || 
           m_movingLeft.load() || m_movingRight.load();
}

// =============================================================================
// FEEDBACK SYSTEM IMPLEMENTATION
// =============================================================================

void FeedbackSystem::startMovement(const std::string& movementName, int expectedDuration) {
    std::lock_guard<std::mutex> lock(m_feedbackMutex);
    
    m_currentMovement = movementName;
    m_activeDuration.store(expectedDuration);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    m_lastFeedbackTime.store(now);
}

void FeedbackSystem::endMovement() {
    std::lock_guard<std::mutex> lock(m_feedbackMutex);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    (void)(now - m_lastFeedbackTime.load()); // Suppress unused variable warning
    
    m_currentMovement.clear();
    m_activeDuration.store(0);
}

bool FeedbackSystem::isMovementActive() const {
    return !m_currentMovement.empty();
}

std::string FeedbackSystem::getCurrentMovement() const {
    std::lock_guard<std::mutex> lock(m_feedbackMutex);
    return m_currentMovement;
}

int FeedbackSystem::getRemainingDuration() const {
    if (m_currentMovement.empty()) return 0;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    auto elapsed = now - m_lastFeedbackTime.load();
    
    return std::max(0, m_activeDuration.load() - static_cast<int>(elapsed));
}

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
// SYSTEM INITIALIZATION AND MANAGEMENT
// =============================================================================
bool initializeAllSystems() {
    logMessage("🚀 Initializing All Systems...");
    
    // Initialize core systems with error checking
    g_performanceOptimizer = std::make_unique<PerformanceOptimizer>();
    if (!g_performanceOptimizer) {
        logMessage("❌ Failed to initialize Performance Optimizer");
        return false;
    }
    
    g_antiDetectionSystem = std::make_unique<AntiDetectionSystem>(*getStateManager());
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
        double velocity = g_predictiveAimSystem->getTargetVelocity();
        logMessage("  Target Velocity: " + std::to_string(velocity));
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