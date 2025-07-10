// description: This code handles raw mouse and keyboard input for a game, managing actions like aiming, firing, and tactical movements based on keybindings and weapon profiles.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.8.1
// date: 2025-06-26
// project: Tactical Aim Assist

#include "input.h"
#include "globals.h"
#include "movements.h"
#include "assist.h"
#include "config.h"
#include "profiles.h"
#include "systems.h"

// Extern state variables
extern std::atomic<bool> isAimingDownSights; // Indicates if the player is aiming down sights
extern std::atomic<bool> inCombatMode; // Indicates if the player is in combat mode 
extern std::atomic<bool> isExecutingMovement; // Indicates if a movement action is currently being executed
extern std::atomic<bool> isControlledAutoFiring; // Indicates if controlled auto-firing is active
extern std::atomic<bool> isRapidFiring; // Indicates if rapid firing is active
extern std::atomic<bool> isTacticalFiring; // Indicates if tactical firing is active
extern std::atomic<bool> g_isSimulatingInput; // Indicates if input simulation is active
extern std::atomic<bool> isSprintingForward; // Indicates if the player is sprinting forward
extern std::atomic<PlayerMovementState> g_playerMovementState; // Player movement state
// Extern performance optimizer
extern std::chrono::steady_clock::time_point g_sprintStartTime; // Time when sprint started

void HandleRawMouseInput(const RAWMOUSE& rawMouse) {
    if (g_isSimulatingInput.load()) {
        return;
    }

    if (g_performanceOpt) g_performanceOpt->registerInputEvent();
    
    // logic for ads (aiming down sights)
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

    // logic for combat mode and firing
    if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        inCombatMode = true;
        handleEnhancedKeyPress(VK_LBUTTON);
    } 
    else if (rawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
        inCombatMode = false;
        isControlledAutoFiring = false;
        isRapidFiring = false;
        isTacticalFiring = false;
    }
}

void HandleRawKeyboardInput(const RAWKEYBOARD& rawKeyboard) {
    if (g_performanceOpt) g_performanceOpt->registerInputEvent();

    // --- NEW: Player State Detection Logic ---
    bool w_down = GetAsyncKeyState('W') & 0x8000;
    bool a_down = GetAsyncKeyState('A') & 0x8000;
    bool s_down = GetAsyncKeyState('S') & 0x8000;
    bool d_down = GetAsyncKeyState('D') & 0x8000;
    bool shift_down = GetAsyncKeyState(VK_LSHIFT) & 0x8000;

    if (w_down && shift_down) {
        g_playerMovementState = PlayerMovementState::Sprinting;
    } else if (w_down || s_down) {
        g_playerMovementState = PlayerMovementState::Walking;
    } else if (a_down || d_down) {
        g_playerMovementState = PlayerMovementState::Strafing;
    } else {
        g_playerMovementState = PlayerMovementState::Stationary;
    }

    if (!(rawKeyboard.Flags & RI_KEY_BREAK)) { // Key down
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
    
    // ... (System and profile switching keys)

    if (vk == g_keybindings.contextual_movement_assist_vk && current_mod == g_keybindings.contextual_movement_assist_mod) {
        auto time_since_sprint_start = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - g_sprintStartTime
        ).count();
        
        if (isSprintingForward.load() && time_since_sprint_start > 500) {
            g_performanceOpt->addTask(executeContextualStrafeJump);
        }
        return;
    }

    if (vk == VK_LBUTTON && current_mod == 0) {
        if (g_weaponProfiles.empty()) return;
        const auto& profile = g_weaponProfiles[g_activeProfileIndex.load()];
        
        if (profile.fireMode == "Rapid") {
            g_performanceOpt->addTask(enhancedIntelligentRapidFire);
        } else if (profile.fireMode == "Controlled") {
            g_performanceOpt->addTask(controlledAutomaticFire);
        } else if (profile.fireMode == "Tactical") {
            g_performanceOpt->addTask(tacticalFire);
        }
        else if (profile.fireMode == "Automatic" || profile.fireMode == "Single") {
            // Intentionally empty. The click is passed through to the game.
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