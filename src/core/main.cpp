// main.cpp - CORRECTED AND UPDATED VERSION v4.2.0
// --------------------------------------------------------------------------------------
// description: Main entry point for Tactical Aim Assist with comprehensive validation system.
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 4.2.0 - Added application validation and error handling
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "main.h"
#include "globals.h"
#include "state_manager.h"
#include "event_system.h"
#include "config.h"
#include "input.h"
#include "action_handler.h"
#include "assist_optimized.h"
#include "gui.h"
#include "performance_profiler.h"
#include "application_validator.h"

#include <iostream>
#include <vector>
#include <csignal>
#include <thread>
#include <chrono>
#include <exception>
#include <stdexcept>

// Global flag to signal all threads to exit.
// g_application_running is defined in globals.cpp

// Handles Ctrl+C and other termination signals gracefully.
void signalHandler(int signum) {
    logMessage("Termination signal received: " + std::to_string(signum) + ". Shutting down...");
    g_application_running.store(false);
}

// Structured exception handling for initialization
class InitializationException : public std::exception {
private:
    std::string m_component;
    std::string m_message;
    
public:
    InitializationException(const std::string& component, const std::string& message) 
        : m_component(component), m_message(message) {}
    
    const char* what() const noexcept override { 
        return ("Initialization failed for " + m_component + ": " + m_message).c_str(); 
    }
    
    const std::string& getComponent() const { return m_component; }
    const std::string& getMessage() const { return m_message; }
};

// Safe initialization wrapper
template<typename Func>
bool safeInitialize(const std::string& componentName, Func initFunc, const std::string& errorMsg) {
    try {
        __try {
            if (!initFunc()) {
                throw InitializationException(componentName, errorMsg);
            }
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            throw InitializationException(componentName, "Windows exception during initialization");
        }
    } catch (const InitializationException& e) {
        logError("Initialization failed: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        logError("Unexpected error during " + componentName + " initialization: " + std::string(e.what()));
        return false;
    } catch (...) {
        logError("Unknown exception during " + componentName + " initialization");
        return false;
    }
}

int main() {
    __try {
        // Initialize validation system first
        if (!safeInitialize("Application Validation", 
                          initializeApplicationValidation, 
                          "Failed to initialize application validation system")) {
            std::cerr << "FATAL: Failed to initialize application validation system." << std::endl;
            return EXIT_FAILURE;
        }
        
        // Set up signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        logMessage("--- Tactical Aim Assist Starting Up ---");

        // --- 1. INITIALIZATION SEQUENCE WITH VALIDATION ---
        if (!safeInitialize("StateManager", initializeStateManager, "StateManager initialization failed")) {
            handleApplicationError("StateManager initialization failed", ValidationLevel::Critical);
            return EXIT_FAILURE;
        }
        logMessage("StateManager initialized successfully");

        if (!safeInitialize("EventSystem", initializeEventSystem, "EventSystem initialization failed")) {
            handleApplicationError("EventSystem initialization failed", ValidationLevel::Critical);
            shutdownStateManager();
            return EXIT_FAILURE;
        }
        logMessage("EventSystem initialized successfully");

        if (!safeInitialize("Configuration", 
                          []() { return loadConfiguration(GlobalConstants::MAIN_CONFIG_FILE); }, 
                          "Configuration loading failed")) {
            logWarning("Could not load configuration file. Applying default settings.");
            applyDefaultConfiguration();
        }
        logMessage("Configuration loaded successfully");
        
        if (!safeInitialize("Input System", initializeInputSystem, "Input System initialization failed")) {
            handleApplicationError("Input System initialization failed", ValidationLevel::Critical);
            shutdownEventSystem();
            shutdownStateManager();
            return EXIT_FAILURE;
        }
        logMessage("Input System initialized successfully");

        if (!safeInitialize("Action Handler", initializeActionHandler, "Action Handler initialization failed")) {
            handleApplicationError("Action Handler initialization failed", ValidationLevel::Critical);
            shutdownInputSystem();
            shutdownEventSystem();
            shutdownStateManager();
            return EXIT_FAILURE;
        }
        logMessage("Action Handler initialized successfully");
        
        if (!safeInitialize("Aim Assist System", initializeAimAssistSystem, "Aim Assist System initialization failed")) {
            handleApplicationError("Aim Assist System initialization failed", ValidationLevel::Critical);
            shutdownActionHandler();
            shutdownInputSystem();
            shutdownEventSystem();
            shutdownStateManager();
            return EXIT_FAILURE;
        }
        logMessage("Aim Assist System initialized successfully");

        if (!safeInitialize("Performance Profiler", initializePerformanceProfiler, "Performance profiler initialization failed")) {
            logWarning("Performance profiler initialization failed, continuing without profiling");
        } else {
            logMessage("Performance profiler initialized successfully");
        }

        // Initialize GUI system with comprehensive safety checks
        if (!safeInitialize("GUI System", initializeGUI, "GUI initialization failed")) {
            handleApplicationError("GUI initialization failed", ValidationLevel::Critical);
            shutdownAimAssistSystem();
            shutdownActionHandler();
            shutdownInputSystem();
            shutdownEventSystem();
            shutdownStateManager();
            return EXIT_FAILURE;
        }
        logMessage("GUI system initialized successfully");

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("An unexpected error occurred during application initialization.");
        return EXIT_FAILURE;
    }

    // --- 2. START BACKGROUND THREADS WITH VALIDATION ---
    std::vector<std::thread> threads;
    logMessage("Spawning application threads...");

    // Start validation monitoring thread with safety checks
    threads.emplace_back([]() {
        __try {
            while (g_application_running.load()) {
                VALIDATE_SYSTEM_HEALTH();
                
                // Perform periodic validation
                if (!isApplicationHealthy()) {
                    handleApplicationError("System health check failed", ValidationLevel::Warning);
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in validation monitoring thread");
            g_application_running.store(false);
        }
    });

    // Start aim assist loop with error handling
    threads.emplace_back([]() {
        __try {
            runOptimizedAimAssistLoop();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in aim assist thread");
            g_application_running.store(false);
        }
    });

    // Start input loop with error handling
    threads.emplace_back([]() {
        __try {
            runInputLoop();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in input thread");
            g_application_running.store(false);
        }
    });

    // Start profiler monitoring thread with safety checks
    threads.emplace_back([]() {
        __try {
            if (auto* profiler = getProfiler()) {
                profiler->startMonitoring();
                while (g_application_running.load()) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                profiler->stopMonitoring();
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in profiler monitoring thread");
            g_application_running.store(false);
        }
    });

    // Start GUI message loop thread with safety checks
    threads.emplace_back([]() {
        __try {
            runGuiMessageLoop();
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in GUI message loop thread");
            g_application_running.store(false);
        }
    });

    // Start performance monitoring thread with profile-guided optimization
    threads.emplace_back([]() {
        __try {
            auto lastUpdate = std::chrono::steady_clock::now();
            auto lastOptimization = std::chrono::steady_clock::now();
            static constexpr auto UPDATE_INTERVAL = std::chrono::milliseconds(1000);
            static constexpr auto OPTIMIZATION_INTERVAL = std::chrono::seconds(30);
            
            while (g_application_running.load()) {
                auto now = std::chrono::steady_clock::now();
                
                if (now - lastUpdate >= UPDATE_INTERVAL) {
                    // Get performance metrics from profiler
                    if (auto* profiler = getProfiler()) {
                        auto stats = profiler->getStatistics();
                        
                        // Update GUI with performance metrics
                        updatePerformanceMetrics(
                            stats.cpu_usage,
                            stats.memory_usage_mb,
                            stats.thread_count,
                            stats.average_response_time_ms
                        );
                        
                        // Record component usage for profile-guided optimization
                        profiler->recordComponentUsage("main_loop", 
                            std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastUpdate).count());
                    }
                    
                    lastUpdate = now;
                }
                
                // Apply profile-guided optimizations periodically
                if (now - lastOptimization >= OPTIMIZATION_INTERVAL) {
                    if (auto* profiler = getProfiler()) {
                        profiler->applyProfileGuidedOptimizations();
                        
                        // Get optimization suggestions
                        auto suggestions = profiler->getOptimizationSuggestions();
                        for (const auto& suggestion : suggestions) {
                            logMessage("Optimization suggestion: " + suggestion);
                        }
                    }
                    
                    lastOptimization = now;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Critical exception in performance monitoring thread");
            g_application_running.store(false);
        }
    });

    // Start memory pool tuning thread
    threads.emplace_back([]() {
        auto lastTuning = std::chrono::steady_clock::now();
        static constexpr auto TUNING_INTERVAL = std::chrono::seconds(60);
        
        while (g_application_running.load()) {
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastTuning >= TUNING_INTERVAL) {
                // Perform adaptive memory pool tuning
                logMessage("Performing adaptive memory pool tuning...");
                
                // Note: PoolManager integration would be implemented here
                // For now, just log the tuning attempt
                
                lastTuning = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    logMessage("--- Application is now running. Press Ctrl+C to exit. ---");

    // --- 3. MAIN THREAD LOOP WITH VALIDATION ---
    auto lastValidationCheck = std::chrono::steady_clock::now();
    static constexpr auto VALIDATION_INTERVAL = std::chrono::seconds(30); // Reduced from 100ms
    
    while (g_application_running.load()) {
        auto now = std::chrono::steady_clock::now();
        
        // Perform periodic application validation less frequently
        if (now - lastValidationCheck >= VALIDATION_INTERVAL) {
            VALIDATE_SYSTEM_HEALTH();
            lastValidationCheck = now;
            
            // Check if shutdown was requested by validation system
            if (!isApplicationHealthy()) {
                logWarning("Application health check failed, initiating recovery...");
                if (!requestApplicationRecovery(RecoveryAction::Reinitialize)) {
                    logError("Recovery failed, shutting down...");
                    g_application_running.store(false);
                    break;
                }
            }
        }
        
        // Increased sleep interval to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Was 100ms
    }

    // --- 4. GRACEFUL SHUTDOWN SEQUENCE WITH VALIDATION ---
    logMessage("--- Shutting down application... ---");

    g_application_running.store(false);

    logMessage("Waiting for threads to join...");
    for (auto& t : threads) {
        if (t.joinable()) {
            __try {
                t.join();
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                logError("Exception while joining thread");
            }
        }
    }
    logMessage("All threads have finished.");

    // Shut down systems in reverse order of initialization with validation
    logMessage("Shutting down systems...");
    
    __try {
        shutdownProfiler();
        logMessage("Profiler shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during profiler shutdown");
    }
    
    __try {
        shutdownAimAssistSystem();
        logMessage("Aim Assist System shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during aim assist system shutdown");
    }
    
    __try {
        shutdownActionHandler();
        logMessage("Action Handler shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during action handler shutdown");
    }
    
    __try {
        shutdownInputSystem();
        logMessage("Input System shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during input system shutdown");
    }
    
    __try {
        shutdownEventSystem();
        logMessage("Event System shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during event system shutdown");
    }
    
    __try {
        shutdownStateManager();
        logMessage("State Manager shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during state manager shutdown");
    }
    
    __try {
        cleanupGUI();
        logMessage("GUI cleanup complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during GUI cleanup");
    }

    // Shutdown validation system last
    __try {
        shutdownApplicationValidation();
        logMessage("Application validation shutdown complete");
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception during application validation shutdown");
    }

    logMessage("--- Tactical Aim Assist Shutdown Complete ---");

    return EXIT_SUCCESS;
}