// description: This header file defines various systems for a tactical game bot, including PID control, adaptive smoothing, anti-detection mechanisms, predictive aiming, performance optimization, momentum handling, visual feedback, fluid movement, and headshot prioritization.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <memory>
#include <random>

// ... (Otras declaraciones de clases sin cambios) ...

// --- Performance Optimizer (Thread Pool) ---
class PerformanceOptimizer {
private:
    std::vector<std::thread> workerThreads;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> shouldStop{false};

    std::atomic<double> avgFrameTime{16.67};
    std::atomic<int> totalInputEvents{0};
    
    void workerFunction();
    void performanceMonitor(); // Hilo para monitorear el rendimiento

public:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    void addTask(std::function<void()> task);

    std::string getPerformanceMetrics();
    void registerInputEvent();
};

// PID Controller
class TacticalPIDController {
public:
    TacticalPIDController(double p, double i, double d);
    void reset();
    double calculate(double current_error);
    void updateParams(double p, double i, double d);

private:
    double kp, ki, kd;
    double prev_error, integral;
    std::chrono::steady_clock::time_point last_time;
    std::mutex pidMutex;
    double integralLimit = 100.0, outputLimit = 50.0;
};

// Adaptive Smoothing System
class AdaptiveSmoothingSystem {
private:
    struct SmoothingProfile { double baseSmoothing, combatSmoothing, precisionSmoothing, transitionSpeed; };
    SmoothingProfile currentProfile;
    double currentSmoothing;
    std::chrono::steady_clock::time_point lastUpdate;
public:
    AdaptiveSmoothingSystem();
    // Now considers target velocity for dynamic adjustments
    double getSmoothingFactor(bool inCombat, bool requiresPrecision, double target_velocity);
    void updateProfile(double base, double combat, double precision);
};

// Anti-Detection System
class AntiDetectionSystem {
public:
    enum Context { IDLE, AIMING, COMBAT, MOVEMENT };
    AntiDetectionSystem();
    void updateContext(Context newContext);
    int getHumanDelay();
    double getHumanAccuracy();
    bool needsMicroPause();
    void executeMicroPause();
    void registerAction(int actionType);
private:
    std::vector<int> humanDelayPatterns;
    std::vector<double> humanAccuracyPatterns;
    double fatigueLevel = 0.0;
    std::chrono::steady_clock::time_point sessionStartTime;
    int totalActions = 0;
    std::deque<std::pair<int, std::chrono::steady_clock::time_point>> actionHistory;
    std::chrono::steady_clock::time_point lastMicroPause;
    bool isInMicroPause = false;
    Context currentContext = IDLE;
    void updateFatigue();
};

// Predictive Aim System
class PredictiveAimSystem {
private:
    struct TargetPrediction {
        POINT estimatedPosition, velocity, acceleration;
        double confidence;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::deque<std::pair<POINT, std::chrono::steady_clock::time_point>> cursorHistory;
    TargetPrediction currentPrediction;
    int maxHistorySize = 15;
    int predictionTimeMs = 30;
    int networkLatencyMs = 20;
    double kalmanGain = 0.6;
    POINT estimatedState = {0, 0};
    std::deque<std::pair<POINT, std::chrono::steady_clock::time_point>> userMouseDeltaHistory;
    double currentUserMouseVelocity = 0.0;
    POINT lastCursorPosForUserVelocity = {0,0};
    int userMouseDeltaHistorySize = 5;

    void updatePrediction();
    void calculateUserMouseVelocity();
    POINT calculateAverageVelocity();
    POINT calculateAcceleration();
    double calculateConfidence();
    double calculateSpeedConsistency();
    double calculateDirectionConsistency();
public:
    PredictiveAimSystem();
    void updateCursorHistory();
    POINT getPredictedTarget(double humanAccuracy = 1.0);
    double getPredictionConfidence();
    POINT getTargetVelocity();
    POINT getAcceleration();
    double getUserMouseVelocity() const;
    POINT getRawMouseDelta();
};

// Momentum System
class MomentumSystem {
private:
    struct MovementState {
        bool isInSprint = false, isSliding = false, isDiving = false, isInAir = false;
        std::chrono::steady_clock::time_point lastMovement;
        POINT lastDirection = {0, 0};
        double momentum = 0.0;
    };
    MovementState state;
public:
    MomentumSystem();
    void updateMovementState(bool sprint, bool slide, bool dive, bool air);
    double getCurrentMomentum();
    void addMomentum(double amount);
    bool canChainMovement();
    POINT getOptimalDirection(POINT targetDirection);
    void setLastDirection(POINT direction);
};

// Visual Feedback System
class VisualFeedbackSystem {
private:
    struct FeedbackState {
        std::string currentMovement;
        std::chrono::steady_clock::time_point movementStart;
        int movementProgress = 0;
        bool showProgress = true;
    };
    FeedbackState feedback;
public:
    void startMovement(const std::string& movementName, int expectedDuration = 1000);
    void updateProgress(int progressPercent);
    void finishMovement(bool success = true);
    void logDirectionalHint(const std::string& direction, const std::string& effect);
};

// Helper Class
class FluidMovementSystem {
public:
    struct BezierCurve { POINT p0, p1, p2, p3; };
    POINT interpolateCubicBezier(const BezierCurve& curve, double t);
    BezierCurve generateNaturalCurve(POINT start, POINT end);
    double getVelocityProfile(double progress);
    std::vector<POINT> generateMovementPath(POINT start, POINT end, int steps);
};