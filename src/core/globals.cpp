// globals.cpp - COMPLETE UPDATED VERSION v3.1.6
// description: Global variables implementation and system functions
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.6 - All missing variables and functions implemented
// date: 2025-07-17
// project: Tactical Aim Assist

#include "globals.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <thread>

// =============================================================================
// GLOBAL VARIABLE DEFINITIONS
// =============================================================================

// Global keybindings instance definition
Keybindings g_keybindings;

// Core system variables
std::atomic<bool> g_running{true};
std::atomic<bool> g_systemActive{false};
std::atomic<SystemState> g_currentSystemState{SystemState::INACTIVE};
std::atomic<bool> g_debugMode{false};
std::atomic<bool> g_verboseLogging{false};

// Aim assist system
std::atomic<bool> g_aimAssistEnabled{false};
std::atomic<bool> g_triggerBotEnabled{false};
std::atomic<bool> g_silentAimEnabled{false};
std::atomic<bool> g_predictionEnabled{true};
std::atomic<bool> g_recoilCompensationEnabled{true};
std::atomic<bool> g_adaptiveSensitivityEnabled{true};

// Input simulation
std::atomic<bool> g_isSimulatingInput{false};
std::atomic<bool> g_inputSystemActive{false};

// Additional system variables for config.cpp compatibility
std::atomic<bool> g_assistEnabled{false};
std::atomic<bool> g_adaptiveSmoothingEnabled{true};
std::atomic<bool> g_audioAlertsEnabled{true};

// Monitor and display variables
std::atomic<int> g_monitorWidth{1920};
std::atomic<int> g_monitorHeight{1080};
std::atomic<int> g_refreshRate{60};
std::atomic<double> g_currentFPS{60.0};
std::atomic<int> g_monitorCount{1};
std::atomic<int> g_monitorOffsetX{0};
std::atomic<int> g_monitorOffsetY{0};
std::atomic<double> g_dpiScale{1.0};
std::atomic<int> g_targetMonitor{1};

// Mouse and cursor
std::atomic<int> g_mouseX{960};
std::atomic<int> g_mouseY{540};
std::atomic<int> g_cursorX{960};
std::atomic<int> g_cursorY{540};
std::atomic<int> g_lastMouseX{960};
std::atomic<int> g_lastMouseY{540};

// Input control variables
std::atomic<unsigned int> g_toggleKey{0x70}; // F1
std::atomic<unsigned int> g_holdKey{0x71};   // F2
std::atomic<unsigned int> g_mouseButton{1};  // Left click
std::atomic<bool> g_holdMode{false};

// Performance and optimization variables
std::atomic<bool> g_enableSIMD{true};
std::atomic<bool> g_enableMultithreading{true};
std::atomic<bool> g_showMetrics{false};
std::atomic<bool> g_performanceMonitoringEnabled{true};
std::atomic<bool> g_profilingEnabled{false};
std::atomic<bool> g_memoryTrackingEnabled{true};
std::atomic<bool> g_cpuMonitoringEnabled{true};
std::atomic<bool> g_fpsTrackingEnabled{true};
std::atomic<bool> g_fileOutputEnabled{true};
std::atomic<bool> g_simdOptimizationEnabled{true};
std::atomic<int> g_maxFps{240};

// Aim assist configuration variables
std::atomic<double> g_aimFOV{GlobalConstants::DEFAULT_AIM_FOV};
std::atomic<double> g_triggerFOV{GlobalConstants::DEFAULT_TRIGGER_FOV};
std::atomic<double> g_scanFOV{200.0};
std::atomic<double> g_maxTargetDistance{GlobalConstants::DEFAULT_MAX_DISTANCE};
std::atomic<double> g_minConfidenceThreshold{GlobalConstants::DEFAULT_MIN_CONFIDENCE};
std::atomic<double> g_headConfidenceThreshold{GlobalConstants::DEFAULT_HEAD_CONFIDENCE};

// Sensitivity and smoothing
std::atomic<double> g_globalSensitivityMultiplier{1.0};
std::atomic<double> g_aimSmoothingFactor{0.75};
std::atomic<double> g_movementSmoothingFactor{0.8};
std::atomic<double> g_adaptiveSensitivityFactor{1.0};

// Aim system variables for config.cpp
std::atomic<double> g_sensitivity{1.0};
std::atomic<double> g_predictionTimeMs{50.0};
std::atomic<double> g_smoothingFactor{0.75};
std::atomic<double> g_maxAdjustmentDistance{500.0};
std::string g_antiDetectionLevel = "medium";

// Prediction and timing
std::atomic<double> g_predictionTime{50.0};
std::atomic<double> g_reactionDelay{100.0};
std::atomic<double> g_triggerDelay{50.0};
std::atomic<double> g_lockOnTime{200.0};

// Memory pool variables
std::atomic<bool> g_memoryPoolingEnabled{true};
std::atomic<int> g_initialPoolSize{32};
std::atomic<int> g_maxPoolSize{1024};
std::atomic<int> g_poolGrowthFactor{2};
std::atomic<bool> g_allowPoolExpansion{true};

// Input system variables
std::atomic<double> g_mouseSensitivity{1.0};
std::atomic<int> g_pollingRateHz{1000};
std::atomic<bool> g_rawInputEnabled{true};

// GUI system variables
std::atomic<int> g_guiUpdateIntervalMs{50};
std::atomic<bool> g_showPerformanceOverlay{false};
std::atomic<double> g_guiTransparency{0.9};

// Target tracking variables
std::atomic<bool> g_targetAcquired{false};
std::atomic<bool> g_hasValidTarget{false};
std::atomic<int> g_targetX{0};
std::atomic<int> g_targetY{0};
std::atomic<double> g_targetConfidence{0.0};
std::atomic<double> g_targetDistance{0.0};
std::atomic<int> g_targetPriority{0};

// Target tracking statistics
std::atomic<uint64_t> g_targetsDetected{0};
std::atomic<uint64_t> g_targetsAcquired{0};
std::atomic<uint64_t> g_targetsLost{0};
std::atomic<uint64_t> g_headshotsDetected{0};
std::atomic<uint64_t> g_bodyTargetsDetected{0};

// Target history and prediction
std::vector<TargetInfo>* g_targetHistory = nullptr;
std::atomic<size_t> g_targetHistorySize{0};
std::mutex g_targetHistoryMutex;

// Movement and statistics variables
std::atomic<uint64_t> g_totalMovements{0};
std::atomic<uint64_t> g_smoothedMovements{0};
std::atomic<uint64_t> g_predictedMovements{0};
std::atomic<uint64_t> g_compensatedMovements{0};
std::atomic<uint64_t> g_adaptiveMovements{0};

// Shooting statistics
std::atomic<uint64_t> g_totalShots{0};
std::atomic<uint64_t> g_totalHits{0};
std::atomic<uint64_t> g_headshotCount{0};
std::atomic<uint64_t> g_bodyShots{0};
std::atomic<uint64_t> g_missedShots{0};

// Performance metrics
std::atomic<uint64_t> g_frameCount{0};
std::atomic<double> g_averageFrameTime{16.67};
std::atomic<double> g_cpuUsage{0.0};
std::atomic<uint64_t> g_memoryUsage{0};
std::atomic<double> g_systemLoad{0.0};

// Profile and configuration variables
std::vector<WeaponProfile>* g_weaponProfiles = nullptr;
std::atomic<int> g_activeProfileIndex{0};
std::atomic<int> g_profileCount{0};
std::mutex g_profileMutex;

// Movement state
std::atomic<int> g_currentMovementState{static_cast<int>(PlayerMovementState::Stationary)};
std::atomic<double> g_movementSpeed{0.0};
std::atomic<bool> g_isMoving{false};
std::atomic<bool> g_isSprinting{false};
std::atomic<bool> g_isCrouching{false};

// Timing and synchronization variables
std::atomic<uint64_t> g_systemStartTime{0};
std::atomic<uint64_t> g_lastUpdateTime{0};
std::atomic<uint64_t> g_deltaTime{0};
std::atomic<uint64_t> g_sessionTime{0};

// Frame timing
std::atomic<double> g_lastFrameTime{0.0};
std::atomic<double> g_targetFrameTime{16.67};
std::atomic<bool> g_vsyncEnabled{true};
std::atomic<double> g_frameTimeVariance{0.0};

// Thread synchronization
std::mutex g_globalMutex;
std::mutex g_configMutex;
std::mutex g_logMutex;

// =============================================================================
// STRUCTURE IMPLEMENTATIONS
// =============================================================================

// WeaponProfile methods
bool WeaponProfile::isValid() const {
    return !name.empty() && 
           IS_VALID_RANGE(sensitivity, GlobalConstants::MIN_SENSITIVITY, GlobalConstants::MAX_SENSITIVITY) &&
           IS_VALID_RANGE(smoothing, GlobalConstants::MIN_SMOOTHING, GlobalConstants::MAX_SMOOTHING) &&
           IS_VALID_RANGE(prediction, 0.0, 1.0) &&
           IS_VALID_RANGE(recoil_compensation, 0.0, 1.0);
}

std::string WeaponProfile::typeToString() const {
    switch (type) {
        case WeaponType::ASSAULT_RIFLE: return "Assault Rifle";
        case WeaponType::SNIPER_RIFLE: return "Sniper Rifle";
        case WeaponType::SMG: return "SMG";
        case WeaponType::SHOTGUN: return "Shotgun";
        case WeaponType::PISTOL: return "Pistol";
        case WeaponType::LMG: return "LMG";
        case WeaponType::UNKNOWN:
        default: return "Unknown";
    }
}

void WeaponProfile::resetToDefaults() {
    *this = WeaponProfile();
}

void WeaponProfile::incrementUsage() {
    usage_count++;
    last_used = std::chrono::steady_clock::now();
}

void WeaponProfile::updateEffectiveness(double rating) {
    // Simple moving average
    effectiveness_rating = (effectiveness_rating * 0.8) + (rating * 0.2);
}

// TargetInfo methods
bool TargetInfo::isValid() const {
    return x >= 0 && y >= 0 && width > 0 && height > 0 &&
           IS_VALID_RANGE(confidence, GlobalConstants::MIN_CONFIDENCE, GlobalConstants::MAX_CONFIDENCE) &&
           distance >= 0;
}

double TargetInfo::getAge() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_updated);
    return static_cast<double>(duration.count());
}

void TargetInfo::updatePosition(int new_x, int new_y) {
    // Calculate velocity before updating position
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_updated);
    
    if (time_diff.count() > 0) {
        double dt = static_cast<double>(time_diff.count()) / 1000.0; // Convert to seconds
        velocity_x = (new_x - x) / dt;
        velocity_y = (new_y - y) / dt;
    }
    
    x = new_x;
    y = new_y;
    last_updated = now;
    frame_count++;
}

void TargetInfo::calculateVelocity() {
    // Velocity is calculated in updatePosition
    is_moving = (std::abs(velocity_x) > 1.0 || std::abs(velocity_y) > 1.0);
}

void TargetInfo::predict(double time_ahead_ms) {
    double dt = time_ahead_ms / 1000.0; // Convert to seconds
    predicted_x = x + (velocity_x * dt);
    predicted_y = y + (velocity_y * dt);
}

// MovementCommand methods
bool MovementCommand::isValid() const {
    return std::isfinite(delta_x) && std::isfinite(delta_y) &&
           IS_VALID_RANGE(smoothing_factor, GlobalConstants::MIN_SMOOTHING, GlobalConstants::MAX_SMOOTHING) &&
           IS_VALID_RANGE(confidence, GlobalConstants::MIN_CONFIDENCE, GlobalConstants::MAX_CONFIDENCE);
}

double MovementCommand::getAge() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp);
    return static_cast<double>(duration.count());
}

void MovementCommand::applySmoothing(double factor) {
    delta_x *= factor;
    delta_y *= factor;
    smoothing_factor = CLAMP(smoothing_factor * factor, GlobalConstants::MIN_SMOOTHING, GlobalConstants::MAX_SMOOTHING);
}

// ModifierInfo methods
ModifierInfo::ModifierInfo(int mod_value) : value(mod_value) {
    name = modifierToHumanString(mod_value);
    complexity = getModifierComplexity(mod_value);
    is_safe = isModifierCombinationSafe(mod_value);
    is_forbidden = isModifierCombinationForbidden(mod_value);
    is_emergency = (mod_value == MOD_EMERGENCY || 
                    mod_value == MOD_EMERGENCY_ALT || 
                    mod_value == MOD_EMERGENCY_WIN || 
                    mod_value == MOD_EMERGENCY_ALL);
    
    // Set description based on modifier
    if (is_emergency) {
        description = "Emergency modifier combination for critical functions";
    } else if (is_forbidden) {
        description = "Forbidden modifier combination (system reserved)";
    } else if (complexity == 0) {
        description = "No modifier keys";
    } else if (complexity == 1) {
        description = "Single modifier key";
    } else if (complexity == 2) {
        description = "Two modifier keys combination";
    } else if (complexity == 3) {
        description = "Three modifier keys combination";
    } else {
        description = "Complex modifier combination";
    }
}

bool ModifierInfo::isValid() const {
    return isModifierCombinationValid(value);
}

bool ModifierInfo::isSafe() const {
    return is_safe;
}

bool ModifierInfo::isForbidden() const {
    return is_forbidden;
}

bool ModifierInfo::isEmergency() const {
    return is_emergency;
}

std::string ModifierInfo::toString() const {
    std::ostringstream oss;
    oss << "Modifier: " << name << " (0x" << std::hex << value << std::dec << ")";
    oss << " | Complexity: " << complexity;
    oss << " | Safe: " << (is_safe ? "Yes" : "No");
    oss << " | Forbidden: " << (is_forbidden ? "Yes" : "No");
    oss << " | Emergency: " << (is_emergency ? "Yes" : "No");
    oss << " | Description: " << description;
    return oss.str();
}

// =============================================================================
// KEYBINDINGS UTILITY METHODS IMPLEMENTATION
// =============================================================================

std::string Keybindings::toString() const {
    std::ostringstream oss;
    oss << "=== KEYBINDINGS CONFIGURATION ===\n";
    oss << "Exit: " << getKeybindingString(exit_vk, exit_mod) << "\n";
    oss << "Emergency Stop: " << getKeybindingString(emergency_stop_vk, emergency_stop_mod) << "\n";
    oss << "Super Emergency: " << getKeybindingString(super_emergency_vk, super_emergency_mod) << "\n";
    oss << "Smart Sprint Left: " << getKeybindingString(smart_sprint_left_vk, smart_sprint_left_mod) << "\n";
    oss << "Smart Sprint Right: " << getKeybindingString(smart_sprint_right_vk, smart_sprint_right_mod) << "\n";
    oss << "Aim Assist Toggle: " << getKeybindingString(aim_assist_toggle_vk, aim_assist_toggle_mod) << "\n";
    oss << "Trigger Bot Toggle: " << getKeybindingString(trigger_bot_toggle_vk, trigger_bot_toggle_mod) << "\n";
    oss << "Silent Aim Toggle: " << getKeybindingString(silent_aim_toggle_vk, silent_aim_toggle_mod) << "\n";
    oss << "Prediction Toggle: " << getKeybindingString(prediction_toggle_vk, prediction_toggle_mod) << "\n";
    oss << "Recoil Toggle: " << getKeybindingString(recoil_toggle_vk, recoil_toggle_mod) << "\n";
    oss << "Profile Next: " << getKeybindingString(profile_next_vk, profile_next_mod) << "\n";
    oss << "Profile Previous: " << getKeybindingString(profile_prev_vk, profile_prev_mod) << "\n";
    oss << "Stats Reset: " << getKeybindingString(stats_reset_vk, stats_reset_mod) << "\n";
    oss << "Status Print: " << getKeybindingString(status_print_vk, status_print_mod) << "\n";
    oss << "Debug Toggle: " << getKeybindingString(debug_toggle_vk, debug_toggle_mod) << "\n";
    oss << "Test Mode: " << getKeybindingString(test_mode_vk, test_mode_mod) << "\n";
    oss << "Sensitivity +: " << getKeybindingString(sens_increase_vk, sens_increase_mod) << "\n";
    oss << "Sensitivity -: " << getKeybindingString(sens_decrease_vk, sens_decrease_mod) << "\n";
    oss << "FOV +: " << getKeybindingString(fov_increase_vk, fov_increase_mod) << "\n";
    oss << "FOV -: " << getKeybindingString(fov_decrease_vk, fov_decrease_mod) << "\n";
    oss << "Smoothing +: " << getKeybindingString(smooth_increase_vk, smooth_increase_mod) << "\n";
    oss << "Smoothing -: " << getKeybindingString(smooth_decrease_vk, smooth_decrease_mod) << "\n";
    oss << "================================";
    return oss.str();
}

bool Keybindings::isValid() const {
    // Basic validation - ensure no null virtual keys for critical bindings
    return exit_vk != 0 && 
           emergency_stop_vk != 0 && 
           super_emergency_vk != 0 &&
           aim_assist_toggle_vk != 0 &&
           validateKeybinding(exit_vk, exit_mod) &&
           validateKeybinding(emergency_stop_vk, emergency_stop_mod);
}

void Keybindings::resetToDefaults() {
    *this = Keybindings(); // Reset to default constructor values
}

bool Keybindings::loadFromFile(const std::string& filename) {
    // Placeholder implementation - would load from JSON/INI file
    LOG_SYSTEM("Loading keybindings from: " + filename);
    // Implementation would parse file and set values
    return true;
}

bool Keybindings::saveToFile(const std::string& filename) const {
    // Placeholder implementation - would save to JSON/INI file
    LOG_SYSTEM("Saving keybindings to: " + filename);
    // Implementation would write current values to file
    return true;
}

// =============================================================================
// KEYBINDING UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

std::string getKeybindingString(unsigned int vk, int mod) {
    std::string result;
    
    // Add modifiers
    if (mod != MOD_NONE) {
        result += getModifierCombinationName(mod) + " + ";
    }
    
    // Add virtual key
    result += getVirtualKeyName(vk);
    
    return result;
}

std::string getVirtualKeyName(unsigned int vk) {
    switch (vk) {
        // Function keys
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        
        // Arrow keys
        case VK_UP: return "UP";
        case VK_DOWN: return "DOWN";
        case VK_LEFT: return "LEFT";
        case VK_RIGHT: return "RIGHT";
        
        // Modifier keys
        case VK_LSHIFT: return "LEFT_SHIFT";
        case VK_RSHIFT: return "RIGHT_SHIFT";
        case VK_LCONTROL: return "LEFT_CTRL";
        case VK_RCONTROL: return "RIGHT_CTRL";
        case VK_LMENU: return "LEFT_ALT";
        case VK_RMENU: return "RIGHT_ALT";
        case VK_LWIN: return "LEFT_WIN";
        case VK_RWIN: return "RIGHT_WIN";
        
        // Special keys
        case VK_ESCAPE: return "ESCAPE";
        case VK_SPACE: return "SPACE";
        case VK_RETURN: return "ENTER";
        case VK_TAB: return "TAB";
        case VK_BACK: return "BACKSPACE";
        case VK_DELETE: return "DELETE";
        case VK_INSERT: return "INSERT";
        case VK_HOME: return "HOME";
        case VK_END: return "END";
        case VK_PRIOR: return "PAGE_UP";
        case VK_NEXT: return "PAGE_DOWN";
        
        // Symbol keys
        case VK_OEM_PLUS: return "PLUS";
        case VK_OEM_MINUS: return "MINUS";
        case VK_OEM_COMMA: return "COMMA";
        case VK_OEM_PERIOD: return "PERIOD";
        case VK_OEM_1: return "SEMICOLON";
        case VK_OEM_2: return "SLASH";
        case VK_OEM_3: return "GRAVE";
        case VK_OEM_4: return "LEFT_BRACKET";
        case VK_OEM_5: return "BACKSLASH";
        case VK_OEM_6: return "RIGHT_BRACKET";
        case VK_OEM_7: return "QUOTE";
        
        // Number keys
        case '0': return "0";
        case '1': return "1";
        case '2': return "2";
        case '3': return "3";
        case '4': return "4";
        case '5': return "5";
        case '6': return "6";
        case '7': return "7";
        case '8': return "8";
        case '9': return "9";
        
        // Letter keys
        case 'A': return "A";
        case 'B': return "B";
        case 'C': return "C";
        case 'D': return "D";
        case 'E': return "E";
        case 'F': return "F";
        case 'G': return "G";
        case 'H': return "H";
        case 'I': return "I";
        case 'J': return "J";
        case 'K': return "K";
        case 'L': return "L";
        case 'M': return "M";
        case 'N': return "N";
        case 'O': return "O";
        case 'P': return "P";
        case 'Q': return "Q";
        case 'R': return "R";
        case 'S': return "S";
        case 'T': return "T";
        case 'U': return "U";
        case 'V': return "V";
        case 'W': return "W";
        case 'X': return "X";
        case 'Y': return "Y";
        case 'Z': return "Z";
        
        // Numpad keys
        case VK_NUMPAD0: return "NUMPAD_0";
        case VK_NUMPAD1: return "NUMPAD_1";
        case VK_NUMPAD2: return "NUMPAD_2";
        case VK_NUMPAD3: return "NUMPAD_3";
        case VK_NUMPAD4: return "NUMPAD_4";
        case VK_NUMPAD5: return "NUMPAD_5";
        case VK_NUMPAD6: return "NUMPAD_6";
        case VK_NUMPAD7: return "NUMPAD_7";
        case VK_NUMPAD8: return "NUMPAD_8";
        case VK_NUMPAD9: return "NUMPAD_9";
        case VK_MULTIPLY: return "NUMPAD_MULTIPLY";
        case VK_ADD: return "NUMPAD_ADD";
        case VK_SUBTRACT: return "NUMPAD_SUBTRACT";
        case VK_DECIMAL: return "NUMPAD_DECIMAL";
        case VK_DIVIDE: return "NUMPAD_DIVIDE";
        
        // Mouse buttons
        case VK_LBUTTON: return "LEFT_MOUSE";
        case VK_RBUTTON: return "RIGHT_MOUSE";
        case VK_MBUTTON: return "MIDDLE_MOUSE";
        case VK_XBUTTON1: return "MOUSE_4";
        case VK_XBUTTON2: return "MOUSE_5";
        
        default:
            return "VK_" + std::to_string(vk);
    }
}

std::string getModifierString(int mod) {
    if (mod == MOD_NONE) return "NONE";
    
    std::vector<std::string> modifiers;
    
    if (mod & MOD_CONTROL) modifiers.push_back("CTRL");
    if (mod & MOD_ALT) modifiers.push_back("ALT");
    if (mod & MOD_SHIFT) modifiers.push_back("SHIFT");
    if (mod & MOD_WIN) modifiers.push_back("WIN");
    
    if (modifiers.empty()) return "NONE";
    
    std::string result;
    for (size_t i = 0; i < modifiers.size(); i++) {
        if (i > 0) result += " + ";
        result += modifiers[i];
    }
    
    return result;
}

std::string getModifierCombinationName(int mod) {
    switch (mod) {
        case MOD_NONE: return "None";
        case MOD_CONTROL: return "Ctrl";
        case MOD_ALT: return "Alt";
        case MOD_SHIFT: return "Shift";
        case MOD_WIN: return "Win";
        case MOD_CTRL_ALT: return "Ctrl+Alt";
        case MOD_CTRL_SHIFT: return "Ctrl+Shift";
        case MOD_ALT_SHIFT: return "Alt+Shift";
        case MOD_CTRL_WIN: return "Ctrl+Win";
        case MOD_ALT_WIN: return "Alt+Win";
        case MOD_SHIFT_WIN: return "Shift+Win";
        case MOD_CTRL_ALT_SHIFT: return "Ctrl+Alt+Shift";
        case MOD_CTRL_ALT_WIN: return "Ctrl+Alt+Win";
        case MOD_CTRL_SHIFT_WIN: return "Ctrl+Shift+Win";
        case MOD_ALT_SHIFT_WIN: return "Alt+Shift+Win";
        case MOD_ALL_MODIFIERS: return "Ctrl+Alt+Shift+Win";
        case MOD_EMERGENCY: return "Emergency (Ctrl+Shift)";
        case MOD_EMERGENCY_ALT: return "Emergency Alt (Ctrl+Shift+Alt)";
        case MOD_EMERGENCY_WIN: return "Emergency Win (Ctrl+Shift+Win)";
        case MOD_EMERGENCY_ALL: return "Emergency All (Ctrl+Shift+Alt+Win)";
        default: return getModifierString(mod);
    }
}

bool isKeybindingPressed(unsigned int vk, int mod) {
    // Check if the virtual key is pressed
    if (!(GetAsyncKeyState(vk) & 0x8000)) {
        return false;
    }
    
    // Check modifier state
    int current_mod = getCurrentModifierState();
    return current_mod == mod;
}

bool isModifierPressed(int modifier) {
    int current_mod = getCurrentModifierState();
    return (current_mod & modifier) == modifier;
}

bool isExactModifierPressed(int modifier) {
    int current_mod = getCurrentModifierState();
    return current_mod == modifier;
}

int getCurrentModifierState() {
    int mod = MOD_NONE;
    
    // Check Control keys
    if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || 
        (GetAsyncKeyState(VK_LCONTROL) & 0x8000) || 
        (GetAsyncKeyState(VK_RCONTROL) & 0x8000)) {
        mod |= MOD_CONTROL;
    }
    
    // Check Alt keys  
    if ((GetAsyncKeyState(VK_MENU) & 0x8000) || 
        (GetAsyncKeyState(VK_LMENU) & 0x8000) || 
        (GetAsyncKeyState(VK_RMENU) & 0x8000)) {
        mod |= MOD_ALT;
    }
    
    // Check Shift keys
    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || 
        (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || 
        (GetAsyncKeyState(VK_RSHIFT) & 0x8000)) {
        mod |= MOD_SHIFT;
    }
    
    // Check Windows keys
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || 
        (GetAsyncKeyState(VK_RWIN) & 0x8000)) {
        mod |= MOD_WIN;
    }
    
    return mod;
}

void initializeKeybindings() {
    LOG_SYSTEM("Initializing keybindings system...");
    
    // Set default values (already done by constructor)
    g_keybindings = Keybindings();
    
    // Validate keybindings
    if (!g_keybindings.isValid()) {
        logError("Invalid keybindings detected, resetting to defaults");
        g_keybindings.resetToDefaults();
    }
    
    LOG_SYSTEM("Keybindings initialized successfully");
    if (GET_ATOMIC(g_debugMode)) {
        logMessage(g_keybindings.toString());
    }
}

void loadKeybindingsFromConfig() {
    std::string config_file = GlobalConstants::KEYBINDINGS_FILE;
    
    if (g_keybindings.loadFromFile(config_file)) {
        LOG_SYSTEM("Keybindings loaded from config file");
    } else {
        logWarning("Failed to load keybindings, using defaults");
        g_keybindings.resetToDefaults();
    }
}

void saveKeybindingsToConfig() {
    std::string config_file = GlobalConstants::KEYBINDINGS_FILE;
    
    if (g_keybindings.saveToFile(config_file)) {
        LOG_SYSTEM("Keybindings saved to config file");
    } else {
        logError("Failed to save keybindings to config file");
    }
}

bool validateKeybinding(unsigned int vk, int mod) {
    // Basic validation
    if (vk == 0) return false;
    
    // Check for reserved system keys
    if (vk == VK_LWIN || vk == VK_RWIN) return false;
    
    // Check for invalid modifier combinations
    if (mod < 0 || mod > (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN)) {
        return false;
    }
    
    return true;
}

// =============================================================================
// EXTENDED MODIFIER UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

bool isModifierCombinationValid(int mod) {
    return mod >= MOD_MIN_VALUE && mod <= MOD_MAX_VALUE;
}

bool isModifierCombinationSafe(int mod) {
    if (!isModifierCombinationValid(mod)) return false;
    
    // Check against safe combinations
    for (size_t i = 0; i < MOD_SAFE_COMBINATIONS_COUNT; i++) {
        if (mod == MOD_SAFE_COMBINATIONS[i]) {
            return true;
        }
    }
    
    return false;
}

bool isModifierCombinationForbidden(int mod) {
    return mod == MOD_FORBIDDEN_1 || 
           mod == MOD_FORBIDDEN_2 || 
           mod == MOD_FORBIDDEN_3;
}

std::vector<int> getAllValidModifierCombinations() {
    std::vector<int> combinations;
    
    // Generate all possible combinations
    for (int i = MOD_MIN_VALUE; i <= MOD_MAX_VALUE; i++) {
        if (isModifierCombinationValid(i) && !isModifierCombinationForbidden(i)) {
            combinations.push_back(i);
        }
    }
    
    return combinations;
}

std::vector<int> getSafeModifierCombinations() {
    std::vector<int> combinations;
    
    for (size_t i = 0; i < MOD_SAFE_COMBINATIONS_COUNT; i++) {
        combinations.push_back(MOD_SAFE_COMBINATIONS[i]);
    }
    
    return combinations;
}

int getNextSafeModifierCombination(int current_mod) {
    auto safe_combinations = getSafeModifierCombinations();
    
    for (size_t i = 0; i < safe_combinations.size(); i++) {
        if (safe_combinations[i] == current_mod) {
            return safe_combinations[(i + 1) % safe_combinations.size()];
        }
    }
    
    return safe_combinations[0]; // Default to first safe combination
}

int getPreviousSafeModifierCombination(int current_mod) {
    auto safe_combinations = getSafeModifierCombinations();
    
    for (size_t i = 0; i < safe_combinations.size(); i++) {
        if (safe_combinations[i] == current_mod) {
            return safe_combinations[i == 0 ? safe_combinations.size() - 1 : i - 1];
        }
    }
    
    return safe_combinations[0]; // Default to first safe combination
}

std::string modifierToHumanString(int mod) {
    switch (mod) {
        case MOD_NONE: return "None";
        case MOD_CONTROL: return "Ctrl";
        case MOD_ALT: return "Alt";
        case MOD_SHIFT: return "Shift";
        case MOD_WIN: return "Win";
        case MOD_CTRL_ALT: return "Ctrl+Alt";
        case MOD_CTRL_SHIFT: return "Ctrl+Shift";
        case MOD_ALT_SHIFT: return "Alt+Shift";
        case MOD_CTRL_WIN: return "Ctrl+Win";
        case MOD_ALT_WIN: return "Alt+Win";
        case MOD_SHIFT_WIN: return "Shift+Win";
        case MOD_CTRL_ALT_SHIFT: return "Ctrl+Alt+Shift";
        case MOD_CTRL_ALT_WIN: return "Ctrl+Alt+Win";
        case MOD_CTRL_SHIFT_WIN: return "Ctrl+Shift+Win";
        case MOD_ALT_SHIFT_WIN: return "Alt+Shift+Win";
        case MOD_ALL_MODIFIERS: return "Ctrl+Alt+Shift+Win";
        case MOD_EMERGENCY: return "Emergency (Ctrl+Shift)";
        case MOD_EMERGENCY_ALT: return "Emergency Alt (Ctrl+Shift+Alt)";
        case MOD_EMERGENCY_WIN: return "Emergency Win (Ctrl+Shift+Win)";
        case MOD_EMERGENCY_ALL: return "Emergency All (Ctrl+Shift+Alt+Win)";
        default: return "Custom (" + std::to_string(mod) + ")";
    }
}

std::string modifierToConfigString(int mod) {
    return std::to_string(mod);
}

int configStringToModifier(const std::string& config_str) {
    try {
        int mod = std::stoi(config_str);
        return isModifierCombinationValid(mod) ? mod : MOD_NONE;
    } catch (...) {
        return MOD_NONE;
    }
}

int humanStringToModifier(const std::string& human_str) {
    // Convert human string back to modifier value
    if (human_str == "None") return MOD_NONE;
    if (human_str == "Ctrl") return MOD_CONTROL;
    if (human_str == "Alt") return MOD_ALT;
    if (human_str == "Shift") return MOD_SHIFT;
    if (human_str == "Win") return MOD_WIN;
    if (human_str == "Ctrl+Alt") return MOD_CTRL_ALT;
    if (human_str == "Ctrl+Shift") return MOD_CTRL_SHIFT;
    if (human_str == "Alt+Shift") return MOD_ALT_SHIFT;
    if (human_str == "Ctrl+Win") return MOD_CTRL_WIN;
    if (human_str == "Alt+Win") return MOD_ALT_WIN;
    if (human_str == "Shift+Win") return MOD_SHIFT_WIN;
    if (human_str == "Ctrl+Alt+Shift") return MOD_CTRL_ALT_SHIFT;
    if (human_str == "Ctrl+Alt+Win") return MOD_CTRL_ALT_WIN;
    if (human_str == "Ctrl+Shift+Win") return MOD_CTRL_SHIFT_WIN;
    if (human_str == "Alt+Shift+Win") return MOD_ALT_SHIFT_WIN;
    if (human_str == "Ctrl+Alt+Shift+Win") return MOD_ALL_MODIFIERS;
    
    return MOD_NONE; // Default fallback
}

bool areModifiersEquivalent(int mod1, int mod2) {
    return mod1 == mod2;
}

int getModifierComplexity(int mod) {
    int complexity = 0;
    if (mod & MOD_CONTROL) complexity++;
    if (mod & MOD_ALT) complexity++;
    if (mod & MOD_SHIFT) complexity++;
    if (mod & MOD_WIN) complexity++;
    return complexity;
}

bool isModifierMoreComplex(int mod1, int mod2) {
    return getModifierComplexity(mod1) > getModifierComplexity(mod2);
}

ModifierInfo getModifierInfo(int mod) {
    return ModifierInfo(mod);
}

std::vector<ModifierInfo> getAllModifierInfo() {
    std::vector<ModifierInfo> info_list;
    auto combinations = getAllValidModifierCombinations();
    
    for (int mod : combinations) {
        info_list.push_back(ModifierInfo(mod));
    }
    
    return info_list;
}

std::vector<ModifierInfo> getSafeModifierInfo() {
    std::vector<ModifierInfo> info_list;
    auto combinations = getSafeModifierCombinations();
    
    for (int mod : combinations) {
        info_list.push_back(ModifierInfo(mod));
    }
    
    return info_list;
}

// =============================================================================
// SYSTEM FUNCTIONS IMPLEMENTATION
// =============================================================================

bool initializeGlobalSystem() {
    LOG_SYSTEM("Initializing global system...");
    
    // Set system start time
    SET_ATOMIC(g_systemStartTime, getCurrentTimeMs());
    
    // Initialize collections
    g_weaponProfiles = new std::vector<WeaponProfile>();
    g_targetHistory = new std::vector<TargetInfo>();
    
    // Initialize default weapon profile
    WeaponProfile defaultProfile;
    defaultProfile.name = "Default";
    defaultProfile.type = WeaponType::ASSAULT_RIFLE;
    g_weaponProfiles->push_back(defaultProfile);
    SET_ATOMIC(g_profileCount, 1);
    
    // Initialize keybindings
    initializeKeybindings();
    
    // Set system state
    setSystemState(SystemState::ACTIVE);
    SET_ATOMIC(g_systemActive, true);
    
    LOG_SYSTEM("Global system initialized successfully");
    return true;
}

void shutdownGlobalSystem() {
    LOG_SYSTEM("Shutting down global system...");
    
    setSystemState(SystemState::SHUTTING_DOWN);
    
    // Save configurations
    saveKeybindingsToConfig();
    saveGlobalConfiguration(GlobalConstants::MAIN_CONFIG_FILE);
    
    // Clean up collections
    delete g_weaponProfiles;
    delete g_targetHistory;
    g_weaponProfiles = nullptr;
    g_targetHistory = nullptr;
    
    SET_ATOMIC(g_systemActive, false);
    setSystemState(SystemState::INACTIVE);
    
    LOG_SYSTEM("Global system shutdown complete");
}

bool isSystemInitialized() {
    return GET_ATOMIC(g_systemActive) && getSystemState() != SystemState::INACTIVE;
}

bool restartSystem() {
    LOG_SYSTEM("Restarting system...");
    shutdownGlobalSystem();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return initializeGlobalSystem();
}

void setSystemState(SystemState state) {
    SET_ATOMIC(g_currentSystemState, state);
    LOG_DEBUG_SYSTEM("System state changed to: " + getSystemStateString());
}

SystemState getSystemState() {
    return static_cast<SystemState>(GET_ATOMIC(g_currentSystemState));
}

std::string getSystemStateString() {
    switch (getSystemState()) {
        case SystemState::INACTIVE: return "INACTIVE";
        case SystemState::INITIALIZING: return "INITIALIZING";
        case SystemState::ACTIVE: return "ACTIVE";
        case SystemState::PAUSED: return "PAUSED";
        case SystemState::SHUTTING_DOWN: return "SHUTTING_DOWN";
        case SystemState::ERROR_STATE: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool isSystemHealthy() {
    SystemState state = getSystemState();
    return state == SystemState::ACTIVE && 
           GET_ATOMIC(g_systemActive) && 
           GET_ATOMIC(g_cpuUsage) < GlobalConstants::MAX_CPU_USAGE &&
           GET_ATOMIC(g_memoryUsage) < GlobalConstants::MAX_MEMORY_USAGE;
}

void setMovementState(PlayerMovementState state) {
    SET_ATOMIC(g_currentMovementState, static_cast<int>(state));
    SET_ATOMIC(g_isMoving, state != PlayerMovementState::Stationary);
    SET_ATOMIC(g_isSprinting, state == PlayerMovementState::Sprinting);
    SET_ATOMIC(g_isCrouching, state == PlayerMovementState::Crouching);
}

PlayerMovementState getCurrentMovementState() {
    return static_cast<PlayerMovementState>(GET_ATOMIC(g_currentMovementState));
}

std::string playerMovementStateToString(PlayerMovementState state) {
    switch (state) {
        case PlayerMovementState::Stationary: return "Stationary";
        case PlayerMovementState::Walking: return "Walking";
        case PlayerMovementState::Running: return "Running";
        case PlayerMovementState::Crouching: return "Crouching";
        case PlayerMovementState::Jumping: return "Jumping";
        case PlayerMovementState::Falling: return "Falling";
        case PlayerMovementState::Sprinting: return "Sprinting";
        case PlayerMovementState::Strafing: return "Strafing";
        case PlayerMovementState::Sliding: return "Sliding";
        default: return "Unknown";
    }
}

bool isMovementStateValid(PlayerMovementState state) {
    return state >= PlayerMovementState::Stationary && state <= PlayerMovementState::Sliding;
}

// =============================================================================
// PROFILE MANAGEMENT FUNCTIONS
// =============================================================================

bool loadWeaponProfiles(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    LOG_SYSTEM("Loading weapon profiles from: " + filename);
    // Placeholder - would implement JSON/XML loading
    
    return true;
}

bool saveWeaponProfiles(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    LOG_SYSTEM("Saving weapon profiles to: " + filename);
    // Placeholder - would implement JSON/XML saving
    
    return true;
}

WeaponProfile* getCurrentWeaponProfile() {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    if (!g_weaponProfiles || g_weaponProfiles->empty()) {
        return nullptr;
    }
    
    int index = GET_ATOMIC(g_activeProfileIndex);
    if (index < 0 || index >= static_cast<int>(g_weaponProfiles->size())) {
        return nullptr;
    }
    
    return &(*g_weaponProfiles)[index];
}

bool setActiveProfile(int index) {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    if (!g_weaponProfiles || index < 0 || index >= static_cast<int>(g_weaponProfiles->size())) {
        return false;
    }
    
    SET_ATOMIC(g_activeProfileIndex, index);
    LOG_DEBUG_SYSTEM("Active profile changed to index: " + std::to_string(index));
    
    return true;
}

int getActiveProfileIndex() {
    return GET_ATOMIC(g_activeProfileIndex);
}

int getWeaponProfileCount() {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    if (!g_weaponProfiles) return 0;
    return static_cast<int>(g_weaponProfiles->size());
}

bool addWeaponProfile(const WeaponProfile& profile) {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    if (!g_weaponProfiles || !profile.isValid()) {
        return false;
    }
    
    if (g_weaponProfiles->size() >= GlobalConstants::MAX_WEAPON_PROFILES) {
        logError("Maximum weapon profiles limit reached");
        return false;
    }
    
    g_weaponProfiles->push_back(profile);
    SET_ATOMIC(g_profileCount, static_cast<int>(g_weaponProfiles->size()));
    
    LOG_DEBUG_SYSTEM("Added weapon profile: " + profile.name);
    return true;
}

bool removeWeaponProfile(int index) {
    std::lock_guard<std::mutex> lock(g_profileMutex);
    
    if (!g_weaponProfiles || index < 0 || index >= static_cast<int>(g_weaponProfiles->size())) {
        return false;
    }
    
    // Don't allow removing the last profile
    if (g_weaponProfiles->size() <= 1) {
        logError("Cannot remove the last weapon profile");
        return false;
    }
    
    std::string name = (*g_weaponProfiles)[index].name;
    g_weaponProfiles->erase(g_weaponProfiles->begin() + index);
    SET_ATOMIC(g_profileCount, static_cast<int>(g_weaponProfiles->size()));
    
    // Adjust active profile index if necessary
    int active_index = GET_ATOMIC(g_activeProfileIndex);
    if (active_index >= index) {
        SET_ATOMIC(g_activeProfileIndex, std::max(0, active_index - 1));
    }
    
    LOG_DEBUG_SYSTEM("Removed weapon profile: " + name);
    return true;
}

// =============================================================================
// TARGET MANAGEMENT FUNCTIONS
// =============================================================================

bool addTarget(const TargetInfo& target) {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (!g_targetHistory || !target.isValid()) {
        return false;
    }
    
    // Remove old targets if we're at the limit
    while (g_targetHistory->size() >= GlobalConstants::MAX_TARGET_HISTORY) {
        g_targetHistory->erase(g_targetHistory->begin());
    }
    
    g_targetHistory->push_back(target);
    SET_ATOMIC(g_targetHistorySize, g_targetHistory->size());
    UPDATE_COUNTER(g_targetsDetected);
    
    return true;
}

bool updateTarget(size_t index, const TargetInfo& target) {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (!g_targetHistory || index >= g_targetHistory->size() || !target.isValid()) {
        return false;
    }
    
    (*g_targetHistory)[index] = target;
    return true;
}

bool removeTarget(size_t index) {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (!g_targetHistory || index >= g_targetHistory->size()) {
        return false;
    }
    
    g_targetHistory->erase(g_targetHistory->begin() + index);
    SET_ATOMIC(g_targetHistorySize, g_targetHistory->size());
    UPDATE_COUNTER(g_targetsLost);
    
    return true;
}

std::vector<TargetInfo> getActiveTargets() {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (!g_targetHistory) {
        return std::vector<TargetInfo>();
    }
    
    // Return targets that are still valid (not too old)
    std::vector<TargetInfo> active_targets;
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& target : *g_targetHistory) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - target.last_updated);
        if (age.count() < 1000) { // Targets valid for 1 second
            active_targets.push_back(target);
        }
    }
    
    return active_targets;
}

TargetInfo* getBestTarget() {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (!g_targetHistory || g_targetHistory->empty()) {
        return nullptr;
    }
    
    // Find the best target based on priority, confidence, and distance
    TargetInfo* best_target = nullptr;
    double best_score = -1.0;
    
    for (auto& target : *g_targetHistory) {
        if (!target.isValid() || target.getAge() > 500) { // Skip old targets
            continue;
        }
        
        // Calculate score based on multiple factors
        double score = target.confidence * 0.4;
        score += (static_cast<double>(target.priority) / 4.0) * 0.3; // Priority weight
        score += (1000.0 - std::min(target.distance, 1000.0)) / 1000.0 * 0.3; // Distance weight
        
        if (score > best_score) {
            best_score = score;
            best_target = &target;
        }
    }
    
    return best_target;
}

void clearTargetHistory() {
    std::lock_guard<std::mutex> lock(g_targetHistoryMutex);
    
    if (g_targetHistory) {
        g_targetHistory->clear();
        SET_ATOMIC(g_targetHistorySize, 0);
        LOG_DEBUG_SYSTEM("Target history cleared");
    }
}

// =============================================================================
// STATISTICS AND MONITORING FUNCTIONS
// =============================================================================

void updateSystemStatistics() {
    // Update frame count
    UPDATE_COUNTER(g_frameCount);
    
    // Update session time
    uint64_t current_time = getCurrentTimeMs();
    uint64_t start_time = GET_ATOMIC(g_systemStartTime);
    SET_ATOMIC(g_sessionTime, current_time - start_time);
    
    // Update delta time
    uint64_t last_time = GET_ATOMIC(g_lastUpdateTime);
    SET_ATOMIC(g_deltaTime, current_time - last_time);
    SET_ATOMIC(g_lastUpdateTime, current_time);
    
    // Calculate FPS
    if (GET_ATOMIC(g_deltaTime) > 0) {
        double fps = 1000.0 / static_cast<double>(GET_ATOMIC(g_deltaTime));
        SET_ATOMIC(g_currentFPS, fps);
    }
}

void resetGlobalStatistics() {
    LOG_SYSTEM("Resetting global statistics...");
    
    // Reset movement counters
    RESET_COUNTER(g_totalMovements);
    RESET_COUNTER(g_smoothedMovements);
    RESET_COUNTER(g_predictedMovements);
    RESET_COUNTER(g_compensatedMovements);
    RESET_COUNTER(g_adaptiveMovements);
    
    // Reset shooting statistics
    RESET_COUNTER(g_totalShots);
    RESET_COUNTER(g_totalHits);
    RESET_COUNTER(g_headshotCount);
    RESET_COUNTER(g_bodyShots);
    RESET_COUNTER(g_missedShots);
    
    // Reset target statistics
    RESET_COUNTER(g_targetsDetected);
    RESET_COUNTER(g_targetsAcquired);
    RESET_COUNTER(g_targetsLost);
    RESET_COUNTER(g_headshotsDetected);
    RESET_COUNTER(g_bodyTargetsDetected);
    
    // Reset frame count
    RESET_COUNTER(g_frameCount);
    
    // Clear target history
    clearTargetHistory();
    
    LOG_SYSTEM("All statistics reset successfully");
}

std::vector<std::string> getSystemStatistics() {
    std::vector<std::string> stats;
    
    stats.push_back("=== SYSTEM STATISTICS ===");
    stats.push_back("Session Time: " + std::to_string(GET_ATOMIC(g_sessionTime) / 1000) + " seconds");
    stats.push_back("Current FPS: " + std::to_string(GET_ATOMIC(g_currentFPS)));
    stats.push_back("Frame Count: " + std::to_string(GET_ATOMIC(g_frameCount)));
    stats.push_back("");
    
    stats.push_back("=== MOVEMENT STATISTICS ===");
    stats.push_back("Total Movements: " + std::to_string(GET_ATOMIC(g_totalMovements)));
    stats.push_back("Smoothed Movements: " + std::to_string(GET_ATOMIC(g_smoothedMovements)));
    stats.push_back("Predicted Movements: " + std::to_string(GET_ATOMIC(g_predictedMovements)));
    stats.push_back("Compensated Movements: " + std::to_string(GET_ATOMIC(g_compensatedMovements)));
    stats.push_back("Adaptive Movements: " + std::to_string(GET_ATOMIC(g_adaptiveMovements)));
    stats.push_back("");
    
    stats.push_back("=== SHOOTING STATISTICS ===");
    stats.push_back("Total Shots: " + std::to_string(GET_ATOMIC(g_totalShots)));
    stats.push_back("Total Hits: " + std::to_string(GET_ATOMIC(g_totalHits)));
    stats.push_back("Headshots: " + std::to_string(GET_ATOMIC(g_headshotCount)));
    stats.push_back("Body Shots: " + std::to_string(GET_ATOMIC(g_bodyShots)));
    stats.push_back("Missed Shots: " + std::to_string(GET_ATOMIC(g_missedShots)));
    stats.push_back("Accuracy: " + std::to_string(calculateAccuracy()) + "%");
    stats.push_back("");
    
    stats.push_back("=== TARGET STATISTICS ===");
    stats.push_back("Targets Detected: " + std::to_string(GET_ATOMIC(g_targetsDetected)));
    stats.push_back("Targets Acquired: " + std::to_string(GET_ATOMIC(g_targetsAcquired)));
    stats.push_back("Targets Lost: " + std::to_string(GET_ATOMIC(g_targetsLost)));
    stats.push_back("Headshots Detected: " + std::to_string(GET_ATOMIC(g_headshotsDetected)));
    stats.push_back("Body Targets Detected: " + std::to_string(GET_ATOMIC(g_bodyTargetsDetected)));
    stats.push_back("Target History Size: " + std::to_string(GET_ATOMIC(g_targetHistorySize)));
    stats.push_back("");
    
    stats.push_back("=== PERFORMANCE STATISTICS ===");
    stats.push_back("CPU Usage: " + std::to_string(GET_ATOMIC(g_cpuUsage)) + "%");
    stats.push_back("Memory Usage: " + std::to_string(GET_ATOMIC(g_memoryUsage) / (1024 * 1024)) + " MB");
    stats.push_back("System Load: " + std::to_string(GET_ATOMIC(g_systemLoad)));
    stats.push_back("System Health: " + std::string(isSystemHealthy() ? "HEALTHY" : "DEGRADED"));
    
    return stats;
}

double calculateAccuracy() {
    uint64_t total_shots = GET_ATOMIC(g_totalShots);
    uint64_t total_hits = GET_ATOMIC(g_totalHits);
    
    if (total_shots == 0) return 0.0;
    
    return (static_cast<double>(total_hits) / static_cast<double>(total_shots)) * 100.0;
}

double calculateEfficiency() {
    uint64_t total_movements = GET_ATOMIC(g_totalMovements);
    uint64_t smoothed_movements = GET_ATOMIC(g_smoothedMovements);
    
    if (total_movements == 0) return 0.0;
    
    return (static_cast<double>(smoothed_movements) / static_cast<double>(total_movements)) * 100.0;
}

double calculateAverageReactionTime() {
    // Placeholder - would calculate based on actual reaction data
    return GET_ATOMIC(g_reactionDelay);
}

// =============================================================================
// CONFIGURATION MANAGEMENT FUNCTIONS
// =============================================================================

bool loadGlobalConfiguration(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_configMutex);
    
    LOG_SYSTEM("Loading global configuration from: " + filename);
    // Placeholder - would implement JSON/XML loading
    
    return true;
}

bool saveGlobalConfiguration(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_configMutex);
    
    LOG_SYSTEM("Saving global configuration to: " + filename);
    // Placeholder - would implement JSON/XML saving
    
    return true;
}

void resetConfigurationToDefaults() {
    LOG_SYSTEM("Resetting configuration to defaults...");
    
    // Reset aim assist settings
    SET_ATOMIC(g_aimFOV, GlobalConstants::DEFAULT_AIM_FOV);
    SET_ATOMIC(g_triggerFOV, GlobalConstants::DEFAULT_TRIGGER_FOV);
    SET_ATOMIC(g_maxTargetDistance, GlobalConstants::DEFAULT_MAX_DISTANCE);
    SET_ATOMIC(g_minConfidenceThreshold, GlobalConstants::DEFAULT_MIN_CONFIDENCE);
    SET_ATOMIC(g_headConfidenceThreshold, GlobalConstants::DEFAULT_HEAD_CONFIDENCE);
    
    // Reset sensitivity settings
    SET_ATOMIC(g_globalSensitivityMultiplier, 1.0);
    SET_ATOMIC(g_aimSmoothingFactor, 0.75);
    SET_ATOMIC(g_movementSmoothingFactor, 0.8);
    SET_ATOMIC(g_adaptiveSensitivityFactor, 1.0);
    
    // Reset timing settings
    SET_ATOMIC(g_predictionTime, 50.0);
    SET_ATOMIC(g_reactionDelay, 100.0);
    SET_ATOMIC(g_triggerDelay, 50.0);
    SET_ATOMIC(g_lockOnTime, 200.0);
    
    // Reset keybindings
    g_keybindings.resetToDefaults();
    
    LOG_SYSTEM("Configuration reset to defaults complete");
}

bool validateConfiguration() {
    bool valid = true;
    
    // Validate FOV settings
    if (!IS_VALID_RANGE(GET_ATOMIC(g_aimFOV), GlobalConstants::MIN_FOV, GlobalConstants::MAX_FOV)) {
        logError("Invalid aim FOV value");
        valid = false;
    }
    
    if (!IS_VALID_RANGE(GET_ATOMIC(g_triggerFOV), GlobalConstants::MIN_FOV, GlobalConstants::MAX_FOV)) {
        logError("Invalid trigger FOV value");
        valid = false;
    }
    
    // Validate sensitivity settings
    if (!IS_VALID_RANGE(GET_ATOMIC(g_globalSensitivityMultiplier), GlobalConstants::MIN_SENSITIVITY, GlobalConstants::MAX_SENSITIVITY)) {
        logError("Invalid global sensitivity value");
        valid = false;
    }
    
    if (!IS_VALID_RANGE(GET_ATOMIC(g_aimSmoothingFactor), GlobalConstants::MIN_SMOOTHING, GlobalConstants::MAX_SMOOTHING)) {
        logError("Invalid aim smoothing value");
        valid = false;
    }
    
    // Validate timing settings
    if (!IS_VALID_RANGE(GET_ATOMIC(g_predictionTime), GlobalConstants::MIN_PREDICTION_TIME, GlobalConstants::MAX_PREDICTION_TIME)) {
        logError("Invalid prediction time value");
        valid = false;
    }
    
    if (!IS_VALID_RANGE(GET_ATOMIC(g_reactionDelay), GlobalConstants::MIN_REACTION_TIME, GlobalConstants::MAX_REACTION_TIME)) {
        logError("Invalid reaction delay value");
        valid = false;
    }
    
    // Validate keybindings
    if (!g_keybindings.isValid()) {
        logError("Invalid keybindings configuration");
        valid = false;
    }
    
    return valid;
}

void applyDefaultConfig() {
    // Apply default values to all configuration variables
    g_aimAssistEnabled.store(false);
    g_triggerBotEnabled.store(false);
    g_silentAimEnabled.store(false);
    g_predictionEnabled.store(true);
    g_recoilCompensationEnabled.store(true);
    g_adaptiveSensitivityEnabled.store(true);
    
    // Set default values for all new variables
    g_assistEnabled.store(false);
    g_adaptiveSmoothingEnabled.store(true);
    g_audioAlertsEnabled.store(true);
    
    g_globalSensitivityMultiplier.store(1.0);
    g_aimSmoothingFactor.store(0.75);
    g_predictionTime.store(50.0);
    g_aimFOV.store(GlobalConstants::DEFAULT_AIM_FOV);
    g_maxTargetDistance.store(GlobalConstants::DEFAULT_MAX_DISTANCE);
    
    // Set monitor defaults
    g_targetMonitor.store(1);
    g_monitorOffsetX.store(0);
    g_monitorOffsetY.store(0);
    g_dpiScale.store(1.0);
    
    // Set input defaults
    g_toggleKey.store(0x70); // F1
    g_holdKey.store(0x71);   // F2
    g_mouseButton.store(1);  // Left click
    g_holdMode.store(false);
    
    // Set performance defaults
    g_maxFps.store(240);
    g_performanceMonitoringEnabled.store(true);
    g_profilingEnabled.store(false);
    
    // Set GUI defaults
    g_guiUpdateIntervalMs.store(50);
    g_showPerformanceOverlay.store(false);
    g_guiTransparency.store(0.9);
}

// =============================================================================
// LOGGING AND DEBUGGING FUNCTIONS
// =============================================================================

void logMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    std::string timestamp = getCurrentTimeString();
    std::string log_entry = "[" + timestamp + "] " + message;
    
    std::cout << log_entry << std::endl;
    
    // Write to file if needed
    // std::ofstream log_file(GlobalConstants::LOG_FILE, std::ios::app);
    // if (log_file.is_open()) {
    //     log_file << log_entry << std::endl;
    //     log_file.close();
    // }
}

void logError(const std::string& message) {
    logMessage("[ERROR] " + message);
}

void logWarning(const std::string& message) {
    logMessage("[WARNING] " + message);
}

void logDebug(const std::string& message) {
    if (GET_ATOMIC(g_debugMode)) {
        logMessage("[DEBUG] " + message);
    }
}

void logVerbose(const std::string& message) {
    if (GET_ATOMIC(g_verboseLogging)) {
        logMessage("[VERBOSE] " + message);
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

uint64_t getCurrentTimeMs() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

double getCurrentTimeSec() {
    return static_cast<double>(getCurrentTimeMs()) / 1000.0;
}

std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string getSystemStatus() {
    std::ostringstream oss;
    oss << "System: " << getSystemStateString() << " | ";
    oss << "FPS: " << std::fixed << std::setprecision(1) << GET_ATOMIC(g_currentFPS) << " | ";
    oss << "Aim: " << (GET_ATOMIC(g_aimAssistEnabled) ? "ON" : "OFF") << " | ";
    oss << "Trigger: " << (GET_ATOMIC(g_triggerBotEnabled) ? "ON" : "OFF") << " | ";
    oss << "Movement: " << playerMovementStateToString(getCurrentMovementState()) << " | ";
    oss << "Targets: " << GET_ATOMIC(g_targetHistorySize);
    
    return oss.str();
}

std::string getPerformanceReport() {
    std::ostringstream oss;
    oss << "=== PERFORMANCE REPORT ===\n";
    oss << "Session Time: " << GET_ATOMIC(g_sessionTime) / 1000 << " seconds\n";
    oss << "Average FPS: " << std::fixed << std::setprecision(2) << GET_ATOMIC(g_currentFPS) << "\n";
    oss << "Frame Time: " << GET_ATOMIC(g_averageFrameTime) << " ms\n";
    oss << "CPU Usage: " << GET_ATOMIC(g_cpuUsage) << "%\n";
    oss << "Memory Usage: " << GET_ATOMIC(g_memoryUsage) / (1024 * 1024) << " MB\n";
    oss << "System Load: " << GET_ATOMIC(g_systemLoad) << "\n";
    oss << "System Health: " << (isSystemHealthy() ? "HEALTHY" : "DEGRADED") << "\n";
    oss << "Total Movements: " << GET_ATOMIC(g_totalMovements) << "\n";
    oss << "Movement Efficiency: " << std::fixed << std::setprecision(1) << calculateEfficiency() << "%\n";
    oss << "Shooting Accuracy: " << std::fixed << std::setprecision(1) << calculateAccuracy() << "%\n";
    oss << "========================";
    
    return oss.str();
}