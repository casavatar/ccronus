// description: This header file declares global variables and forward declarations for the Tactical Aim Assist project.
// It includes necessary libraries and defines constants used throughout the project.
// It also provides a logging function for debugging purposes.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.2.1
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <random>
#include <chrono>

// NEW: Enum to represent the player's current movement state
enum class PlayerMovementState {
    Stationary,
    Walking,
    Sprinting,
    Strafing
};

// NEW: Global variable to track the player's movement state
extern std::atomic<PlayerMovementState> g_playerMovementState; // NEW: Tracks the player's movement state

// Forward declarations to break circular dependencies
class WeaponProfile;
class AntiDetectionSystem;
class PredictiveAimSystem;
class PerformanceOptimizer;
class AdaptiveSmoothingSystem;
class MomentumSystem;
class VisualFeedbackSystem;
class TacticalPIDController;
class AudioManager;

// --- Global Variables ---
extern HWND g_hWnd;
extern std::atomic<bool> g_running;
extern std::atomic<bool> g_assistEnabled;
extern std::atomic<bool> g_isSimulatingInput;

// Screen configuration
extern int MONITOR_WIDTH; // Width and height of the target monitor
extern int MONITOR_HEIGHT; // Width and height of the target monitor
extern int g_monitorOffsetX; // X offset of the target monitor
extern int g_monitorOffsetY; // Y offset of the target monitor
extern int SCREEN_CENTER_X; // Center coordinates of the screen
extern int SCREEN_CENTER_Y; // Center coordinates of the screen

// Weapon Profiles
extern std::vector<WeaponProfile> g_weaponProfiles;
extern std::atomic<int> g_activeProfileIndex;
extern std::atomic<bool> isTacticalFiring;

// Contextual Movement States
extern std::atomic<bool> isSprintingForward;
extern std::chrono::steady_clock::time_point g_sprintStartTime; // Tracks when sprint started

// Systems
extern std::unique_ptr<AntiDetectionSystem> g_antiDetection;
extern std::unique_ptr<PredictiveAimSystem> g_predictiveAim;
extern std::unique_ptr<PerformanceOptimizer> g_performanceOpt;
extern std::unique_ptr<AdaptiveSmoothingSystem> g_smoothingSystem;
extern std::unique_ptr<MomentumSystem> g_momentumSys;
extern std::unique_ptr<VisualFeedbackSystem> g_feedbackSys;
extern std::unique_ptr<AudioManager> g_audioManager;
// PID Controllers
extern std::unique_ptr<TacticalPIDController> g_pidX;
extern std::unique_ptr<TacticalPIDController> g_pidY;


// Enhanced random number generation
extern std::random_device rd;
extern std::mt19937 gen;

// Logging
void logMessage(const std::string& message);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif