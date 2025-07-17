// input.cpp - COMPLETE UPDATED VERSION v3.1.3
// description: Input handling system with contextual movement assist
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.3 - Complete integration with globals and keybindings
// date: 2025-07-17
// project: Tactical Aim Assist

#include "input.h"
#include "globals.h"
#include "common_defines.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cmath>

// =============================================================================
// GLOBALSTATE NAMESPACE IMPLEMENTATION
// =============================================================================

namespace GlobalState {
    bool isAssistEnabled() {
        return SAFE_ATOMIC_LOAD(g_aimAssistEnabled);
    }
    
    PlayerMovementState getCurrentMovementState() {
        return static_cast<PlayerMovementState>(SAFE_ATOMIC_LOAD(g_currentMovementState));
    }
    
    void setMovementState(PlayerMovementState state) {
        SAFE_ATOMIC_STORE(g_currentMovementState, static_cast<int>(state));
    }
    
    int getActiveProfileIndex() {
        return SAFE_ATOMIC_LOAD(g_activeProfileIndex);
    }
    
    bool isSystemActive() {
        return SAFE_ATOMIC_LOAD(g_systemActive) && SAFE_ATOMIC_LOAD(g_inputSystemActive);
    }
    
    bool isSimulating() {
        return SAFE_ATOMIC_LOAD(g_isSimulatingInput);
    }
}

// =============================================================================
// CONTEXTUAL MOVEMENT ASSIST
// =============================================================================

void executeContextualMovementAssist() {
    PlayerMovementState currentState = GlobalState::getCurrentMovementState();
    
    switch (currentState) {
        case PlayerMovementState::Sprinting:
            // Assist with sprint movement - smooth acceleration
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double sprint_smoothing = SAFE_ATOMIC_LOAD(g_movementSmoothingFactor) * 0.8;
                SAFE_ATOMIC_STORE(g_aimSmoothingFactor, sprint_smoothing);
            }
            break;
            
        case PlayerMovementState::Strafing:
            // Assist with strafing movement - adjust aim compensation
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double strafe_compensation = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier) * 1.2;
                SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, 
                                std::min(strafe_compensation, GlobalConstants::MAX_SENSITIVITY));
            }
            break;
            
        case PlayerMovementState::Running:
            // Assist with running movement - moderate smoothing
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double run_smoothing = SAFE_ATOMIC_LOAD(g_movementSmoothingFactor) * 0.9;
                SAFE_ATOMIC_STORE(g_aimSmoothingFactor, run_smoothing);
            }
            break;
            
        case PlayerMovementState::Walking:
            // Assist with walking movement - high precision
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double walk_precision = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier) * 0.8;
                SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, walk_precision);
            }
            break;
            
        case PlayerMovementState::Crouching:
            // Assist with crouched movement - maximum precision
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double crouch_precision = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier) * 0.6;
                SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, crouch_precision);
                SAFE_ATOMIC_STORE(g_aimSmoothingFactor, 0.9);
            }
            break;
            
        case PlayerMovementState::Jumping:
            // Assist with jumping movement - predictive aiming
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled) && SAFE_ATOMIC_LOAD(g_predictionEnabled)) {
                double jump_prediction = SAFE_ATOMIC_LOAD(g_predictionTime) * 1.5;
                SAFE_ATOMIC_STORE(g_predictionTime, jump_prediction);
            }
            break;
            
        case PlayerMovementState::Falling:
            // Assist with falling movement - enhanced prediction
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled) && SAFE_ATOMIC_LOAD(g_predictionEnabled)) {
                double fall_prediction = SAFE_ATOMIC_LOAD(g_predictionTime) * 2.0;
                SAFE_ATOMIC_STORE(g_predictionTime, fall_prediction);
            }
            break;
            
        case PlayerMovementState::Sliding:
            // Assist with sliding movement - dynamic compensation
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                double slide_compensation = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier) * 1.5;
                SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, 
                                std::min(slide_compensation, GlobalConstants::MAX_SENSITIVITY));
            }
            break;
            
        case PlayerMovementState::Stationary:
        default:
            // Reset to default settings for stationary state
            if (SAFE_ATOMIC_LOAD(g_aimAssistEnabled)) {
                SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, 1.0);
                SAFE_ATOMIC_STORE(g_aimSmoothingFactor, 0.75);
                SAFE_ATOMIC_STORE(g_predictionTime, 50.0);
            }
            break;
    }
    
    UPDATE_COUNTER(g_totalMovements);
}

// =============================================================================
// MOVEMENT STATE DETECTION
// =============================================================================

void updateMovementState() {
    if (!GlobalState::isSystemActive()) {
        return;
    }
    
    // Check key states
    bool isShiftPressed = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);
    bool isCtrlPressed = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000);
    bool isSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000);
    bool isWASDPressed = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('A') & 0x8000) ||
                        (GetAsyncKeyState('S') & 0x8000) || (GetAsyncKeyState('D') & 0x8000);
    
    PlayerMovementState newState = PlayerMovementState::Stationary;
    
    if (isShiftPressed && isWASDPressed) {
        if (isCtrlPressed) {
            // Shift + Ctrl + Movement = Sliding
            newState = PlayerMovementState::Sliding;
        } else {
            // Shift + Movement = Sprinting
            newState = PlayerMovementState::Sprinting;
        }
    } else if (isCtrlPressed && isWASDPressed) {
        // Ctrl + Movement = Crouching
        newState = PlayerMovementState::Crouching;
    } else if (isSpacePressed) {
        // Space = Jumping (check if also falling)
        static auto lastJumpTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        auto jumpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastJumpTime);
        
        if (jumpDuration.count() > 300) { // After 300ms, consider it falling
            newState = PlayerMovementState::Falling;
        } else {
            newState = PlayerMovementState::Jumping;
            lastJumpTime = currentTime;
        }
    } else if (isWASDPressed) {
        // Check if strafing (A or D pressed without W or S)
        bool isStrafingOnly = ((GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState('D') & 0x8000)) &&
                             !((GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('S') & 0x8000));
        
        if (isStrafingOnly) {
            newState = PlayerMovementState::Strafing;
        } else {
            // Determine if running based on sustained movement
            static auto lastMovementTime = std::chrono::steady_clock::now();
            auto currentTime = std::chrono::steady_clock::now();
            auto movementDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMovementTime);
            
            if (movementDuration.count() > 500) { // After 500ms of movement, consider it running
                newState = PlayerMovementState::Running;
            } else {
                newState = PlayerMovementState::Walking;
            }
            lastMovementTime = currentTime;
        }
    } else {
        // No movement keys pressed
        newState = PlayerMovementState::Stationary;
    }
    
    // Update movement state if changed
    PlayerMovementState currentState = GlobalState::getCurrentMovementState();
    if (currentState != newState) {
        GlobalState::setMovementState(newState);
        SAFE_ATOMIC_STORE(g_isMoving, newState != PlayerMovementState::Stationary);
        
        LOG_DEBUG_SYSTEM("Movement state changed: " + playerMovementStateToString(newState));
    }
}

// =============================================================================
// KEY INPUT PROCESSING
// =============================================================================

void processKeyInput(int vk_code, int scan_code) {
    (void)scan_code; // Suppress unused parameter warning
    
    if (!GlobalState::isSystemActive()) {
        return;
    }
    
    unsigned int vk_uint = static_cast<unsigned int>(vk_code);
    int current_mod = getCurrentModifierState();
    
    // Check for system exit
    if (isKeybindingPressed(g_keybindings.exit_vk, g_keybindings.exit_mod)) {
        LOG_SYSTEM("Exit keybinding detected");
        SAFE_ATOMIC_STORE(g_running, false);
        return;
    }
    
    // Check for emergency stop
    if (isKeybindingPressed(g_keybindings.emergency_stop_vk, g_keybindings.emergency_stop_mod)) {
        LOG_SYSTEM("Emergency stop activated!");
        SAFE_ATOMIC_STORE(g_running, false);
        SAFE_ATOMIC_STORE(g_aimAssistEnabled, false);
        SAFE_ATOMIC_STORE(g_triggerBotEnabled, false);
        return;
    }
    
    // Update movement state on any key input
    updateMovementState();
    
    // Handle specific keybindings
    if (isKeybindingPressed(g_keybindings.aim_assist_toggle_vk, g_keybindings.aim_assist_toggle_mod)) {
        bool current = SAFE_ATOMIC_LOAD(g_aimAssistEnabled);
        SAFE_ATOMIC_STORE(g_aimAssistEnabled, !current);
        LOG_SYSTEM("Aim assist " + std::string(!current ? "enabled" : "disabled"));
    }
    
    if (isKeybindingPressed(g_keybindings.trigger_bot_toggle_vk, g_keybindings.trigger_bot_toggle_mod)) {
        bool current = SAFE_ATOMIC_LOAD(g_triggerBotEnabled);
        SAFE_ATOMIC_STORE(g_triggerBotEnabled, !current);
        LOG_SYSTEM("Trigger bot " + std::string(!current ? "enabled" : "disabled"));
    }
    
    if (isKeybindingPressed(g_keybindings.stats_reset_vk, g_keybindings.stats_reset_mod)) {
        LOG_SYSTEM("Resetting statistics");
        resetGlobalStatistics();
    }
    
    if (isKeybindingPressed(g_keybindings.status_print_vk, g_keybindings.status_print_mod)) {
        LOG_SYSTEM("Printing system status");
        printSystemStatus();
    }
    
    if (isKeybindingPressed(g_keybindings.debug_toggle_vk, g_keybindings.debug_toggle_mod)) {
        bool current = SAFE_ATOMIC_LOAD(g_debugMode);
        SAFE_ATOMIC_STORE(g_debugMode, !current);
        LOG_SYSTEM("Debug mode " + std::string(!current ? "enabled" : "disabled"));
    }
    
    // Profile management
    if (isKeybindingPressed(g_keybindings.profile_next_vk, g_keybindings.profile_next_mod)) {
        int current_profile = SAFE_ATOMIC_LOAD(g_activeProfileIndex);
        int profile_count = SAFE_ATOMIC_LOAD(g_profileCount);
        int next_profile = (current_profile + 1) % std::max(1, profile_count);
        setActiveProfile(next_profile);
        LOG_SYSTEM("Switched to profile " + std::to_string(next_profile));
    }
    
    if (isKeybindingPressed(g_keybindings.profile_prev_vk, g_keybindings.profile_prev_mod)) {
        int current_profile = SAFE_ATOMIC_LOAD(g_activeProfileIndex);
        int profile_count = SAFE_ATOMIC_LOAD(g_profileCount);
        int prev_profile = (current_profile - 1 + profile_count) % std::max(1, profile_count);
        setActiveProfile(prev_profile);
        LOG_SYSTEM("Switched to profile " + std::to_string(prev_profile));
    }
    
    // Sensitivity adjustment
    if (isKeybindingPressed(g_keybindings.sens_increase_vk, g_keybindings.sens_increase_mod)) {
        double current_sens = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier);
        double new_sens = std::min(current_sens + 0.1, GlobalConstants::MAX_SENSITIVITY);
        SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, new_sens);
        LOG_SYSTEM("Sensitivity increased to " + std::to_string(new_sens));
    }
    
    if (isKeybindingPressed(g_keybindings.sens_decrease_vk, g_keybindings.sens_decrease_mod)) {
        double current_sens = SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier);
        double new_sens = std::max(current_sens - 0.1, GlobalConstants::MIN_SENSITIVITY);
        SAFE_ATOMIC_STORE(g_globalSensitivityMultiplier, new_sens);
        LOG_SYSTEM("Sensitivity decreased to " + std::to_string(new_sens));
    }
    
    // FOV adjustment
    if (isKeybindingPressed(g_keybindings.fov_increase_vk, g_keybindings.fov_increase_mod)) {
        double current_fov = SAFE_ATOMIC_LOAD(g_aimFOV);
        double new_fov = std::min(current_fov + 10.0, 500.0);
        SAFE_ATOMIC_STORE(g_aimFOV, new_fov);
        LOG_SYSTEM("Aim FOV increased to " + std::to_string(new_fov));
    }
    
    if (isKeybindingPressed(g_keybindings.fov_decrease_vk, g_keybindings.fov_decrease_mod)) {
        double current_fov = SAFE_ATOMIC_LOAD(g_aimFOV);
        double new_fov = std::max(current_fov - 10.0, 10.0);
        SAFE_ATOMIC_STORE(g_aimFOV, new_fov);
        LOG_SYSTEM("Aim FOV decreased to " + std::to_string(new_fov));
    }
    
    // Smoothing adjustment
    if (isKeybindingPressed(g_keybindings.smooth_increase_vk, g_keybindings.smooth_increase_mod)) {
        double current_smooth = SAFE_ATOMIC_LOAD(g_aimSmoothingFactor);
        double new_smooth = std::min(current_smooth + 0.05, 1.0);
        SAFE_ATOMIC_STORE(g_aimSmoothingFactor, new_smooth);
        LOG_SYSTEM("Smoothing increased to " + std::to_string(new_smooth));
    }
    
    if (isKeybindingPressed(g_keybindings.smooth_decrease_vk, g_keybindings.smooth_decrease_mod)) {
        double current_smooth = SAFE_ATOMIC_LOAD(g_aimSmoothingFactor);
        double new_smooth = std::max(current_smooth - 0.05, 0.1);
        SAFE_ATOMIC_STORE(g_aimSmoothingFactor, new_smooth);
        LOG_SYSTEM("Smoothing decreased to " + std::to_string(new_smooth));
    }
    
    // Handle smart sprint keybindings
    if (vk_code == VK_LSHIFT || vk_code == VK_RSHIFT) {
        if (isKeybindingPressed(g_keybindings.smart_sprint_left_vk, g_keybindings.smart_sprint_left_mod)) {
            LOG_DEBUG_SYSTEM("Smart sprint left activated");
        } else if (isKeybindingPressed(g_keybindings.smart_sprint_right_vk, g_keybindings.smart_sprint_right_mod)) {
            LOG_DEBUG_SYSTEM("Smart sprint right activated");
        }
        
        // Check for sliding combination
        bool isWASDPressed = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('A') & 0x8000) ||
                            (GetAsyncKeyState('S') & 0x8000) || (GetAsyncKeyState('D') & 0x8000);
        bool isCtrlPressed = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || (GetAsyncKeyState(VK_RCONTROL) & 0x8000);
        
        if (isWASDPressed && isCtrlPressed) {
            GlobalState::setMovementState(PlayerMovementState::Sliding);
        }
    }
}

// =============================================================================
// MOUSE INPUT PROCESSING
// =============================================================================

void processMouseInput(int x, int y, int delta_x, int delta_y) {
    (void)delta_x; // Suppress unused parameter warning
    (void)delta_y; // Suppress unused parameter warning
    
    if (!GlobalState::isSystemActive()) {
        return;
    }
    
    // Update global mouse position
    SAFE_ATOMIC_STORE(g_mouseX, x);
    SAFE_ATOMIC_STORE(g_mouseY, y);
    
    // Update movement state based on mouse movement
    updateMovementState();
    
    // Apply contextual movement assist if enabled
    if (GlobalState::isAssistEnabled() && !GlobalState::isSimulating()) {
        executeContextualMovementAssist();
    }
    
    // Update cursor tracking
    SAFE_ATOMIC_STORE(g_cursorX, x);
    SAFE_ATOMIC_STORE(g_cursorY, y);
}

// =============================================================================
// PREDICTIVE MOUSE MOVEMENT
// =============================================================================

void predictiveMouseMove(int target_x, int target_y, int duration_ms) {
    if (GlobalState::isSimulating()) {
        return; // Avoid recursive simulation
    }
    
    int current_x = SAFE_ATOMIC_LOAD(g_mouseX);
    int current_y = SAFE_ATOMIC_LOAD(g_mouseY);
    
    int delta_x = target_x - current_x;
    int delta_y = target_y - current_y;
    
    // Skip if movement is too small
    if (std::abs(delta_x) < 2 && std::abs(delta_y) < 2) {
        return;
    }
    
    // Calculate steps for smooth movement
    int steps = std::max(1, duration_ms / 10); // 10ms per step
    double step_x = static_cast<double>(delta_x) / steps;
    double step_y = static_cast<double>(delta_y) / steps;
    
    SAFE_ATOMIC_STORE(g_isSimulatingInput, true);
    
    for (int i = 0; i < steps; i++) {
        if (!SAFE_ATOMIC_LOAD(g_running)) break; // Stop if system is shutting down
        
        int new_x = current_x + static_cast<int>(step_x * (i + 1));
        int new_y = current_y + static_cast<int>(step_y * (i + 1));
        
        // Apply bounds checking
        int monitor_width = SAFE_ATOMIC_LOAD(g_monitorWidth);
        int monitor_height = SAFE_ATOMIC_LOAD(g_monitorHeight);
        
        new_x = std::clamp(new_x, 0, monitor_width - 1);
        new_y = std::clamp(new_y, 0, monitor_height - 1);
        
        // Simulate mouse movement (placeholder - would use actual Windows API)
        SAFE_ATOMIC_STORE(g_mouseX, new_x);
        SAFE_ATOMIC_STORE(g_mouseY, new_y);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    UPDATE_COUNTER(g_smoothedMovements);
    SAFE_ATOMIC_STORE(g_isSimulatingInput, false);
}

// =============================================================================
// SYSTEM CONTROL FUNCTIONS
// =============================================================================

bool initializeInputSystem() {
    LOG_SYSTEM("Initializing input system...");
    
    // Initialize keybindings
    initializeKeybindings();
    
    // Initialize system state
    SAFE_ATOMIC_STORE(g_isSimulatingInput, false);
    SAFE_ATOMIC_STORE(g_inputSystemActive, true);
    GlobalState::setMovementState(PlayerMovementState::Stationary);
    
    // Reset mouse position
    SAFE_ATOMIC_STORE(g_mouseX, SAFE_ATOMIC_LOAD(g_monitorWidth) / 2);
    SAFE_ATOMIC_STORE(g_mouseY, SAFE_ATOMIC_LOAD(g_monitorHeight) / 2);
    
    LOG_SYSTEM("Input system initialized successfully");
    return true;
}

void shutdownInputSystem() {
    LOG_SYSTEM("Shutting down input system...");
    
    SAFE_ATOMIC_STORE(g_isSimulatingInput, false);
    SAFE_ATOMIC_STORE(g_inputSystemActive, false);
    GlobalState::setMovementState(PlayerMovementState::Stationary);
    
    // Save keybindings configuration
    saveKeybindingsToConfig();
    
    LOG_SYSTEM("Input system shutdown complete");
}

void runInputLoop() {
    LOG_SYSTEM("Starting input processing loop...");
    
    auto lastUpdate = std::chrono::steady_clock::now();
    
    while (SAFE_ATOMIC_LOAD(g_running) && SAFE_ATOMIC_LOAD(g_inputSystemActive)) {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdate);
        
        // Update movement state continuously
        updateMovementState();
        
        // Process contextual movement assist
        if (GlobalState::isAssistEnabled() && !GlobalState::isSimulating()) {
            executeContextualMovementAssist();
        }
        
        // Update timing information
        SAFE_ATOMIC_STORE(g_deltaTime, static_cast<uint64_t>(deltaTime.count()));
        lastUpdate = currentTime;
        
        // Sleep to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LOG_SYSTEM("Input loop finished");
}

// =============================================================================
// STATUS AND MONITORING FUNCTIONS
// =============================================================================

std::string getMovementStateString() {
    PlayerMovementState state = GlobalState::getCurrentMovementState();
    return playerMovementStateToString(state);
}

void printInputSystemStatus() {
    logMessage("=== INPUT SYSTEM STATUS ===");
    logMessage("System Running: " + std::string(SAFE_ATOMIC_LOAD(g_running) ? "Yes" : "No"));
    logMessage("Input System Active: " + std::string(SAFE_ATOMIC_LOAD(g_inputSystemActive) ? "Yes" : "No"));
    logMessage("Input Simulation Active: " + std::string(GlobalState::isSimulating() ? "Yes" : "No"));
    logMessage("Current Movement State: " + getMovementStateString());
    logMessage("Mouse Position: (" + std::to_string(SAFE_ATOMIC_LOAD(g_mouseX)) + ", " + 
               std::to_string(SAFE_ATOMIC_LOAD(g_mouseY)) + ")");
    logMessage("Cursor Position: (" + std::to_string(SAFE_ATOMIC_LOAD(g_cursorX)) + ", " + 
               std::to_string(SAFE_ATOMIC_LOAD(g_cursorY)) + ")");
    logMessage("Aim Assist Enabled: " + std::string(GlobalState::isAssistEnabled() ? "Yes" : "No"));
    logMessage("Trigger Bot Enabled: " + std::string(SAFE_ATOMIC_LOAD(g_triggerBotEnabled) ? "Yes" : "No"));
    logMessage("Debug Mode: " + std::string(SAFE_ATOMIC_LOAD(g_debugMode) ? "Yes" : "No"));
    logMessage("===========================");
}

void printSystemStatus() {
    logMessage("📊 === COMPLETE SYSTEM STATUS ===");
    
    // Input system status
    printInputSystemStatus();
    
    // Aim assist status
    logMessage("--- AIM ASSIST STATUS ---");
    logMessage("Aim Assist Enabled: " + std::string(SAFE_ATOMIC_LOAD(g_aimAssistEnabled) ? "Yes" : "No"));
    logMessage("Trigger Bot Enabled: " + std::string(SAFE_ATOMIC_LOAD(g_triggerBotEnabled) ? "Yes" : "No"));
    logMessage("Silent Aim Enabled: " + std::string(SAFE_ATOMIC_LOAD(g_silentAimEnabled) ? "Yes" : "No"));
    logMessage("Prediction Enabled: " + std::string(SAFE_ATOMIC_LOAD(g_predictionEnabled) ? "Yes" : "No"));
    logMessage("Recoil Compensation: " + std::string(SAFE_ATOMIC_LOAD(g_recoilCompensationEnabled) ? "Yes" : "No"));
    logMessage("Active Profile Index: " + std::to_string(SAFE_ATOMIC_LOAD(g_activeProfileIndex)));
    
    // Current settings
    logMessage("--- CURRENT SETTINGS ---");
    logMessage("Global Sensitivity: " + std::to_string(SAFE_ATOMIC_LOAD(g_globalSensitivityMultiplier)));
    logMessage("Aim Smoothing: " + std::to_string(SAFE_ATOMIC_LOAD(g_aimSmoothingFactor)));
    logMessage("Aim FOV: " + std::to_string(SAFE_ATOMIC_LOAD(g_aimFOV)));
    logMessage("Max Target Distance: " + std::to_string(SAFE_ATOMIC_LOAD(g_maxTargetDistance)));
    logMessage("Min Confidence: " + std::to_string(SAFE_ATOMIC_LOAD(g_minConfidenceThreshold)));
    
    // Movement statistics
    logMessage("--- MOVEMENT STATISTICS ---");
    logMessage("Total Movements: " + std::to_string(SAFE_ATOMIC_LOAD(g_totalMovements)));
    logMessage("Smoothed Movements: " + std::to_string(SAFE_ATOMIC_LOAD(g_smoothedMovements)));
    logMessage("Predicted Movements: " + std::to_string(SAFE_ATOMIC_LOAD(g_predictedMovements)));
    
    // Target statistics
    logMessage("--- TARGET STATISTICS ---");
    logMessage("Targets Detected: " + std::to_string(SAFE_ATOMIC_LOAD(g_targetsDetected)));
    logMessage("Targets Acquired: " + std::to_string(SAFE_ATOMIC_LOAD(g_targetsAcquired)));
    logMessage("Current Target: " + std::string(SAFE_ATOMIC_LOAD(g_targetAcquired) ? "YES" : "NO"));
    
    // System performance
    logMessage("--- SYSTEM PERFORMANCE ---");
    logMessage("Current FPS: " + std::to_string(SAFE_ATOMIC_LOAD(g_currentFPS)));
    logMessage("Frame Count: " + std::to_string(SAFE_ATOMIC_LOAD(g_frameCount)));
    logMessage("CPU Usage: " + std::to_string(SAFE_ATOMIC_LOAD(g_cpuUsage)) + "%");
    logMessage("System State: " + getSystemStateString());
    
    // Keybindings
    logMessage("--- KEYBINDINGS ---");
    logMessage(g_keybindings.toString());
    
    logMessage("================================");
}

void testAllKeybindings() {
    logMessage("=== KEYBINDING TEST ===");
    logMessage("Exit: " + getKeybindingString(g_keybindings.exit_vk, g_keybindings.exit_mod));
    logMessage("Emergency Stop: " + getKeybindingString(g_keybindings.emergency_stop_vk, g_keybindings.emergency_stop_mod));
    logMessage("Aim Assist Toggle: " + getKeybindingString(g_keybindings.aim_assist_toggle_vk, g_keybindings.aim_assist_toggle_mod));
    logMessage("Trigger Bot Toggle: " + getKeybindingString(g_keybindings.trigger_bot_toggle_vk, g_keybindings.trigger_bot_toggle_mod));
    logMessage("Smart Sprint Left: " + getKeybindingString(g_keybindings.smart_sprint_left_vk, g_keybindings.smart_sprint_left_mod));
    logMessage("Smart Sprint Right: " + getKeybindingString(g_keybindings.smart_sprint_right_vk, g_keybindings.smart_sprint_right_mod));
    logMessage("Profile Next: " + getKeybindingString(g_keybindings.profile_next_vk, g_keybindings.profile_next_mod));
    logMessage("Profile Previous: " + getKeybindingString(g_keybindings.profile_prev_vk, g_keybindings.profile_prev_mod));
    logMessage("Stats Reset: " + getKeybindingString(g_keybindings.stats_reset_vk, g_keybindings.stats_reset_mod));
    logMessage("Status Print: " + getKeybindingString(g_keybindings.status_print_vk, g_keybindings.status_print_mod));
    logMessage("Debug Toggle: " + getKeybindingString(g_keybindings.debug_toggle_vk, g_keybindings.debug_toggle_mod));
    logMessage("======================");
}

// =============================================================================
// INPUT SIMULATION FUNCTIONS
// =============================================================================

void simulateKeySequence(const std::vector<unsigned int>& keys, int delay_ms) {
    SAFE_ATOMIC_STORE(g_isSimulatingInput, true);
    
    for (unsigned int key : keys) {
        if (!SAFE_ATOMIC_LOAD(g_running)) break;
        
        // Simulate key press (placeholder - would use actual Windows API)
        LOG_DEBUG_SYSTEM("Simulating key press: " + getVirtualKeyName(key));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
    
    SAFE_ATOMIC_STORE(g_isSimulatingInput, false);
}

void simulateMouseClick(int x, int y, bool left_click) {
    SAFE_ATOMIC_STORE(g_isSimulatingInput, true);
    
    // Move to position first
    SAFE_ATOMIC_STORE(g_mouseX, x);
    SAFE_ATOMIC_STORE(g_mouseY, y);
    
    // Simulate click (placeholder)
    LOG_DEBUG_SYSTEM("Simulating " + std::string(left_click ? "left" : "right") + 
                     " click at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    SAFE_ATOMIC_STORE(g_isSimulatingInput, false);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

bool isKeyPressed(unsigned int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

std::vector<unsigned int> getCurrentPressedKeys() {
    std::vector<unsigned int> pressed_keys;
    
    // Check common keys
    for (unsigned int vk = 8; vk <= 255; vk++) {
        if (isKeyPressed(vk)) {
            pressed_keys.push_back(vk);
        }
    }
    
    return pressed_keys;
}

void logInputEvent(const std::string& event) {
    if (SAFE_ATOMIC_LOAD(g_debugMode)) {
        logMessage("[INPUT] " + event);
    }
}

bool isInputSystemHealthy() {
    return SAFE_ATOMIC_LOAD(g_inputSystemActive) && 
           !GlobalState::isSimulating() && 
           GlobalState::isSystemActive();
}

std::string getInputSystemStatus() {
    if (!SAFE_ATOMIC_LOAD(g_inputSystemActive)) return "INACTIVE";
    if (GlobalState::isSimulating()) return "SIMULATING";
    if (!GlobalState::isSystemActive()) return "SYSTEM_INACTIVE";
    return "ACTIVE";
}