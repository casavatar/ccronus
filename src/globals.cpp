// description: This file contains the definitions of global variables and objects used throughout the application.
// It includes necessary libraries and provides a random number generator for various functionalities.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.8.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "globals.h"
#include "profiles.h"
#include "systems.h"
#include "audio_system.h"

// Definition of global variables
HWND g_hWnd = NULL;
std::atomic<bool> g_running(true); // Flag to indicate if the application is running
std::atomic<bool> g_assistEnabled(true); // Flag to indicate if the assist is enabled
std::atomic<bool> g_isSimulatingInput(false); // Flag to indicate if input simulation is active

int MONITOR_WIDTH = 1920; // Width and height of the target monitor
int MONITOR_HEIGHT = 1080; // Width and height of the target monitor
int g_monitorOffsetX = 0; // X offset definition
int g_monitorOffsetY = 0; // Y offset definition
int SCREEN_CENTER_X = 1920 / 2; // Center coordinates of the screen
int SCREEN_CENTER_Y = 1080 / 2; // Center coordinates of the screen

std::vector<WeaponProfile> g_weaponProfiles;
std::atomic<int> g_activeProfileIndex(0);

// Systems
std::unique_ptr<AntiDetectionSystem> g_antiDetection;
std::unique_ptr<PredictiveAimSystem> g_predictiveAim;
std::unique_ptr<PerformanceOptimizer> g_performanceOpt;
std::unique_ptr<AdaptiveSmoothingSystem> g_smoothingSystem;
std::unique_ptr<MomentumSystem> g_momentumSys;
std::unique_ptr<VisualFeedbackSystem> g_feedbackSys;
std::unique_ptr<AudioManager> g_audioManager;
// PID Controllers
std::unique_ptr<TacticalPIDController> g_pidX;
std::unique_ptr<TacticalPIDController> g_pidY;
// NEW: Definition for the player's movement state
std::atomic<PlayerMovementState> g_playerMovementState(PlayerMovementState::Stationary);


// Firing mode states - Centralized definition
std::atomic<bool> isTacticalFiring(false);
std::atomic<bool> isControlledAutoFiring(false);
std::atomic<bool> isRapidFiring(false);

// Contextual Movement States
std::atomic<bool> isSprintingForward(false);
std::chrono::steady_clock::time_point g_sprintStartTime;

// Enhanced random number generation
std::random_device rd;
std::mt19937 gen(rd());