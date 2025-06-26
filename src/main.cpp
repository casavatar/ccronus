// description: 
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist
//

#include "globals.h" // Include the global header for shared variables and objects
#include "config.h" // Include the configuration header for loading settings
#include "gui.h" // Include the GUI header for the enhanced GUI thread
#include "profiles.h" // Include the profiles header for weapon profiles
#include "systems.h" // Include the systems header for system management 
#include "audio_system.h" // Include the new audio system header
#include <thread> // Include the thread library for multithreading support
#include <iostream> // Include the iostream library for console output
#include <sstream> // Include the sstream library for string stream operations

// Initialize all systems
void initializeSystems() {
    g_antiDetection = std::make_unique<AntiDetectionSystem>(); // Anti-detection system to simulate human-like behavior
    g_predictiveAim = std::make_unique<PredictiveAimSystem>(); // Predictive aim system for target tracking
    g_performanceOpt = std::make_unique<PerformanceOptimizer>(); // Performance optimizer for managing system resources
    g_smoothingSystem = std::make_unique<AdaptiveSmoothingSystem>(); // Adaptive smoothing system for mouse movements
    g_momentumSys = std::make_unique<MomentumSystem>(); // Momentum system for movement optimization
    g_feedbackSys = std::make_unique<VisualFeedbackSystem>(); // Visual feedback system for user interaction
    g_audioManager = std::make_unique<AudioManager>(44100, 512, 256); // Initialize the audio manager with sample rate, buffer size, and hop size
}

// Load configuration from a file
void macroLoop() {
    logMessage("Macro loop started. Waiting for input events...");
    
    while (g_running.load()) {
        static auto lastAnalyticsUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAnalyticsUpdate).count() > 100) {
            if (guiReady.load()) {
                 g_performanceOpt->addTask(updateAnalyticsLabel);
            }
            if (g_audioManager) {
                std::string alert = g_audioManager->getLatestAlert();
                if (!alert.empty()) {
                    updateAudioAlertLabel(alert);
                }
            }
            lastAnalyticsUpdate = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    logMessage("Macro loop finished.");
}

// Function to switch weapon profiles
int main() {
    initializeSystems();
    
    if (!loadConfiguration("config.json")) {
        return 1;
    }
    
    logMessage("Configuration loaded.");
    
    std::ostringstream systemInfo;
    systemInfo << "System: " << std::thread::hardware_concurrency() << " cores | "
               << "Monitor: " << MONITOR_WIDTH << "x" << MONITOR_HEIGHT;
    logMessage(systemInfo.str());

    if (!g_weaponProfiles.empty()) {
        const auto& first_profile = g_weaponProfiles[0];
        g_pidX = std::make_unique<TacticalPIDController>(first_profile.pid_kp, first_profile.pid_ki, first_profile.pid_kd);
        g_pidY = std::make_unique<TacticalPIDController>(first_profile.pid_kp, first_profile.pid_ki, first_profile.pid_kd);
        g_smoothingSystem->updateProfile(first_profile.smoothingFactor, first_profile.smoothingFactor * 0.8, first_profile.smoothingFactor * 1.1);
        logMessage("PID Controllers initialized for " + first_profile.name);
    } else {
        MessageBoxW(NULL, L"No weapon profiles found in config.json!", L"Config Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    std::thread guiThread(enhancedGuiThread);
    
    // Wait for the GUI to be ready before proceeding
    while(!guiReady.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    switchProfile(0); // Establish the initial profile and update the label

    // Start the audio manager after the GUI is ready
    if (g_audioManager) {
        g_audioManager->start();
    }
    
    std::thread macroThread(macroLoop);

    logMessage("All threads started successfully.");
    
    if(guiThread.joinable()) {
        guiThread.join();
    }
    
    g_running = false; // Make sure the macro loop ends

    if(macroThread.joinable()) {
        macroThread.join();
    }

    logMessage("System Shutdown Complete.");
    return 0;
}