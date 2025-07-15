// description: Main entry point for the Tactical Aim Assist application.
// Initializes all systems, manages threads, and handles the main application loop.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.1
// date: 2025-06-26
// project: Tactical Aim Assist

#include "globals.h"
#include "config.h"
#include "gui.h"
#include "profiles.h"
#include "systems.h"
#include "audio_system.h"
#include <thread>
#include <iostream>
#include <sstream>

extern std::atomic<bool> isExecutingMovement;

void initializeSystems() {
    g_antiDetection = std::make_unique<AntiDetectionSystem>();
    g_predictiveAim = std::make_unique<PredictiveAimSystem>();
    g_performanceOpt = std::make_unique<PerformanceOptimizer>();
    g_smoothingSystem = std::make_unique<AdaptiveSmoothingSystem>();
    g_momentumSys = std::make_unique<MomentumSystem>();
    g_feedbackSys = std::make_unique<VisualFeedbackSystem>();
    g_audioManager = std::make_unique<AudioManager>(44100, 512);
}

void macroLoop() {
    logMessage("Macro loop started. Waiting for input events...");

    while (g_running.load()) {
        static auto lastAnalyticsUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAnalyticsUpdate).count() > 100) {
            if (guiReady.load()) {
                 postUpdateAnalytics();
            }
            if (g_audioManager) {
                std::string alert = g_audioManager->getLatestAlert();
                if (!alert.empty()) {
                    postUpdateAudioAlert(alert);
                }
            }
            
            if (isSprintingForward.load()) {
                auto time_since_sprint_start = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_sprintStartTime).count();
                if (time_since_sprint_start > 500) {
                    postUpdateMovementStatus("Movement: Strafe-Jump Ready");
                } else {
                    postUpdateMovementStatus("Movement: Sprinting...");
                }
            } else {
                 if (!isExecutingMovement.load()) {
                    postUpdateMovementStatus("Movement: Idle");
                 }
            }

            lastAnalyticsUpdate = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    logMessage("Macro loop finished.");
}

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
        
        // FIX: Initialize PID controllers using the new pid_states structure.
        // We will use the "stationary" state as the default for initialization.
        if (first_profile.pid_states.count("stationary")) {
            const auto& initial_pid = first_profile.pid_states.at("stationary");
            g_pidX = std::make_unique<TacticalPIDController>(initial_pid.kp, initial_pid.ki, initial_pid.kd);
            g_pidY = std::make_unique<TacticalPIDController>(initial_pid.kp, initial_pid.ki, initial_pid.kd);
        } else {
            // Fallback to default values if "stationary" state is missing for some reason
            g_pidX = std::make_unique<TacticalPIDController>(0.5, 0.1, 0.1);
            g_pidY = std::make_unique<TacticalPIDController>(0.5, 0.1, 0.1);
        }

        g_smoothingSystem->updateProfile(first_profile.smoothingFactor, first_profile.smoothingFactor * 0.8, first_profile.smoothingFactor * 1.1);
        logMessage("PID Controllers initialized for " + first_profile.name);
    } else {
        MessageBoxW(NULL, L"No weapon profiles found in config.json!", L"Config Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::thread guiThread(enhancedGuiThread);

    while(!guiReady.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    switchProfile(0);

    if (g_audioManager) {
        g_audioManager->start();
    }

    std::thread macroThread(macroLoop);

    logMessage("All threads started successfully.");

    if(guiThread.joinable()) {
        guiThread.join();
    }

    g_running = false;
    if (g_audioManager) {
        g_audioManager->stop();
    }
    if(macroThread.joinable()) {
        macroThread.join();
    }

    logMessage("System Shutdown Complete.");
    return 0;
}