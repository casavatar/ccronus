// movements.cpp - FIXED all errors v3.0.2
// description: Movement system - Fixed missing variables and declarations
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.2 - Fixed all compilation issues
// date: 2025-07-16
// project: Tactical Aim Assist

#include <iostream>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <random>
#include <mutex>
#include <vector>
#include <cmath>

#include "globals.h"
#include "core/config.h"

// =============================================================================
// MISSING VARIABLES AND STUBS
// =============================================================================
std::mutex g_random_mutex;
thread_local std::mt19937 gen(std::random_device{}());

// Anti-detection system stub
struct AntiDetectionSystem {
    enum Context { IDLE, MOVEMENT, COMBAT };
    
    void updateContext(Context ctx) {
        logDebug("Anti-detection context: " + std::to_string(static_cast<int>(ctx)));
    }
    
    void registerAction(UINT key) {
        logDebug("Registered action: " + std::to_string(key));
    }
    
    double getHumanAccuracy() const { return 0.85; }
    int getHumanDelay() const { return 25 + (rand() % 15); }
    bool shouldAddJitter() const { return true; }
    std::pair<double, double> getJitter() const {
        std::uniform_real_distribution<double> dist(-0.5, 0.5);
        std::lock_guard<std::mutex> lock(g_random_mutex);
        return {dist(gen), dist(gen)};
    }
};

// Global systems (stubs)
std::unique_ptr<AntiDetectionSystem> g_antiDetection = std::make_unique<AntiDetectionSystem>();
std::atomic<int> g_activeProfileIndex{0};
bool g_predictiveAim = true;

// Feedback system stub
struct FeedbackSystem {
    void startMovement(const std::string& name, int duration = 500) {
        logDebug("Movement started: " + name + " (" + std::to_string(duration) + "ms)");
    }
};

std::unique_ptr<FeedbackSystem> g_feedbackSys = std::make_unique<FeedbackSystem>();

// Momentum system stub
struct MomentumSystem {
    void updateMovementState(bool forward, bool backward, bool left, bool right) {
        logDebug("Momentum state: F=" + std::to_string(forward) + " B=" + std::to_string(backward) + 
                " L=" + std::to_string(left) + " R=" + std::to_string(right));
    }
    
    void addMomentum(double amount) {
        logDebug("Added momentum: " + std::to_string(amount));
    }
    
    bool canChainMovement() const { return true; }
    
    int getOptimalDirection(int base_direction) const {
        return base_direction; // Simple passthrough
    }
    
    void setLastDirection(int direction) {
        logDebug("Set last direction: " + std::to_string(direction));
    }
};

std::unique_ptr<MomentumSystem> g_momentumSys = std::make_unique<MomentumSystem>();

// Smoothing system stub
struct SmoothingSystem {
    std::pair<int, int> smooth(int x, int y) {
        return {x, y}; // No smoothing for now
    }
};

std::unique_ptr<SmoothingSystem> g_smoothingSystem = std::make_unique<SmoothingSystem>();

// =============================================================================
// RANDOM AND TIMING FUNCTIONS
// =============================================================================
int smartRandom(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    std::lock_guard<std::mutex> lock(g_random_mutex);
    return dist(gen);
}

void smartKey(WORD key, int delay) {
    if (g_antiDetection) {
        g_antiDetection->registerAction(key);
    }
    
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.dwFlags = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    
    // Press key
    SendInput(1, &input, sizeof(INPUT));
    
    // Add human-like delay variation
    int actualDelay = delay + smartRandom(-5, 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(actualDelay));
    
    // Release key
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void smartMouseClick() {
    if (g_antiDetection) g_antiDetection->registerAction(VK_LBUTTON);
    
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(smartRandom(15, 35)));
    
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

// =============================================================================
// MOUSE MOVEMENT FUNCTIONS
// =============================================================================
void fluidMouseMove(int deltaX, int deltaY, int steps) {
    if (deltaX == 0 && deltaY == 0) return;
    
    steps = std::max(1, std::min(steps, 20));
    
    float stepX = static_cast<float>(deltaX) / steps;
    float stepY = static_cast<float>(deltaY) / steps;
    
    for (int i = 0; i < steps; ++i) {
        int moveX = static_cast<int>(stepX);
        int moveY = static_cast<int>(stepY);
        
        // Add micro-jitter for humanization
        if (g_antiDetection && g_antiDetection->shouldAddJitter()) {
            auto jitter = g_antiDetection->getJitter();
            moveX += static_cast<int>(jitter.first);
            moveY += static_cast<int>(jitter.second);
        }
        
        if (moveX != 0 || moveY != 0) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = moveX;
            input.mi.dy = moveY;
            
            SendInput(1, &input, sizeof(INPUT));
        }
        
        if (i < steps - 1) {
            std::this_thread::sleep_for(std::chrono::microseconds(500 + smartRandom(0, 300)));
        }
    }
}

void predictiveMouseMove(int targetX, int targetY, int confidence) {
    POINT cursor;
    if (!GetCursorPos(&cursor)) return;
    
    int deltaX = targetX - cursor.x;
    int deltaY = targetY - cursor.y;
    
    if (std::abs(deltaX) < 2 && std::abs(deltaY) < 2) return;
    
    // Apply smoothing if available
    if (g_smoothingSystem) {
        auto smoothed = g_smoothingSystem->smooth(deltaX, deltaY);
        deltaX = smoothed.first;
        deltaY = smoothed.second;
    }
    
    // Apply human accuracy factor
    double humanAccuracy = g_antiDetection ? g_antiDetection->getHumanAccuracy() : 1.0;
    
    if (humanAccuracy < 1.0) {
        std::uniform_real_distribution<double> errorDist(-2.0, 2.0);
        std::lock_guard<std::mutex> lock(g_random_mutex);
        deltaX += static_cast<int>(errorDist(gen));
        deltaY += static_cast<int>(errorDist(gen));
    }
    
    // Calculate movement steps based on distance and confidence
    int distance = static_cast<int>(std::sqrt(deltaX * deltaX + deltaY * deltaY));
    int steps = std::max(1, std::min(distance / 8, confidence / 10));
    
    fluidMouseMove(deltaX, deltaY, steps);
}

// =============================================================================
// RECOIL AND WEAPON FUNCTIONS
// =============================================================================
std::pair<int, int> getCurrentRecoil() {
    static int recoilIndex = 0;
    
    // FIXED: Check if weapon profiles exist and are accessible
    if (!g_weaponProfiles || g_weaponProfiles->empty()) {
        return {0, 0};
    }
    
    int profileIndex = g_activeProfileIndex.load();
    if (profileIndex < 0 || profileIndex >= static_cast<int>(g_weaponProfiles->size())) {
        return {0, 0};
    }
    
    // FIXED: Use the profile variable to avoid warning
    const auto& profile = (*g_weaponProfiles)[profileIndex];
    
    // Basic vertical recoil with slight horizontal variance
    // Use profile sensitivity to adjust recoil magnitude
    double recoilMultiplier = profile.sensitivity * 0.5; // Use profile data
    
    int verticalRecoil = static_cast<int>(-(recoilIndex * 2 + smartRandom(0, 3)) * recoilMultiplier);
    int horizontalRecoil = static_cast<int>((recoilIndex % 2 == 0 ? 1 : -1) * smartRandom(0, 2) * recoilMultiplier);
    
    recoilIndex = (recoilIndex + 1) % 30; // Reset after 30 shots
    
    return {horizontalRecoil, verticalRecoil};
}

void controlledAutomaticFire() {
    static int recoilIndex = 0;
    static auto lastShotTime = std::chrono::steady_clock::now();
    
    if (!g_weaponProfiles || g_weaponProfiles->empty()) return;
    
    int profileIndex = g_activeProfileIndex.load();
    if (profileIndex < 0 || profileIndex >= static_cast<int>(g_weaponProfiles->size())) return;
    
    const auto& currentProfile = (*g_weaponProfiles)[profileIndex];
    
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastShot = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShotTime).count();
    
    if (timeSinceLastShot < currentProfile.fire_delay_base) return;
    
    // Fire shot
    smartMouseClick();
    lastShotTime = now;
    
    // Apply recoil compensation
    if (currentProfile.recoil_compensation) {
        auto recoil = getCurrentRecoil();
        
        // Apply predictive aim adjustment periodically
        if (g_predictiveAim && recoilIndex % 3 == 0) {
            recoil.first += smartRandom(-1, 1);
            recoil.second += smartRandom(-2, 0);
        }
        
        fluidMouseMove(recoil.first, recoil.second, 2);
    }
    
    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    }
    
    recoilIndex++;
}

void enhancedIntelligentRapidFire() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    
    if (!g_weaponProfiles || g_weaponProfiles->empty()) return;
    
    int profileIndex = g_activeProfileIndex.load();
    if (profileIndex < 0 || profileIndex >= static_cast<int>(g_weaponProfiles->size())) return;
    
    const auto& currentProfile = (*g_weaponProfiles)[profileIndex];
    
    int burstCount = 3 + smartRandom(0, 2);
    
    for (int i = 0; i < burstCount; ++i) {
        smartMouseClick();
        
        if (currentProfile.recoil_compensation) {
            auto recoil = getCurrentRecoil();
            fluidMouseMove(recoil.first, recoil.second, 1);
        }
        
        if (i < burstCount - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                currentProfile.fire_delay_base + smartRandom(-10, 10)));
        }
    }
    
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::IDLE);
}

void tacticalFire() {
    static auto lastTacticalShot = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTacticalShot).count();
    
    if (elapsed < 200) return; // Minimum tactical fire interval
    
    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    }
    
    if (!g_weaponProfiles || g_weaponProfiles->empty()) return;
    
    int profileIndex = g_activeProfileIndex.load();
    if (profileIndex < 0 || profileIndex >= static_cast<int>(g_weaponProfiles->size())) return;
    
    const auto& currentProfile = (*g_weaponProfiles)[profileIndex];
    
    // Single precise shot
    smartMouseClick();
    
    // Apply minimal recoil compensation for precision
    if (currentProfile.recoil_compensation) {
        auto recoil = getCurrentRecoil();
        recoil.first /= 2; // Reduce recoil for tactical fire
        recoil.second /= 2;
        fluidMouseMove(recoil.first, recoil.second, 3);
    }
    
    lastTacticalShot = now;
    
    if (g_antiDetection) {
        g_antiDetection->updateContext(AntiDetectionSystem::IDLE);
    }
}

// =============================================================================
// MOVEMENT FUNCTIONS
// =============================================================================
void executeSmartDiagonalSprint(bool leftDirection) {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement(leftDirection ? "Smart Sprint Left" : "Smart Sprint Right");
    
    // Smart diagonal sprint sequence
    WORD primaryKey = leftDirection ? 'A' : 'D';
    WORD secondaryKey = 'W';
    
    // Press both keys simultaneously
    INPUT inputs[3];
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = primaryKey;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = secondaryKey;
    inputs[1].ki.dwFlags = 0;
    
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_SHIFT;
    inputs[2].ki.dwFlags = 0;
    
    SendInput(3, inputs, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(150 + smartRandom(-20, 20)));
    
    // Release keys
    for (int i = 0; i < 3; ++i) {
        inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput(3, inputs, sizeof(INPUT));
    
    if (g_momentumSys) g_momentumSys->addMomentum(0.1);
}

void executePredictiveSlide() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Tactical Slide");
    
    // Predictive slide with momentum preservation
    INPUT inputs[2];
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'S';
    inputs[1].ki.dwFlags = 0;
    
    SendInput(2, inputs, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(200 + smartRandom(-30, 30)));
    
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
    
    if (g_momentumSys) g_momentumSys->addMomentum(0.2);
}

void executeAntiDetectionDiveBack() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Tactical Dive Back");
    
    // Enhanced dive back with variable timing
    for (int phase = 0; phase < 3; ++phase) {
        WORD key = (phase == 0) ? 'S' : VK_CONTROL;
        smartKey(key, 40 + smartRandom(-10, 10));
        
        int humanDelay = g_antiDetection ? g_antiDetection->getHumanDelay() : 30;
        std::this_thread::sleep_for(std::chrono::milliseconds(humanDelay));
    }
    
    if (g_momentumSys) g_momentumSys->addMomentum(0.15);
}

void executeSmartCornerBounce(bool leftCorner) {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement(leftCorner ? "Corner Bounce Left" : "Corner Bounce Right");
    
    WORD directionKey = leftCorner ? 'A' : 'D';
    
    // Complex corner bounce sequence
    smartKey(directionKey, 30);
    smartKey('W', 20);
    smartKey(VK_SPACE, 50);
    
    if (g_momentumSys) g_momentumSys->addMomentum(0.25);
}

void executePredictiveCutback() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Predictive Cutback");
    
    // Quick direction change maneuver
    smartKey('S', 25);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    smartKey('W', 80);
    
    if (g_momentumSys) g_momentumSys->addMomentum(0.3);
}

void executeDropShotSupineSlide() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    g_feedbackSys->startMovement("Drop-Shot Supine Slide", 1200);
    if (g_momentumSys) { g_momentumSys->updateMovementState(true, false, false, false); g_momentumSys->addMomentum(0.4); }
    
    // Advanced drop-shot with slide combination
    INPUT inputs[4];
    
    // Phase 1: Initiate prone
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    SendInput(1, inputs, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Phase 2: Add directional slide
    for (int slidePhase = 0; slidePhase < 3; ++slidePhase) {
        int slideDirection = smartRandom(0, 3);
        WORD slideKey = (slideDirection == 0) ? 'W' : (slideDirection == 1) ? 'A' : (slideDirection == 2) ? 'S' : 'D';
        
        if (g_momentumSys) { slideDirection = g_momentumSys->getOptimalDirection(slideDirection); g_momentumSys->setLastDirection(slideDirection); }
        
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = slideKey;
        inputs[1].ki.dwFlags = 0;
        SendInput(1, &inputs[1], sizeof(INPUT));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(80 + smartRandom(-15, 15)));
        
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &inputs[1], sizeof(INPUT));
        
        if (slidePhase < 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    
    // Phase 3: Complete maneuver
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, inputs, sizeof(INPUT));
    
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
}

void executeSlideCancelDirectional() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Slide-Cancel Direccional", 1000);
    bool hasMomentum = g_momentumSys && g_momentumSys->canChainMovement();
    
    // Advanced slide-cancel with directional intelligence
    INPUT inputs[3];
    
    // Phase 1: Initiate slide
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'W';
    inputs[1].ki.dwFlags = 0;
    
    SendInput(2, inputs, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(hasMomentum ? 120 : 150));
    
    // Phase 2: Quick cancel and redirect
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    
    // Phase 3: Directional burst
    WORD redirectKey = hasMomentum ? 'D' : 'A';
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = redirectKey;
    inputs[2].ki.dwFlags = 0;
    SendInput(1, &inputs[2], sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputs[2], sizeof(INPUT));
}

void executeDiveDirectionalIntelligent() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::COMBAT);
    g_feedbackSys->startMovement("Buceo Direccional Inteligente", 1500);
    
    // Get current game state for intelligent direction selection
    POINT cursor;
    GetCursorPos(&cursor);
    
    if (g_momentumSys) { g_momentumSys->updateMovementState(false, false, true, false); g_momentumSys->addMomentum(0.5); }
    
    // Phase 1: Initiate dive
    INPUT inputs[4];
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    SendInput(1, inputs, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    
    // Phase 2: Intelligent directional movement
    for (int divePhase = 0; divePhase < 4; ++divePhase) {
        int diveMove = (cursor.x < 960) ? 'D' : 'A'; // Screen center-based decision
        
        if (g_momentumSys) { diveMove = g_momentumSys->getOptimalDirection(diveMove); }
        
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = diveMove;
        inputs[1].ki.dwFlags = 0;
        SendInput(1, &inputs[1], sizeof(INPUT));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(90 + smartRandom(-20, 20)));
        
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &inputs[1], sizeof(INPUT));
        
        if (divePhase < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }
    }
    
    // Phase 3: Finalize dive
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, inputs, sizeof(INPUT));
    
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
}

void executeOmnidirectionalSlide() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Omnidirectional Slide", 1800);
    
    // Ultra-advanced omnidirectional slide system
    if (g_momentumSys) { g_momentumSys->updateMovementState(true, false, false, false); g_momentumSys->addMomentum(0.6); }
    
    for (int omniPhase = 0; omniPhase < 8; ++omniPhase) {
        int move = omniPhase % 4; // Cycle through directions
        if (g_momentumSys) { move = g_momentumSys->getOptimalDirection(move); g_momentumSys->setLastDirection(move); }
        
        WORD omniKey = (move == 0) ? 'W' : (move == 1) ? 'D' : (move == 2) ? 'S' : 'A';
        
        if (g_momentumSys) g_momentumSys->updateMovementState(false, true, false, false);
        
        INPUT inputs[2];
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        inputs[0].ki.dwFlags = 0;
        
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = omniKey;
        inputs[1].ki.dwFlags = 0;
        
        SendInput(2, inputs, sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(180 + smartRandom(-30, 30)));
        
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, inputs, sizeof(INPUT));
        
        if (omniPhase < 7) {
            std::this_thread::sleep_for(std::chrono::milliseconds(45));
        }
    }
    
    if (g_momentumSys) g_momentumSys->updateMovementState(false, false, false, false);
}

void executeContextualStrafeJump() {
    if (g_antiDetection) g_antiDetection->updateContext(AntiDetectionSystem::MOVEMENT);
    g_feedbackSys->startMovement("Contextual Strafe-Jump", 1500);
    
    // Context-aware strafe jumping based on current state
    bool leftStrafe = smartRandom(0, 1) == 0;
    WORD strafeKey = leftStrafe ? 'A' : 'D';
    
    // Pre-strafe
    smartKey(strafeKey, 50);
    
    // Jump with strafe maintenance
    INPUT inputs[2];
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SPACE;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = strafeKey;
    inputs[1].ki.dwFlags = 0;
    
    SendInput(2, inputs, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void executeMovementTest() {
    logMessage("🧪 Executing Movement System Test...");
    g_feedbackSys->startMovement("Movement System Test");
    
    // Test sequence of all movement types
    executeSmartDiagonalSprint(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    executePredictiveSlide();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    executeSmartCornerBounce(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    logMessage("✅ Movement System Test completed");
}

// =============================================================================
// INITIALIZATION AND UTILITY FUNCTIONS
// =============================================================================
bool initializeMovementSystem() {
    logMessage("🏃 Initializing Movement System...");
    
    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Initialize systems
    if (!g_antiDetection) {
        g_antiDetection = std::make_unique<AntiDetectionSystem>();
    }
    
    if (!g_feedbackSys) {
        g_feedbackSys = std::make_unique<FeedbackSystem>();
    }
    
    if (!g_momentumSys) {
        g_momentumSys = std::make_unique<MomentumSystem>();
    }
    
    if (!g_smoothingSystem) {
        g_smoothingSystem = std::make_unique<SmoothingSystem>();
    }
    
    logMessage("✅ Movement System initialized successfully");
    return true;
}

void shutdownMovementSystem() {
    logMessage("🛑 Shutting down Movement System...");
    
    g_antiDetection.reset();
    g_feedbackSys.reset();
    g_momentumSys.reset();
    g_smoothingSystem.reset();
    
    logMessage("✅ Movement System shutdown complete");
}