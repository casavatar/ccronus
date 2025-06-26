// description: This code implements various systems for a game, including performance optimization, PID control, and anti-detection mechanisms. It uses multithreading for performance tasks and provides a structured approach to managing game systems.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist

#include "systems.h"
#include "globals.h"
#include <numeric>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

// --- Implementación de PerformanceOptimizer ---
PerformanceOptimizer::PerformanceOptimizer() {
    unsigned int numThreads = std::max(2u, std::thread::hardware_concurrency() / 2);
    if (numThreads == 0) numThreads = 1;

    for (unsigned int i = 0; i < numThreads; ++i) {
        workerThreads.emplace_back(&PerformanceOptimizer::workerFunction, this);
    }
    // Iniciar el hilo de monitoreo
    workerThreads.emplace_back(&PerformanceOptimizer::performanceMonitor, this);
}

PerformanceOptimizer::~PerformanceOptimizer() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        shouldStop = true;
    }
    queueCondition.notify_all();
    for (std::thread &worker : workerThreads) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}

void PerformanceOptimizer::workerFunction() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] { return !taskQueue.empty() || shouldStop; });
            if (shouldStop && taskQueue.empty()) return;
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }
        try {
            task();
        } catch (const std::exception& e) {
            logMessage("Error in worker thread task: " + std::string(e.what()));
        }
    }
}

void PerformanceOptimizer::addTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (shouldStop) return;
        taskQueue.push(std::move(task));
    }
    queueCondition.notify_one();
}

// --- NUEVAS IMPLEMENTACIONES PARA ANALYTICS ---
void PerformanceOptimizer::performanceMonitor() {
    auto lastTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    while (!shouldStop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        if (duration > 0 && frameCount > 0) {
            avgFrameTime = static_cast<double>(duration) / frameCount;
        }
        lastTime = now;
        // La cuenta de frames se podría hacer en el bucle principal si fuera un juego,
        // pero para este caso, monitoreamos los eventos de input.
    }
}

void PerformanceOptimizer::registerInputEvent() {
    totalInputEvents++;
}

std::string PerformanceOptimizer::getPerformanceMetrics() {
    std::ostringstream metrics;
    // Como no tenemos un bucle de renderizado, FPS no es una métrica directa.
    // Usaremos "Events/Sec" en su lugar.
    static int lastEventCount = 0;
    static auto lastTime = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now-lastTime).count();
    
    int currentEvents = totalInputEvents.load();
    double eps = 0.0;
    if (duration > 0) {
        eps = static_cast<double>(currentEvents - lastEventCount) / duration;
    }

    if(duration > 2) { // Reset cada 2 segundos
        lastEventCount = currentEvents;
        lastTime = now;
    }
    
    metrics << "EPS: " << std::fixed << std::setprecision(1) << eps;
    return metrics.str();
}
// --- Implementación de TacticalPIDController ---
TacticalPIDController::TacticalPIDController(double p, double i, double d) : kp(p), ki(i), kd(d), prev_error(0.0), integral(0.0) {
    last_time = std::chrono::steady_clock::now();
}
void TacticalPIDController::reset() {
    std::lock_guard<std::mutex> lock(pidMutex);
    prev_error = 0.0; integral = 0.0;
    last_time = std::chrono::steady_clock::now();
}
double TacticalPIDController::calculate(double current_error) {
    std::lock_guard<std::mutex> lock(pidMutex);
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration_cast<std::chrono::microseconds>(now - last_time).count() / 1000000.0;
    last_time = now;
    integral += current_error * dt;
    integral = std::max(-integralLimit, std::min(integralLimit, integral));
    double derivative = (dt > 0) ? (current_error - prev_error) / dt : 0.0;
    prev_error = current_error;
    double output = kp * current_error + ki * integral + kd * derivative;
    return std::max(-outputLimit, std::min(outputLimit, output));
}
void TacticalPIDController::updateParams(double p, double i, double d) {
    std::lock_guard<std::mutex> lock(pidMutex);
    kp = p; ki = i; kd = d;
}

// --- Implementación de AdaptiveSmoothingSystem ---
AdaptiveSmoothingSystem::AdaptiveSmoothingSystem() {
    currentProfile = {0.85, 0.65, 0.95, 0.15};
    currentSmoothing = currentProfile.baseSmoothing;
    lastUpdate = std::chrono::steady_clock::now();
}
double AdaptiveSmoothingSystem::getSmoothingFactor(bool inCombat, bool requiresPrecision) {
    auto now = std::chrono::steady_clock::now();
    double deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() / 1000.0;
    lastUpdate = now;
    double targetSmoothing = currentProfile.baseSmoothing;
    if (requiresPrecision) targetSmoothing = currentProfile.precisionSmoothing;
    else if (inCombat) targetSmoothing = currentProfile.combatSmoothing;
    currentSmoothing += (targetSmoothing - currentSmoothing) * currentProfile.transitionSpeed * deltaTime;
    currentSmoothing = std::max(0.5, std::min(1.0, currentSmoothing));
    return currentSmoothing;
}
void AdaptiveSmoothingSystem::updateProfile(double base, double combat, double precision) {
    currentProfile.baseSmoothing = base;
    currentProfile.combatSmoothing = combat;
    currentProfile.precisionSmoothing = precision;
}

// --- Implementación de AntiDetectionSystem ---
AntiDetectionSystem::AntiDetectionSystem() {
    humanDelayPatterns = {8, 10, 7, 12, 9, 11, 8, 13, 6, 11, 14, 7, 15, 6};
    humanAccuracyPatterns = {0.88, 0.85, 0.90, 0.82, 0.87, 0.89, 0.84, 0.91};
    sessionStartTime = std::chrono::steady_clock::now();
    lastMicroPause = sessionStartTime;
}
void AntiDetectionSystem::updateContext(Context newContext) { currentContext = newContext; }
int AntiDetectionSystem::getHumanDelay() {
    int baseDelay = humanDelayPatterns[totalActions % humanDelayPatterns.size()];
    switch (currentContext) {
        case COMBAT: baseDelay = static_cast<int>(baseDelay * 0.8); break;
        case AIMING: baseDelay = static_cast<int>(baseDelay * 1.2); break;
        case MOVEMENT: baseDelay = static_cast<int>(baseDelay * 0.9); break;
        default: break;
    }
    double fatigueMultiplier = 1.0 + (fatigueLevel * 0.15);
    std::normal_distribution<double> naturalVariation(0.0, 1.0);
    int variation = static_cast<int>(naturalVariation(gen));
    int finalDelay = static_cast<int>((baseDelay + variation) * fatigueMultiplier);
    return std::max(5, std::min(20, finalDelay));
}
double AntiDetectionSystem::getHumanAccuracy() {
    double baseAccuracy = humanAccuracyPatterns[totalActions % humanAccuracyPatterns.size()];
    switch (currentContext) {
        case AIMING: baseAccuracy += 0.05; break;
        case COMBAT: baseAccuracy -= 0.03; break;
        default: break;
    }
    double fatigueReduction = fatigueLevel * 0.05;
    std::normal_distribution<double> accuracyVariation(0.0, 0.015);
    double variation = accuracyVariation(gen);
    return std::max(0.80, std::min(0.98, baseAccuracy - fatigueReduction + variation));
}
bool AntiDetectionSystem::needsMicroPause() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastPause = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMicroPause).count();
    int pauseProbability = 8 + static_cast<int>(fatigueLevel * 5);
    if (timeSinceLastPause > 20000 && totalActions > 30 && std::uniform_int_distribution<>(0, 100)(gen) < pauseProbability) return true;
    return false;
}
void AntiDetectionSystem::executeMicroPause() {
    if (isInMicroPause) return;
    isInMicroPause = true;
    lastMicroPause = std::chrono::steady_clock::now();
    int pauseTime = std::uniform_int_distribution<>(40, 120)(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(pauseTime));
    fatigueLevel = std::max(0.0, fatigueLevel - 0.02);
    isInMicroPause = false;
}
void AntiDetectionSystem::registerAction(int actionType) {
    actionHistory.push_back({actionType, std::chrono::steady_clock::now()});
    if (actionHistory.size() > 100) actionHistory.pop_front();
    totalActions++;
    updateFatigue();
}
void AntiDetectionSystem::updateFatigue() {
    auto sessionDuration = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - sessionStartTime).count();
    fatigueLevel = std::min(1.0, (sessionDuration * 0.01) + (totalActions * 0.00005));
}

// --- Implementación de PredictiveAimSystem ---
PredictiveAimSystem::PredictiveAimSystem() {
    currentPrediction.confidence = 0.0;
    currentPrediction.estimatedPosition = {0, 0};
    currentPrediction.velocity = {0, 0};
    currentPrediction.acceleration = {0, 0};
    GetCursorPos(&lastCursorPosForUserVelocity);
}
void PredictiveAimSystem::updateCursorHistory() {
    POINT current; GetCursorPos(&current);
    auto now = std::chrono::steady_clock::now();
    estimatedState.x = static_cast<long>(estimatedState.x + kalmanGain * (current.x - estimatedState.x));
    estimatedState.y = static_cast<long>(estimatedState.y + kalmanGain * (current.y - estimatedState.y));
    cursorHistory.push_back({estimatedState, now});
    if (cursorHistory.size() > static_cast<size_t>(maxHistorySize)) cursorHistory.pop_front();
    userMouseDeltaHistory.push_back({current, now});
    if (userMouseDeltaHistory.size() > static_cast<size_t>(userMouseDeltaHistorySize)) userMouseDeltaHistory.pop_front();
    updatePrediction();
    calculateUserMouseVelocity();
}
POINT PredictiveAimSystem::getPredictedTarget(double humanAccuracy) {
    if (currentPrediction.confidence < 0.2) { POINT current; GetCursorPos(&current); return current; }
    POINT predicted = currentPrediction.estimatedPosition;
    if (humanAccuracy < 1.0) {
        double distance = sqrt(pow(static_cast<double>(predicted.x) - SCREEN_CENTER_X, 2) + pow(static_cast<double>(predicted.y) - SCREEN_CENTER_Y, 2));
        double distanceFactor = std::min(1.0, distance / 500.0);
        std::normal_distribution<double> errorDist(0.0, (1.0 - humanAccuracy) * 10.0 * distanceFactor);
        predicted.x += static_cast<int>(errorDist(gen));
        predicted.y += static_cast<int>(errorDist(gen));
    }
    return predicted;
}
double PredictiveAimSystem::getPredictionConfidence() { return currentPrediction.confidence; }
POINT PredictiveAimSystem::getTargetVelocity() { return currentPrediction.velocity; }
double PredictiveAimSystem::getUserMouseVelocity() const { return currentUserMouseVelocity; }
POINT PredictiveAimSystem::getRawMouseDelta() {
    POINT current; GetCursorPos(&current);
    POINT delta = {current.x - lastCursorPosForUserVelocity.x, current.y - lastCursorPosForUserVelocity.y};
    lastCursorPosForUserVelocity = current;
    return delta;
}
void PredictiveAimSystem::updatePrediction() {
    if (cursorHistory.size() < 3) { currentPrediction.confidence = 0.0; return; }
    POINT avgVelocity = calculateAverageVelocity();
    POINT acceleration = calculateAcceleration();
    auto latest = cursorHistory.back();
    POINT currentPos = latest.first;
    double timeAhead = (predictionTimeMs + networkLatencyMs) / 1000.0;
    currentPrediction.estimatedPosition.x = currentPos.x + static_cast<int>(avgVelocity.x * timeAhead + 0.5 * acceleration.x * timeAhead * timeAhead);
    currentPrediction.estimatedPosition.y = currentPos.y + static_cast<int>(avgVelocity.y * timeAhead + 0.5 * acceleration.y * timeAhead * timeAhead);
    currentPrediction.velocity = avgVelocity;
    currentPrediction.acceleration = acceleration;
    currentPrediction.timestamp = std::chrono::steady_clock::now();
    currentPrediction.confidence = calculateConfidence();
}
void PredictiveAimSystem::calculateUserMouseVelocity() {
    if (userMouseDeltaHistory.size() < 2) { currentUserMouseVelocity = 0.0; return; }
    double totalDistance = 0.0;
    for (size_t i = 1; i < userMouseDeltaHistory.size(); ++i) {
        POINT p1 = userMouseDeltaHistory[i-1].first; POINT p2 = userMouseDeltaHistory[i].first;
        totalDistance += sqrt(pow(static_cast<double>(p2.x) - p1.x, 2) + pow(static_cast<double>(p2.y) - p1.y, 2));
    }
    double totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(userMouseDeltaHistory.back().second - userMouseDeltaHistory.front().second).count();
    currentUserMouseVelocity = (totalTimeMs > 0) ? (totalDistance / totalTimeMs) : 0.0;
}
POINT PredictiveAimSystem::calculateAverageVelocity() { /* Paste implementation */ POINT p = {0,0}; return p; }
POINT PredictiveAimSystem::calculateAcceleration() { /* Paste implementation */ POINT p = {0,0}; return p; }
double PredictiveAimSystem::calculateConfidence() { /* Paste implementation */ return 0.0; }
double PredictiveAimSystem::calculateSpeedConsistency() { /* Paste implementation */ return 0.0; }
double PredictiveAimSystem::calculateDirectionConsistency() { /* Paste implementation */ return 0.0; }

// --- Implementación de MomentumSystem ---
MomentumSystem::MomentumSystem() { state.lastMovement = std::chrono::steady_clock::now(); }
void MomentumSystem::updateMovementState(bool sprint, bool slide, bool dive, bool air) {
    state.isInSprint = sprint; state.isSliding = slide; state.isDiving = dive; state.isInAir = air;
    state.lastMovement = std::chrono::steady_clock::now();
}
double MomentumSystem::getCurrentMomentum() {
    auto timeSince = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - state.lastMovement).count();
    double decay = std::max(0.0, 1.0 - (timeSince / 2000.0));
    return state.momentum * decay;
}
void MomentumSystem::addMomentum(double amount) { state.momentum = std::min(1.0, state.momentum + amount); }
bool MomentumSystem::canChainMovement() { return getCurrentMomentum() > 0.3; }
POINT MomentumSystem::getOptimalDirection(POINT targetDirection) {
    double momentum = getCurrentMomentum();
    if (momentum > 0.5) {
        POINT result;
        result.x = static_cast<int>((targetDirection.x * 0.7) + (state.lastDirection.x * 0.3));
        result.y = static_cast<int>((targetDirection.y * 0.7) + (state.lastDirection.y * 0.3));
        return result;
    }
    return targetDirection;
}
void MomentumSystem::setLastDirection(POINT direction) { state.lastDirection = direction; }

// --- Implementación de VisualFeedbackSystem ---
void VisualFeedbackSystem::startMovement(const std::string& name, int duration) { logMessage("Starting: " + name); feedback.currentMovement = name; }
void VisualFeedbackSystem::updateProgress(int percent) {
    if(percent % 25 == 0) logMessage(feedback.currentMovement + "..." + std::to_string(percent) + "%");
}
void VisualFeedbackSystem::finishMovement(bool success) { logMessage("Finished: " + feedback.currentMovement); }
void VisualFeedbackSystem::logDirectionalHint(const std::string& dir, const std::string& effect) { logMessage(dir + " -> " + effect); }

// --- Implementación de FluidMovementSystem ---
POINT FluidMovementSystem::interpolateCubicBezier(const BezierCurve& curve, double t) {
    double u = 1.0 - t, tt = t * t, uu = u * u;
    POINT p;
    p.x = static_cast<long>(uu*u*curve.p0.x + 3*uu*t*curve.p1.x + 3*u*tt*curve.p2.x + tt*t*curve.p3.x);
    p.y = static_cast<long>(uu*u*curve.p0.y + 3*uu*t*curve.p1.y + 3*u*tt*curve.p2.y + tt*t*curve.p3.y);
    return p;
}
FluidMovementSystem::BezierCurve FluidMovementSystem::generateNaturalCurve(POINT start, POINT end) {
    BezierCurve curve; curve.p0 = start; curve.p3 = end;
    int dX = end.x - start.x, dY = end.y - start.y;
    double dist = sqrt(static_cast<double>(dX)*dX + static_cast<double>(dY)*dY);
    double cF = std::min(0.3, 20.0/dist);
    std::normal_distribution<double> v(0.0, dist*cF*0.5);
    curve.p1.x = start.x + dX*0.33 + static_cast<int>(v(gen)); curve.p1.y = start.y + dY*0.33 + static_cast<int>(v(gen));
    curve.p2.x = start.x + dX*0.67 + static_cast<int>(v(gen)); curve.p2.y = start.y + dY*0.67 + static_cast<int>(v(gen));
    return curve;
}
double FluidMovementSystem::getVelocityProfile(double progress) {
    if (progress < 0.5) return 2.0 * progress * progress;
    return 1.0 - pow(-2.0 * progress + 2.0, 2.0) / 2.0;
}
std::vector<POINT> FluidMovementSystem::generateMovementPath(POINT start, POINT end, int steps) {
    std::vector<POINT> path; BezierCurve curve = generateNaturalCurve(start, end);
    POINT lastPoint = start;
    for (size_t i = 0; i <= static_cast<size_t>(steps); ++i) {
        double rawProgress = static_cast<double>(i) / steps;
        POINT currentPoint = interpolateCubicBezier(curve, getVelocityProfile(rawProgress));
        POINT delta = {currentPoint.x - lastPoint.x, currentPoint.y - lastPoint.y};
        path.push_back(delta); lastPoint = currentPoint;
    }
    return path;
}