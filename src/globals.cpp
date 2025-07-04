// description: This file contains the definitions of global variables and objects used throughout the application.
// It includes necessary libraries and provides a random number generator for various functionalities.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "globals.h" // Include the global header for definitions and declarations
#include "profiles.h" // Include the profiles header for weapon profiles
#include "systems.h" // Include the systems header for various tactical systems
#include "audio_system.h" // Include the audio system header for audio management

// Definition of global variables
HWND g_hWnd = NULL;
std::atomic<bool> g_running(true);
std::atomic<bool> g_assistEnabled(true);

int MONITOR_WIDTH = 1920;
int MONITOR_HEIGHT = 1080;
int SCREEN_CENTER_X = 1920 / 2;
int SCREEN_CENTER_Y = 1080 / 2;

std::vector<WeaponProfile> g_weaponProfiles;
std::atomic<int> g_activeProfileIndex(0);

std::unique_ptr<AntiDetectionSystem> g_antiDetection; // Anti-detection system to simulate human-like behavior
std::unique_ptr<PredictiveAimSystem> g_predictiveAim; // Predictive aim system for target tracking
std::unique_ptr<PerformanceOptimizer> g_performanceOpt; // Performance optimizer for managing system resources
std::unique_ptr<AdaptiveSmoothingSystem> g_smoothingSystem; // Adaptive smoothing system for mouse movements
std::unique_ptr<MomentumSystem> g_momentumSys; // Momentum system for movement optimization
std::unique_ptr<VisualFeedbackSystem> g_feedbackSys; // Visual feedback system for user interaction
std::unique_ptr<TacticalPIDController> g_pidX; // PID controllers for X and Y axes
std::unique_ptr<TacticalPIDController> g_pidY; // PID controllers for X and Y axes
std::unique_ptr<AudioManager> g_audioManager; // Definition for the new audio manager

// Definition for the new Tactical fire mode state
std::atomic<bool> isTacticalFiring(false);

std::random_device rd;
std::mt19937 gen(rd());