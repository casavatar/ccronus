// description: This code handles raw mouse and keyboard input for a game, managing actions like aiming, firing, and tactical movements based on keybindings and weapon profiles.
// It uses atomic variables for thread-safe state management and integrates with a performance optimization system to execute tasks based on user input.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "input.h"
#include "globals.h"
#include "movements.h"
#include "assist.h"
#include "config.h"
#include "profiles.h"
#include "systems.h"

// External state variables (defined in movements.cpp)
extern std::atomic<bool> isAimingDownSights; // Flag to indicate if the user is aiming down sights
extern std::atomic<bool> inCombatMode; // Flag to indicate if the user is in combat mode
extern std::atomic<bool> isExecutingMovement; // Flag to indicate if a movement is currently being executed
extern std::atomic<bool> isControlledAutoFiring; // Flag to indicate if controlled automatic firing is active
extern std::atomic<bool> isRapidFiring; // Flag to indicate if rapid firing is active
extern std::atomic<bool> isTacticalFiring; // Extern for the new Tactical fire mode
extern std::atomic<bool> isSprintingForward; // Flag to indicate if the user is sprinting forward
extern std::chrono::steady_clock::time_point g_sprintStartTime; // Timestamp for when the sprint started

// Function to handle raw mouse input
void HandleRawMouseInput(const RAWMOUSE& rawMouse) {
    if (g_performanceOpt) g_performanceOpt->registerInputEvent();
    
    // --- LÓGICA DE APUNTADO (ADS) ---
    if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
        isAimingDownSights = true;
        if (g_assistEnabled.load()) {
            logMessage("ADS Active - Aim Assist Enhanced");
            g_performanceOpt->addTask([](){
                while(isAimingDownSights.load() && g_running.load() && g_assistEnabled.load()) {
                    enhancedHeadshotAimAssist();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                logMessage("ADS Inactive - Aim Assist Stopped");
            });
        }
    } else if (rawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
        isAimingDownSights = false;
    }

    // Shooting logic
    if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        inCombatMode = true;
        handleEnhancedKeyPress(VK_LBUTTON); // Start firing mode
    } 
    else if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
        inCombatMode = false;
        // Stop all automatic firing modes centrally
        isControlledAutoFiring = false;
        isRapidFiring = false;
        isTacticalFiring = false; // Stop tactical firing when left mouse is released
    }
}

void HandleRawKeyboardInput(const RAWKEYBOARD& rawKeyboard) {
    if (g_performanceOpt) g_performanceOpt->registerInputEvent();

    // --- Contextual Movement Logic ---
    // Check for combined W + Shift press to detect forward sprint
    if (rawKeyboard.VKey == 'W' && (GetAsyncKeyState(VK_LSHIFT) & 0x8000)) {
        if (!isSprintingForward.load()) {
            isSprintingForward = true;
            g_sprintStartTime = std::chrono::steady_clock::now();
        }
    }

    if (rawKeyboard.Flags & RI_KEY_BREAK) {
        // Key up
        if (rawKeyboard.VKey == 'W' || rawKeyboard.VKey == VK_LSHIFT) {
            isSprintingForward = false;
        }
    } else {
        // Key down
        handleEnhancedKeyPress(rawKeyboard.VKey);
    }
}

void handleEnhancedKeyPress(int vk) {
    if(isExecutingMovement.load()) return;

    bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
    bool altPressed = GetAsyncKeyState(VK_LMENU) & 0x8000;
    
    int current_mod = 0;
    if(ctrlPressed) current_mod = VK_CONTROL;
    else if(altPressed) current_mod = VK_LMENU;

    // --- REFACTORED FIRING LOGIC ---
    if (vk == VK_LBUTTON && current_mod == 0) {
        if (g_weaponProfiles.empty()) return;
        const auto& profile = g_weaponProfiles[g_activeProfileIndex.load()];
        
        if (profile.fireMode == "Rapid") {
            g_performanceOpt->addTask(enhancedIntelligentRapidFire);
        } else if (profile.fireMode == "Controlled") {
            g_performanceOpt->addTask(controlledAutomaticFire);
        } else if (profile.fireMode == "Tactical") { // <-- NEW CONDITION
            g_performanceOpt->addTask(tacticalFire);
        } else if (profile.fireMode == "Single") {
            // For "Single" mode, we do nothing.
            // The click will be processed natively by the game.
            // This is intentional.
        }
        return;
    }

    if (vk == g_keybindings.contextual_movement_assist_vk && current_mod == g_keybindings.contextual_movement_assist_mod) {
        auto time_since_sprint_start = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - g_sprintStartTime
        ).count();
        
        // Only trigger if sprinting forward for at least 500ms
        if (isSprintingForward.load() && time_since_sprint_start > 500) {
            g_performanceOpt->addTask(executeContextualStrafeJump);
        }
        return; // Prevent other keybinds from firing
    }
        
    // Asignaciones de movimiento al pool de hilos
    if (vk == g_keybindings.smart_sprint_left_vk && current_mod == g_keybindings.smart_sprint_left_mod) g_performanceOpt->addTask([]{ executeSmartDiagonalSprint(true); });
    else if (vk == g_keybindings.smart_sprint_right_vk && current_mod == g_keybindings.smart_sprint_right_mod) g_performanceOpt->addTask([]{ executeSmartDiagonalSprint(false); });
    else if (vk == g_keybindings.predictive_slide_vk && current_mod == g_keybindings.predictive_slide_mod) g_performanceOpt->addTask(executePredictiveSlide);
    else if (vk == g_keybindings.dive_back_vk && current_mod == g_keybindings.dive_back_mod) g_performanceOpt->addTask(executeAntiDetectionDiveBack);
    else if (vk == g_keybindings.corner_bounce_left_vk && current_mod == g_keybindings.corner_bounce_left_mod) g_performanceOpt->addTask([]{ executeSmartCornerBounce(true); });
    else if (vk == g_keybindings.corner_bounce_right_vk && current_mod == g_keybindings.corner_bounce_right_mod) g_performanceOpt->addTask([]{ executeSmartCornerBounce(false); });
    else if (vk == g_keybindings.cutback_vk && current_mod == g_keybindings.cutback_mod) g_performanceOpt->addTask(executePredictiveCutback);
    else if (vk == g_keybindings.dropshot_supine_slide_vk && current_mod == g_keybindings.dropshot_supine_slide_mod) g_performanceOpt->addTask(executeDropShotSupineSlide);
    else if (vk == g_keybindings.slide_cancel_directional_vk && current_mod == g_keybindings.slide_cancel_directional_mod) g_performanceOpt->addTask(executeSlideCancelDirectional);
    else if (vk == g_keybindings.dive_directional_intelligent_vk && current_mod == g_keybindings.dive_directional_intelligent_mod) g_performanceOpt->addTask(executeDiveDirectionalIntelligent);
    else if (vk == g_keybindings.omnidirectional_slide_vk && current_mod == g_keybindings.omnidirectional_slide_mod) g_performanceOpt->addTask(executeOmnidirectionalSlide);
    else if (vk == g_keybindings.movement_test_vk && current_mod == g_keybindings.movement_test_mod) g_performanceOpt->addTask(executeMovementTest);
}