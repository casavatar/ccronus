// main.cpp - CORRECTED AND UPDATED VERSION v4.1.1
// description: Main entry point for Tactical Aim Assist.
// version: 4.1.1 - Fixed missing include for the input system.
// date: 2025-07-21
// project: Tactical Aim Assist

#include "globals.h"
#include "config.h"
#include "state_manager.h"
#include "event_system.h"
#include "assist_optimized.h"
#include "input.h"
#include "action_handler.h"
#include "performance_profiler.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <csignal>

// Global flag to signal all threads to exit.
// g_application_running is defined in globals.cpp

// Handles Ctrl+C and other termination signals gracefully.
void signalHandler(int signum) {
    logMessage("Termination signal received: " + std::to_string(signum) + ". Shutting down...");
    g_application_running.store(false);
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    logMessage("--- Tactical Aim Assist Starting Up ---");

    // --- 1. INITIALIZATION SEQUENCE ---
    if (!initializeStateManager()) {
        logError("FATAL: StateManager initialization failed. Exiting.");
        return EXIT_FAILURE;
    }

    if (!initializeEventSystem()) {
        logError("FATAL: EventSystem initialization failed. Exiting.");
        shutdownStateManager();
        return EXIT_FAILURE;
    }

    if (!loadConfiguration(GlobalConstants::MAIN_CONFIG_FILE)) {
        logWarning("Could not load configuration file. Applying default settings.");
        applyDefaultConfiguration();
    }
    
    if (!initializeInputSystem()) {
        logError("FATAL: Input System initialization failed. Exiting.");
        shutdownEventSystem();
        shutdownStateManager();
        return EXIT_FAILURE;
    }

    if (!initializeActionHandler()) {
        logError("FATAL: Action Handler initialization failed. Exiting.");
        shutdownInputSystem();
        shutdownEventSystem();
        shutdownStateManager();
        return EXIT_FAILURE;
    }
    
    if (!initializeAimAssistSystem()) {
        logError("FATAL: Aim Assist System initialization failed. Exiting.");
        shutdownActionHandler();
        shutdownInputSystem();
        shutdownEventSystem();
        shutdownStateManager();
        return EXIT_FAILURE;
    }
    
    initializeProfiler();

    // --- 2. START BACKGROUND THREADS ---
    std::vector<std::thread> threads;
    logMessage("Spawning application threads...");

    threads.emplace_back(runOptimizedAimAssistLoop);
    threads.emplace_back(runInputLoop);

    threads.emplace_back([]() {
        if (auto* profiler = getProfiler()) {
            profiler->startMonitoring();
            while (g_application_running.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            profiler->stopMonitoring();
        }
    });

    logMessage("--- Application is now running. Press Ctrl+C to exit. ---");

    // --- 3. MAIN THREAD LOOP ---
    while (g_application_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // --- 4. SHUTDOWN SEQUENCE ---
    logMessage("--- Shutting down application... ---");

    g_application_running.store(false);

    logMessage("Waiting for threads to join...");
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    logMessage("All threads have finished.");

    // Shut down systems in reverse order of initialization.
    logMessage("Shutting down systems...");
    shutdownProfiler();
    shutdownAimAssistSystem();
    shutdownActionHandler();
    shutdownInputSystem();
    shutdownEventSystem();
    shutdownStateManager();

    logMessage("--- Tactical Aim Assist Shutdown Complete ---");

    return EXIT_SUCCESS;
}