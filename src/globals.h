// description: This header file declares global variables and forward declarations for the Tactical Aim Assist project.
// It includes necessary libraries and defines constants used throughout the project.
// It also provides a logging function for debugging purposes.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <random>

// Forward declarations to break circular dependencies
class WeaponProfile; // Forward declaration for the weapon profile class
class AntiDetectionSystem; // Forward declaration for the anti-detection system
class PredictiveAimSystem; // Forward declaration for the predictive aim system
class PerformanceOptimizer; // Forward declaration for the performance optimizer
class AdaptiveSmoothingSystem; // Forward declaration for the adaptive smoothing system
class MomentumSystem; // Forward declaration for the momentum system
class VisualFeedbackSystem; // Forward declaration for the visual feedback system
class TacticalPIDController; // Forward declaration for the Tactical PID controller
class AudioManager; // Forward declaration for the new audio manager

// --- Global Variables ---
extern HWND g_hWnd;
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_assistEnabled;

// Screen configuration
extern int MONITOR_WIDTH;
extern int MONITOR_HEIGHT;
extern int SCREEN_CENTER_X;
extern int SCREEN_CENTER_Y;

// Weapon Profiles
extern std::vector<WeaponProfile> g_weaponProfiles;
extern std::atomic<int> g_activeProfileIndex;

// Systems
extern std::unique_ptr<AntiDetectionSystem> g_antiDetection; // Anti-detection system to simulate human-like behavior
extern std::unique_ptr<PredictiveAimSystem> g_predictiveAim; // Predictive aim system for target tracking
extern std::unique_ptr<PerformanceOptimizer> g_performanceOpt; // Performance optimizer for managing system resources
extern std::unique_ptr<AdaptiveSmoothingSystem> g_smoothingSystem; // Adaptive smoothing system for mouse movements
extern std::unique_ptr<MomentumSystem> g_momentumSys; // Momentum system for movement optimization
extern std::unique_ptr<VisualFeedbackSystem> g_feedbackSys; // Visual feedback system for user interaction
extern std::unique_ptr<TacticalPIDController> g_pidX; // PID controllers for X and Y axes
extern std::unique_ptr<TacticalPIDController> g_pidY; // PID controllers for X and Y axes
extern std::unique_ptr<AudioManager> g_audioManager; // New audio manager
extern std::atomic<bool> isTacticalFiring; // Flag to indicate if tactical firing is active

// Enhanced random number generation
extern std::random_device rd;
extern std::mt19937 gen;

// Logging
void logMessage(const std::string& message);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif