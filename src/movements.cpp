// description: Implements mouse and keyboard simulation functions, including fire modes and complex tactical movements for the game enhancement tool.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.3.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "movements.h"
#include "globals.h"
#include "systems.h"
#include "profiles.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

// FIX: Removed the definitions of these variables. They are now only declared as extern
// because their single definition resides in globals.cpp. This resolves the 'multiple definition' linker error.
extern std::atomic<bool> isRapidFiring; // Indicates if the rapid firing mode is currently active, preventing overlapping actions
extern std::atomic<bool> isControlledAutoFiring; // Indicates if the controlled automatic firing mode is currently active, preventing overlapping actions
extern std::atomic<bool> isTacticalFiring; // Indicates if the tactical firing mode is currently active, preventing overlapping actions
extern std::atomic<bool> g_isSimulatingInput; // Indicates if input simulation is active

// Global state variables for movement logic
std::atomic<bool> isExecutingMovement(false); // Indicates if a movement action is currently being executed, preventing overlapping actions
std::atomic<bool> isAimingDownSights(false); // Indicates if the player is currently aiming down sights, affecting movement and firing behavior
std::atomic<bool> inCombatMode(false); // Indicates if the player is in combat mode, affecting movement and firing behavior

int recoilIndex = 0; // Index for tracking recoil patterns in firing modes

// Random number generator for smart randomization  
int smartRandom(int base, int variance) {
    if (variance <= 0) return base;
    std::uniform_int_distribution<> dist(base, base + variance);
    return dist(gen);
}

void smartKey(WORD key, int baseDurationMs = 20) {
    if (g_antiDetection) {
        g_antiDetection->registerAction(key);
        if (g_antiDetection->needsMicroPause()) g_antiDetection->executeMicroPause();
    }
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = key;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = key;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;

    g_isSimulatingInput = true; // --- FIX: Announce we are simulating input ---
    SendInput(1, &input[0], sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(baseDurationMs, 5)));
    SendInput(1, &input[1], sizeof(INPUT));
    g_isSimulatingInput = false;
}


void smartMouseClick() {
    if (g_antiDetection) g_antiDetection->registerAction(VK_LBUTTON);
    
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    g_isSimulatingInput = true; // --- FIX: Announce we are simulating input ---
    SendInput(2, input, sizeof(INPUT)); // Send both down and up events in one go
    g_isSimulatingInput = false; // --- FIX: Announce simulation is over ---
}


// --- Implementaciones de Movimiento del Ratón ---
// ... (fluidMouseMove, arcMouseMove, acceleratedMouseMove, predictiveMouseMove functions without changes) ...
void fluidMouseMove(int dx, int dy, int steps) {
    if (dx == 0 && dy == 0) return;
    
    FluidMovementSystem fms;
    POINT current;
    GetCursorPos(&current);
    POINT target = {current.x + dx, current.y + dy};
    
    std::vector<POINT> path = fms.generateMovementPath(current, target, steps);
    
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i].x != 0 || path[i].y != 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = path[i].x;
            input.mi.dy = path[i].y;
            SendInput(1, &input, sizeof(INPUT));
        }
        
        double progress = static_cast<double>(i) / path.size();
        double velocity = fms.getVelocityProfile(progress);
        int delay = static_cast<int>(5.0 + (1.0 - velocity) * 3.0);
        
        if (g_antiDetection) {
            delay = std::max(delay, g_antiDetection->getHumanDelay() / 3);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

void arcMouseMove(int dx, int dy, double arcHeight) {
    if (dx == 0 && dy == 0) return;
    FluidMovementSystem fms;
    int steps = std::max(8, static_cast<int>(sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy) / 30));
    
    POINT current;
    GetCursorPos(&current);
    
    for (int i = 0; i <= steps; ++i) {
        double progress = static_cast<double>(i) / steps;
        double easedProgress = fms.getVelocityProfile(progress);
        
        int targetX_step = static_cast<int>(dx * easedProgress);
        int targetY_step = static_cast<int>(dy * easedProgress);
        
        double arcOffset = arcHeight * sin(progress * M_PI) * sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy);
        
        double angle = atan2(static_cast<double>(dy), static_cast<double>(dx)) + M_PI / 2.0;
        targetX_step += static_cast<int>(cos(angle) * arcOffset);
        targetY_step += static_cast<int>(sin(angle) * arcOffset);
        
        if (i > 0) {
            double prev_progress = static_cast<double>(i-1) / steps;
            double prev_easedProgress = fms.getVelocityProfile(prev_progress);

            int prev_targetX_step = static_cast<int>(dx * prev_easedProgress);
            int prev_targetY_step = static_cast<int>(dy * prev_easedProgress);
            
            double prev_arcOffset = arcHeight * sin(prev_progress * M_PI) * sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy);
            prev_targetX_step += static_cast<int>(cos(angle) * prev_arcOffset);
            prev_targetY_step += static_cast<int>(sin(angle) * prev_arcOffset);

            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = targetX_step - prev_targetX_step;
            input.mi.dy = targetY_step - prev_targetY_step;
            
            SendInput(1, &input, sizeof(INPUT));
        }
        
        int delay = 4 + static_cast<int>((1.0 - easedProgress) * 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

void acceleratedMouseMove(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    FluidMovementSystem fms;
    int steps = std::max(6, static_cast<int>(sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy) / 40));
    
    POINT lastReportedAbsMove = {0, 0};
    
    for (int i = 0; i < steps; ++i) {
        double progress = static_cast<double>(i) / (steps - 1);
        double velocity = fms.getVelocityProfile(progress);
        
        int targetAbsX = static_cast<int>(dx * velocity);
        int targetAbsY = static_cast<int>(dy * velocity);
        
        int stepX = targetAbsX - lastReportedAbsMove.x;
        int stepY = targetAbsY - lastReportedAbsMove.y;
        
        if (stepX != 0 || stepY != 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = stepX;
            input.mi.dy = stepY;
            
            SendInput(1, &input, sizeof(INPUT));
            
            lastReportedAbsMove.x += stepX;
            lastReportedAbsMove.y += stepY;
        }
        
        int delay = static_cast<int>(6.0 * (1.0 + (1.0 - velocity)));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

void predictiveMouseMove(int dx, int dy, int movementType) {
    if (dx == 0 && dy == 0) return;
    
    double smoothingFactor = 1.0;
    if (g_smoothingSystem) {
        // FIX: The getSmoothingFactor function now requires a third argument for target velocity.
        // Since we don't have target velocity information here (it's a generic mouse move),
        // we pass a neutral value of 0.0.
        smoothingFactor = g_smoothingSystem->getSmoothingFactor(inCombatMode.load(), (abs(dx) < 20 && abs(dy) < 20), 0.0);
    }
    
    dx = static_cast<int>(dx * smoothingFactor);
    dy = static_cast<int>(dy * smoothingFactor);
    
    double humanAccuracy = g_antiDetection ? g_antiDetection->getHumanAccuracy() : 1.0;
    
    if (humanAccuracy < 1.0) {
        std::normal_distribution<double> errorDist(0.0, (1.0 - humanAccuracy) * 2.0);
        dx += static_cast<int>(errorDist(gen));
        dy += static_cast<int>(errorDist(gen));
    }
    
    int distance = static_cast<int>(sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));
    
    if (movementType == 0) {
        if (distance > 100) movementType = 1;
        else if (distance > 40) movementType = 2;
        else movementType = 3;
    }
    
    switch (movementType) {
        case 1: fluidMouseMove(dx, dy, std::max(8, distance / 20)); break;
        case 2: arcMouseMove(dx, dy, 0.15 + smartRandom(0, 10) / 100.0); break;
        case 3: acceleratedMouseMove(dx, dy); break;
        default: fluidMouseMove(dx, dy, 12); break;
    }
}

// Controlled and Fast Fire Implementations
// (Controlledautomaticfire and enhancedintelligentrapidfire Functions Without Changes)

// Controlled Automatic Fire Implementation
void controlledAutomaticFire() {
    bool expected = false;
    // If the variable is already 'True' (the function is already in execution), compare_exchange_strong returns 'false' and we leave.
    // If it is 'false', it changes it to 'True' and returns 'True', allowing the function to continue.
    if (!isControlledAutoFiring.compare_exchange_strong(expected, true)) {
        return;
    }
    
    logMessage("Controlled Automatic Fire Active");
    const auto& currentProfile = g_weaponProfiles[g_activeProfileIndex.load()];
    recoilIndex = 0;

    while (g_running.load() && isControlledAutoFiring.load()) {
        smartMouseClick();

        int baseRecoil = currentProfile.recoilPattern[recoilIndex % currentProfile.recoilPattern.size()];
        std::normal_distribution<double> recoilVariation(0.0, 1.5);
        int adjustedRecoil = baseRecoil + static_cast<int>(recoilVariation(gen)) + (recoilIndex / 5);
        int horizontalRecoil = (recoilIndex > 3) ? static_cast<int>(sin(recoilIndex * 0.2) * 2) : 0;
        
        if (g_predictiveAim && g_predictiveAim->getPredictionConfidence() > 0.3) {
            POINT predicted = g_predictiveAim->getPredictedTarget();
            POINT current;
            GetCursorPos(&current);
            int predictionAdjustX = static_cast<int>((predicted.x - current.x) * 0.05);
            int predictionAdjustY = static_cast<int>((predicted.y - current.y) * 0.05);
            horizontalRecoil += predictionAdjustX;
            adjustedRecoil += predictionAdjustY;
        }

        predictiveMouseMove(horizontalRecoil, adjustedRecoil, 3);
        recoilIndex++;

        int finalDelay = smartRandom(currentProfile.fireDelayBase, currentProfile.fireDelayVariance);     
        std::this_thread::sleep_for(std::chrono::milliseconds(finalDelay));
        
        if (g_predictiveAim && recoilIndex % 3 == 0) {
            g_predictiveAim->updateCursorHistory();
        }
    }
    recoilIndex = 0;
    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::IDLE);
    }
    logMessage("Controlled Automatic Fire Inactive");
}

void enhancedIntelligentRapidFire() {
    // Atomic guard to ensure only one instance of the loop runs.
    bool expected = false;
    if (!isRapidFiring.compare_exchange_strong(expected, true)) {
        return;
    }
    
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    logMessage("Smart Rapid Fire Active");
    recoilIndex = 0;

    const auto& currentProfile = g_weaponProfiles[g_activeProfileIndex.load()];
    static std::chrono::steady_clock::time_point firingStartTime, lastEvasiveMove;
    static bool wasFiring = false;
    
    if (!wasFiring) {
        firingStartTime = std::chrono::steady_clock::now();
        wasFiring = true;
    }

    while (g_running.load() && isRapidFiring.load()) {
        smartMouseClick();
        // ... (resto de la lógica del bucle sin cambios) ...
        int finalDelay = smartRandom(currentProfile.fireDelayBase, currentProfile.fireDelayVariance);
        std::this_thread::sleep_for(std::chrono::milliseconds(finalDelay));
    }
    
    wasFiring = false;
    recoilIndex = 0;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::IDLE);
    logMessage("Smart Rapid Fire Inactive");
}

/**
 * @brief Handles the new 'Tactical' firing mode.
 *
 * This mode is designed for high-recoil weapons, providing a fast but controlled fire rate.
 * It uses a specific recoil pattern that increases vertical recoil with each shot
 * and a more pronounced horizontal recoil compared to other modes.
 * An atomic guard ensures that only one instance of this firing loop runs at a time.
 */
void tacticalFire() {
    // Atomic guard to ensure only one instance of the loop runs.
    // If the variable is already 'true' (function is running), compare_exchange_strong returns 'false' and we exit.
    // If it's 'false', it's set to 'true' and the function proceeds.
    bool expected = false;
    if (!isTacticalFiring.compare_exchange_strong(expected, true)) {
        return;
    }

    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    }
    logMessage("Tactical Fire Active");
    
    const auto& currentProfile = g_weaponProfiles[g_activeProfileIndex.load()];
    recoilIndex = 0;

    // Main firing loop, continues as long as the application is running and the fire key is held.
    while (g_running.load() && isTacticalFiring.load()) {
        smartMouseClick();

        // Specific recoil logic for Tactical mode.
        // It's designed for high-recoil weapons.
        int baseRecoil = currentProfile.recoilPattern[recoilIndex % currentProfile.recoilPattern.size()];
        // Vertical recoil increases with each shot to simulate difficulty in control.
        int adjustedRecoil = baseRecoil + (recoilIndex * 2);
        // Horizontal recoil is more pronounced than in other modes, simulating instability.
        int horizontalRecoil = (recoilIndex > 2 && recoilIndex % 4 != 0) ? smartRandom(-3, 3) : smartRandom(-1, 1);

        predictiveMouseMove(horizontalRecoil, adjustedRecoil, 3);
        recoilIndex++;

        // Specific delay logic for Tactical mode.
        // It's faster than single-shot but slower than rapid-fire.
        int finalDelay = smartRandom(currentProfile.fireDelayBase, currentProfile.fireDelayVariance) + 15;
        std::this_thread::sleep_for(std::chrono::milliseconds(finalDelay));
    }

    // Cleanup after the loop finishes.
    recoilIndex = 0;
    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::IDLE);
    }
    logMessage("Tactical Fire Inactive");
}

// Tactical movements implementations
void executeSmartDiagonalSprint(bool leftDirection) {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement(leftDirection ? "Smart Sprint Left" : "Smart Sprint Right");
    
    INPUT sprintInput = {}; sprintInput.type = INPUT_KEYBOARD; sprintInput.ki.wVk = VK_LSHIFT; SendInput(1, &sprintInput, sizeof(INPUT));
    INPUT forward = {}; forward.type = INPUT_KEYBOARD; forward.ki.wVk = 'W'; SendInput(1, &forward, sizeof(INPUT));
    INPUT diagonal = {}; diagonal.type = INPUT_KEYBOARD; diagonal.ki.wVk = leftDirection ? 'A' : 'D'; SendInput(1, &diagonal, sizeof(INPUT));
    
    for (int i = 0; i < 3; i++) {
        int deltaX = leftDirection ? smartRandom(-60, -40) : smartRandom(40, 60);
        int deltaY = smartRandom(-15, 10);
        predictiveMouseMove(deltaX, deltaY, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(50, 80)));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(80, 120)));
    
    forward.ki.dwFlags = KEYEVENTF_KEYUP; diagonal.ki.dwFlags = KEYEVENTF_KEYUP; sprintInput.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &forward, sizeof(INPUT)); SendInput(1, &diagonal, sizeof(INPUT)); SendInput(1, &sprintInput, sizeof(INPUT));
    
    predictiveMouseMove(smartRandom(-10, 10), smartRandom(-5, 5), 3);
    if (g_momentumSys) g_momentumSys->addMomentum(0.1);

    isExecutingMovement = false;
}

void executePredictiveSlide() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Tactical Slide");
    INPUT shiftInput = {}; shiftInput.type = INPUT_KEYBOARD; shiftInput.ki.wVk = VK_LSHIFT; SendInput(1, &shiftInput, sizeof(INPUT));
    smartKey('W', smartRandom(30, 50));
    smartKey('C', smartRandom(60, 90));
    for (int i = 0; i < 3; i++) {
        int deltaX = smartRandom(-50, 50); int deltaY = smartRandom(-10, 5);
        predictiveMouseMove(deltaX, deltaY, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(40, 60)));
    }
    shiftInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &shiftInput, sizeof(INPUT));
    predictiveMouseMove(smartRandom(-15, 15), smartRandom(-8, 8), 3);
    if (g_momentumSys) g_momentumSys->addMomentum(0.2);
    isExecutingMovement = false;
}

void executeAntiDetectionDiveBack() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Tactical Dive Back");
    INPUT backward = {}; backward.type = INPUT_KEYBOARD; backward.ki.wVk = 'S'; SendInput(1, &backward, sizeof(INPUT));
    smartKey('C', smartRandom(60, 90));
    for (int i = 0; i < 4; i++) {
        int deltaX = smartRandom(-70, 70); int deltaY = smartRandom(-20, 10);
        predictiveMouseMove(deltaX, deltaY, 1);
        int humanDelay = g_antiDetection ? g_antiDetection->getHumanDelay() : 30;
        std::this_thread::sleep_for(std::chrono::milliseconds(humanDelay + 20));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(60, 100)));
    backward.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &backward, sizeof(INPUT));
    predictiveMouseMove(smartRandom(-20, 20), smartRandom(-10, 10), 2);
    if (g_momentumSys) g_momentumSys->addMomentum(0.15);
    isExecutingMovement = false;
}

void executeSmartCornerBounce(bool leftCorner) {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement(leftCorner ? "Corner Bounce Left" : "Corner Bounce Right");
    INPUT shiftInput = {}; shiftInput.type = INPUT_KEYBOARD; shiftInput.ki.wVk = VK_LSHIFT; SendInput(1, &shiftInput, sizeof(INPUT));
    smartKey(leftCorner ? 'A' : 'D', smartRandom(40, 60));
    predictiveMouseMove(leftCorner ? smartRandom(-55, -35) : smartRandom(35, 55), smartRandom(-10, 5), 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(30, 50)));
    smartKey(leftCorner ? 'D' : 'A', smartRandom(40, 60));
    predictiveMouseMove(leftCorner ? smartRandom(35, 60) : smartRandom(-60, -35), smartRandom(-15, 10), 2);
    shiftInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &shiftInput, sizeof(INPUT));
    predictiveMouseMove(smartRandom(-20, 20), smartRandom(-10, 10), 2);
    if (g_momentumSys) g_momentumSys->addMomentum(0.25);
    isExecutingMovement = false;
}

void executePredictiveCutback() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Predictive Cutback");
    INPUT shiftInput = {}; shiftInput.type = INPUT_KEYBOARD; shiftInput.ki.wVk = VK_LSHIFT; SendInput(1, &shiftInput, sizeof(INPUT));
    smartKey('W', smartRandom(30, 50));
    predictiveMouseMove(smartRandom(-30, 30), smartRandom(-10, 5), 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(30, 50)));
    smartKey('S', smartRandom(30, 50));
    bool turnRight = smartRandom(0, 1);
    int turnAmount = turnRight ? smartRandom(90, 140) : smartRandom(-140, -90);
    predictiveMouseMove(turnAmount, smartRandom(-20, 15), 1);
    shiftInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &shiftInput, sizeof(INPUT));
    predictiveMouseMove(smartRandom(-25, 25), smartRandom(-15, 15), 2);
    if (g_momentumSys) g_momentumSys->addMomentum(0.3);
    isExecutingMovement = false;
}

void executeDropShotSupineSlide() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    g_feedbackSys->startMovement("Drop-Shot Supine Slide", 1200);
    if (g_momentumSys) { g_momentumSys->updateMovementState(true, false, false, false); g_momentumSys->addMomentum(0.4); }
    g_feedbackSys->logDirectionalHint("Sprint Forward", "Building momentum");
    INPUT sprintInput = {}; sprintInput.type = INPUT_KEYBOARD; sprintInput.ki.wVk = VK_LSHIFT; SendInput(1, &sprintInput, sizeof(INPUT));
    INPUT forward = {}; forward.type = INPUT_KEYBOARD; forward.ki.wVk = 'W'; SendInput(1, &forward, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(80, 120)));
    g_feedbackSys->updateProgress(25);
    g_feedbackSys->logDirectionalHint("Slide Activation", "Tactical sprint -> slide");
    INPUT slideInput = {}; slideInput.type = INPUT_KEYBOARD; slideInput.ki.wVk = 'C'; SendInput(1, &slideInput, sizeof(INPUT));
    for (int i = 0; i < 4; i++) {
        bool leftSlide = smartRandom(0, 1);
        POINT slideDirection = { leftSlide ? smartRandom(-80, -60) : smartRandom(60, 80), smartRandom(-15, 5) };
        if (g_momentumSys) { slideDirection = g_momentumSys->getOptimalDirection(slideDirection); g_momentumSys->setLastDirection(slideDirection); }
        predictiveMouseMove(slideDirection.x, slideDirection.y, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(60, 90)));
    }
    g_feedbackSys->updateProgress(50);
    g_feedbackSys->logDirectionalHint("Supine Transition", "Slide -> prone position");
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(100, 150)));
    slideInput.ki.dwFlags = 0; SendInput(1, &slideInput, sizeof(INPUT)); // Hold 'C'
    for (int i = 0; i < 6; i++) {
        double angle = i * M_PI / 3.0;
        int circularX = static_cast<int>(cos(angle) * smartRandom(30, 50));
        int circularY = static_cast<int>(sin(angle) * smartRandom(15, 25));
        predictiveMouseMove(circularX, circularY, 3);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(40, 70)));
    }
    g_feedbackSys->updateProgress(75);
    g_feedbackSys->logDirectionalHint("Combat Ready", "Supine firing position");
    INPUT fire = {}; fire.type = INPUT_MOUSE; fire.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &fire, sizeof(INPUT));
    for (int i = 0; i < 3; i++) {
        smartMouseClick();
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(80, 120)));
    }
    fire.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &fire, sizeof(INPUT));
    g_feedbackSys->updateProgress(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(200, 300)));
    forward.ki.dwFlags = KEYEVENTF_KEYUP; sprintInput.ki.dwFlags = KEYEVENTF_KEYUP; slideInput.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &forward, sizeof(INPUT)); SendInput(1, &slideInput, sizeof(INPUT)); SendInput(1, &sprintInput, sizeof(INPUT));
    smartKey(VK_SPACE, smartRandom(60, 90));
    predictiveMouseMove(smartRandom(-40, 40), smartRandom(-20, 10), 2);
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}

void executeSlideCancelDirectional() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Slide-Cancel Direccional", 1000);
    bool hasMomentum = g_momentumSys && g_momentumSys->canChainMovement();
    g_feedbackSys->logDirectionalHint("Sprint Start", hasMomentum ? "Chaining with momentum" : "Building new momentum");
    INPUT sprintInput = {}; sprintInput.type = INPUT_KEYBOARD; sprintInput.ki.wVk = VK_LSHIFT; SendInput(1, &sprintInput, sizeof(INPUT));
    INPUT forward = {}; forward.type = INPUT_KEYBOARD; forward.ki.wVk = 'W'; SendInput(1, &forward, sizeof(INPUT));
    bool initialLeft = smartRandom(0, 1);
    smartKey(initialLeft ? 'A' : 'D', smartRandom(40, 70));
    POINT initialMove = { initialLeft ? smartRandom(-70, -50) : smartRandom(50, 70), smartRandom(-10, 5) };
    if (g_momentumSys) { g_momentumSys->updateMovementState(true, false, false, false); initialMove = g_momentumSys->getOptimalDirection(initialMove); g_momentumSys->addMomentum(0.3); }
    predictiveMouseMove(initialMove.x, initialMove.y, 1);
    g_feedbackSys->updateProgress(25);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(100, 150)));
    g_feedbackSys->logDirectionalHint("Slide Activation", "Speed -> slide transition");
    INPUT slideInput = {}; slideInput.type = INPUT_KEYBOARD; slideInput.ki.wVk = 'C'; SendInput(1, &slideInput, sizeof(INPUT));
    for (int i = 0; i < 3; i++) {
        int slideX = initialLeft ? smartRandom(40, 70) : smartRandom(-70, -40); int slideY = smartRandom(-12, 8);
        predictiveMouseMove(slideX, slideY, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(50, 80)));
    }
    g_feedbackSys->updateProgress(50);
    g_feedbackSys->logDirectionalHint("Slide-Cancel", "Jump cancellation -> direction change");
    smartKey(VK_SPACE, smartRandom(50, 80));
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, true);
    slideInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &slideInput, sizeof(INPUT));
    g_feedbackSys->updateProgress(75);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(80, 120)));
    bool newDirection = !initialLeft;
    smartKey(newDirection ? 'A' : 'D', smartRandom(60, 90));
    POINT postCancelMove = { newDirection ? smartRandom(-90, -70) : smartRandom(70, 90), smartRandom(-20, 15) };
    predictiveMouseMove(postCancelMove.x, postCancelMove.y, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(60, 100)));
    smartKey('C', smartRandom(40, 70));
    predictiveMouseMove(newDirection ? smartRandom(-60, -40) : smartRandom(40, 60), smartRandom(-10, 5), 2);
    g_feedbackSys->updateProgress(100);
    forward.ki.dwFlags = KEYEVENTF_KEYUP; sprintInput.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &forward, sizeof(INPUT)); SendInput(1, &sprintInput, sizeof(INPUT));
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}

void executeDiveDirectionalIntelligent() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    g_feedbackSys->startMovement("Buceo Direccional Inteligente", 1500);
    POINT currentPos; GetCursorPos(&currentPos);
    bool diveLeft = (currentPos.x > SCREEN_CENTER_X);
    bool diveBack = smartRandom(0, 2) == 0;
    std::string diveDirection = diveLeft ? "Left" : "Right";
    if (diveBack) diveDirection = "Back";
    g_feedbackSys->logDirectionalHint("Dive Direction", "Tactical " + diveDirection + " dive with recovery");
    if (g_momentumSys) { g_momentumSys->updateMovementState(false, false, true, false); g_momentumSys->addMomentum(0.5); }
    if (!diveBack) {
        smartKey(diveLeft ? 'A' : 'D', smartRandom(60, 90));
        POINT preMove = { diveLeft ? smartRandom(-60, -40) : smartRandom(40, 60), smartRandom(-15, 5) };
        predictiveMouseMove(preMove.x, preMove.y, 2);
    } else {
        smartKey('S', smartRandom(60, 90));
        predictiveMouseMove(smartRandom(-40, 40), smartRandom(10, 25), 2);
    }
    g_feedbackSys->updateProgress(25);
    g_feedbackSys->logDirectionalHint("Dive Execution", "Tactical dive -> prone");
    INPUT prone = {}; prone.type = INPUT_KEYBOARD; prone.ki.wVk = 'Z'; SendInput(1, &prone, sizeof(INPUT));
    for (int i = 0; i < 5; i++) {
        POINT diveMove;
        if (diveBack) { diveMove = { smartRandom(-80, 80), smartRandom(20, 40) }; }
        else { diveMove = { diveLeft ? smartRandom(-100, -80) : smartRandom(80, 100), smartRandom(-25, 15) }; }
        if (g_momentumSys) { diveMove = g_momentumSys->getOptimalDirection(diveMove); }
        predictiveMouseMove(diveMove.x, diveMove.y, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(70, 110)));
    }
    g_feedbackSys->updateProgress(50);
    g_feedbackSys->logDirectionalHint("Combat Position", "Prone firing stance");
    INPUT fire = {}; fire.type = INPUT_MOUSE; fire.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &fire, sizeof(INPUT));
    for (int i = 0; i < 4; i++) {
        smartMouseClick();
        predictiveMouseMove(smartRandom(-25, 25), smartRandom(-10, 10), 3);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(100, 150)));
    }
    fire.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &fire, sizeof(INPUT));
    g_feedbackSys->updateProgress(75);
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(200, 350)));
    g_feedbackSys->logDirectionalHint("Smart Recovery", "Tactical repositioning");
    prone.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &prone, sizeof(INPUT));
    smartKey(VK_SPACE, smartRandom(70, 100));
    POINT recoveryMove;
    if (diveBack) { smartKey('W', smartRandom(60, 90)); recoveryMove = {smartRandom(-50, 50), smartRandom(-30, -10)}; }
    else { smartKey(diveLeft ? 'D' : 'A', smartRandom(60, 90)); recoveryMove = { diveLeft ? smartRandom(60, 90) : smartRandom(-90, -60), smartRandom(-20, 10) }; }
    predictiveMouseMove(recoveryMove.x, recoveryMove.y, 2);
    g_feedbackSys->updateProgress(100);
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}

void executeOmnidirectionalSlide() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Omnidirectional Slide", 1800);
    g_feedbackSys->logDirectionalHint("Omni-Sprint", "Building 360 momentum");
    INPUT sprintInput = {}; sprintInput.type = INPUT_KEYBOARD; sprintInput.ki.wVk = VK_LSHIFT; SendInput(1, &sprintInput, sizeof(INPUT));
    if (g_momentumSys) { g_momentumSys->updateMovementState(true, false, false, false); g_momentumSys->addMomentum(0.6); }
    std::vector<std::pair<char, POINT>> sprintPattern = { {'W', {0, smartRandom(-60, -40)}}, {'D', {smartRandom(50, 70), smartRandom(-20, 0)}}, {'S', {smartRandom(-30, 30), smartRandom(40, 60)}}, {'A', {smartRandom(-70, -50), smartRandom(-20, 0)}} };
    for (size_t i = 0; i < sprintPattern.size(); i++) {
        smartKey(sprintPattern[i].first, smartRandom(50, 80));
        POINT move = sprintPattern[i].second;
        if (g_momentumSys) { move = g_momentumSys->getOptimalDirection(move); g_momentumSys->setLastDirection(move); }
        predictiveMouseMove(move.x, move.y, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(80, 120)));
        g_feedbackSys->updateProgress(25 * (i + 1));
    }
    g_feedbackSys->logDirectionalHint("Omni-Slide", "360 slide sequence");
    if (g_momentumSys) g_momentumSys->updateMovementState(false, true, false, false);
    std::vector<std::pair<char, POINT>> slidePattern = { {'C', {smartRandom(-80, -60), smartRandom(-15, 5)}}, {'C', {smartRandom(60, 80), smartRandom(-15, 5)}}, {'C', {smartRandom(-40, 40), smartRandom(20, 40)}}, {'C', {smartRandom(-30, 30), smartRandom(-25, -15)}} };
    for (size_t i = 0; i < slidePattern.size(); i++) {
        INPUT slideInput = {}; slideInput.type = INPUT_KEYBOARD; slideInput.ki.wVk = slidePattern[i].first; SendInput(1, &slideInput, sizeof(INPUT));
        for (int j = 0; j < 2; j++) {
            POINT slideMove = slidePattern[i].second;
            slideMove.x += smartRandom(-20, 20); slideMove.y += smartRandom(-10, 10);
            predictiveMouseMove(slideMove.x, slideMove.y, 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(60, 90)));
        }
        slideInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &slideInput, sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(100, 150)));
        g_feedbackSys->logDirectionalHint("Slide " + std::to_string(i+1), "Direction: " + std::string(1, slidePattern[i].first));
    }
    g_feedbackSys->logDirectionalHint("360 Finish", "Complete rotation");
    int totalRotation = 0;
    int targetRotation = smartRandom(350, 370);
    while (totalRotation < targetRotation) {
        int stepRotation = smartRandom(70, 100);
        predictiveMouseMove(stepRotation, smartRandom(-5, 5), 1);
        totalRotation += stepRotation;
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(25, 45)));
    }
    sprintInput.ki.dwFlags = KEYEVENTF_KEYUP; SendInput(1, &sprintInput, sizeof(INPUT));
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}

void executeContextualStrafeJump() {
    if (isExecutingMovement.exchange(true)) return;
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Contextual Strafe-Jump", 1500);

    bool strafe_left = true;
    for (int i = 0; i < 5; ++i) { // Perform 5 jumps
        // Strafe key
        smartKey(strafe_left ? 'A' : 'D', 50);

        // Jump
        smartKey(VK_SPACE, 60);
        g_feedbackSys->updateProgress((i + 1) * 20);

        // Small mouse movement in the direction of the strafe to enhance the effect
        int mouse_strafe = strafe_left ? smartRandom(-15, -10) : smartRandom(10, 15);
        predictiveMouseMove(mouse_strafe, smartRandom(-5, 5), 3);

        // Wait a bit before the next jump
        std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(200, 50)));

        // Alternate strafe direction
        strafe_left = !strafe_left;
    }

    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}

void executeMovementTest() {
    if (isExecutingMovement.exchange(true)) return;
    g_feedbackSys->startMovement("Movement System Test");
    logMessage("Test 1: Fluid Horizontal");
    predictiveMouseMove(100, 0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    logMessage("Test 2: Arc Diagonal");
    predictiveMouseMove(-80, -60, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    logMessage("Test 3: Accelerated Short Move");
    predictiveMouseMove(0, 30, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    g_feedbackSys->finishMovement(true);
    isExecutingMovement = false;
}