// globals.cpp - FINAL CLEANED VERSION v5.0.0
// description: Implementation of global stateless utility functions.
//              All mutable global state variables and their related functions
//              have been removed.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 5.0.0 - Major refactor complete. Removed all global state variables.
// date: 2025-07-20
// project: Tactical Aim Assist

#include "globals.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <algorithm>

// =============================================================================
// GLOBAL VARIABLE DEFINITIONS
// =============================================================================

std::atomic<bool> g_application_running{true};
std::atomic<bool> g_debugMode{false};

// A single mutex for thread-safe logging to cout/file.
static std::mutex g_logMutex;

// =============================================================================
// STATELESS UTILITY FUNCTION IMPLEMENTATIONS
// =============================================================================

// --- Logging ---

void logMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimeString();
    std::cout << "[" << timestamp << "] [INFO] " << message << std::endl;

     std::ofstream log_file(GlobalConstants::LOG_FILE, std::ios::app);
     if (log_file.is_open()) {
         log_file << "[" << timestamp << "] [INFO] " << message << std::endl;
    }
}

void logError(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimeString();
    std::cerr << "[" << timestamp << "] [ERROR] " << message << std::endl;

    std::ofstream log_file(GlobalConstants::LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << timestamp << "] [ERROR] " << message << std::endl;
    }   
}

void logWarning(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimeString();
    std::cout << "[" << timestamp << "] [WARN] " << message << std::endl;

    std::ofstream log_file(GlobalConstants::LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << timestamp << "] [WARN] " << message << std::endl;
    }
}

void logDebug(const std::string& message) {
    // In a real application, this would check a debug flag from the StateManager.
    // For now, we assume it's always on for development.
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimeString();
    std::cout << "[" << timestamp << "] [DEBUG] " << message << std::endl;

    std::ofstream log_file(GlobalConstants::LOG_FILE, std::ios::app);
    if (log_file.is_open()) {
        log_file << "[" << timestamp << "] [DEBUG] " << message << std::endl;
    }
}


// --- Time ---

uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::tm buf;
    localtime_s(&buf, &in_time_t);

    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}


// --- Input & Keybinding Helpers ---

int getCurrentModifierState() {
    int mod = MOD_NONE;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mod |= MOD_CONTROL;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) mod |= MOD_SHIFT;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) mod |= MOD_ALT; // VK_MENU is Alt
    return mod;
}

std::string getVirtualKeyName(unsigned int vk) {
    // This is a simplified version. A full implementation would be much larger.
    switch (vk) {
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
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        case VK_ESCAPE: return "Escape";
        case VK_SPACE: return "Space";
        default:
            char key_char = static_cast<char>(MapVirtualKeyA(vk, MAPVK_VK_TO_CHAR));
            if (isprint(key_char)) {
                return std::string(1, key_char);
            }
            return "VK_" + std::to_string(vk);
    }
}

std::string getKeybindingString(unsigned int vk, int mod) {
    std::string result;
    if (mod & MOD_CONTROL) result += "Ctrl + ";
    if (mod & MOD_SHIFT) result += "Shift + ";
    if (mod & MOD_ALT) result += "Alt + ";
    result += getVirtualKeyName(vk);
    return result;
}


// --- Enum to String Converters ---

std::string playerMovementStateToString(PlayerMovementState state) {
    switch (state) {
        case PlayerMovementState::Stationary: return "Stationary";
        case PlayerMovementState::Walking:    return "Walking";
        case PlayerMovementState::Running:    return "Running";
        case PlayerMovementState::Crouching:  return "Crouching";
        case PlayerMovementState::Jumping:    return "Jumping";
        case PlayerMovementState::Falling:    return "Falling";
        case PlayerMovementState::Sprinting:  return "Sprinting";
        case PlayerMovementState::Strafing:   return "Strafing";
        case PlayerMovementState::Sliding:    return "Sliding";
        default:                              return "Unknown";
    }
}

// =============================================================================
// STRUCT METHOD IMPLEMENTATIONS
// =============================================================================

// --- TargetInfo Methods ---

bool TargetInfo::isValid() const {
    return width > 0 && height > 0 && confidence > 0.0 && confidence <= 1.0;
}

double TargetInfo::getAge() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - first_detected).count();
}

void TargetInfo::updatePosition(int new_x, int new_y) {
    // Calculate velocity based on position change
    if (frame_count > 0) {
        velocity_x = static_cast<double>(new_x - x);
        velocity_y = static_cast<double>(new_y - y);
    }
    
    x = new_x;
    y = new_y;
    last_updated = std::chrono::steady_clock::now();
    frame_count++;
}

void TargetInfo::calculateVelocity() {
    // This would typically be called with a time delta
    // For now, we'll use a simple calculation
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_updated).count();
    if (time_diff > 0) {
        velocity_x = static_cast<double>(x) / time_diff;
        velocity_y = static_cast<double>(y) / time_diff;
    }
}

void TargetInfo::predict(double time_ahead_ms) {
    predicted_x = x + (velocity_x * time_ahead_ms);
    predicted_y = y + (velocity_y * time_ahead_ms);
}

// --- MovementCommand Methods ---

bool MovementCommand::isValid() const {
    return std::isfinite(delta_x) && std::isfinite(delta_y) && 
           smoothing_factor >= 0.0 && smoothing_factor <= 1.0 &&
           confidence >= 0.0 && confidence <= 1.0;
}

double MovementCommand::getAge() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - timestamp).count();
}

void MovementCommand::applySmoothing(double factor) {
    smoothing_factor = std::clamp(factor, 0.0, 1.0);
    delta_x *= smoothing_factor;
    delta_y *= smoothing_factor;
}

// --- WeaponProfile Methods ---

bool WeaponProfile::isValid() const {
    return !name.empty() && 
           sensitivity > 0.0 && smoothing >= 0.0 && smoothing <= 1.0 &&
           prediction >= 0.0 && prediction <= 1.0 &&
           recoil_compensation >= 0.0 && recoil_compensation <= 1.0 &&
           aim_speed > 0.0 && trigger_delay >= 0.0 &&
           headshot_multiplier > 0.0 && body_multiplier > 0.0 && limb_multiplier > 0.0 &&
           max_range > 0.0 && optimal_range > 0.0 && min_range > 0.0 &&
           aim_fov > 0.0 && trigger_fov > 0.0 && scan_fov > 0.0 &&
           reaction_time >= 0.0 && acquisition_time >= 0.0 && lock_time >= 0.0;
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
    name = "Default";
    type = WeaponType::UNKNOWN;
    sensitivity = 1.0;
    smoothing = 0.75;
    prediction = 0.5;
    recoil_compensation = 0.8;
    aim_speed = 1.0;
    trigger_delay = 50.0;
    headshot_multiplier = 1.5;
    body_multiplier = 1.0;
    limb_multiplier = 0.8;
    max_range = 1000.0;
    optimal_range = 300.0;
    min_range = 50.0;
    aim_fov = 100.0;
    trigger_fov = 50.0;
    scan_fov = 200.0;
    enable_prediction = true;
    enable_recoil_comp = true;
    enable_auto_aim = false;
    enable_trigger_bot = false;
    enable_silent_aim = false;
    enable_adaptive_sensitivity = true;
    reaction_time = 100.0;
    acquisition_time = 50.0;
    lock_time = 200.0;
    usage_count = 0;
    last_used = std::chrono::steady_clock::now();
    effectiveness_rating = 0.0;
    accuracy_rating = 0.0;
    fire_mode = FireMode::Single;
    fire_delay_base = 50.0;
    fire_delay_variance = 5.0;
    smoothing_factor = 0.75;
    prediction_aggressiveness = 0.5;
    pid_states.clear();
    recoil_pattern.clear();
    advanced.resetToDefaults();
}

void WeaponProfile::incrementUsage() {
    usage_count++;
    last_used = std::chrono::steady_clock::now();
}

void WeaponProfile::updateEffectiveness(double rating) {
    effectiveness_rating = std::clamp(rating, 0.0, 1.0);
}

void WeaponProfile::initializeDefaultPIDStates() {
    pid_states.clear();
    pid_states["stationary"] = PIDParameters(0.7, 0.3, 0.2);
    pid_states["walking"] = PIDParameters(0.8, 0.4, 0.3);
    pid_states["running"] = PIDParameters(0.9, 0.5, 0.4);
    pid_states["crouching"] = PIDParameters(0.6, 0.2, 0.1);
    pid_states["jumping"] = PIDParameters(1.0, 0.6, 0.5);
}

PIDParameters WeaponProfile::getPIDParameters(const std::string& movement_state) const {
    auto it = pid_states.find(movement_state);
    if (it != pid_states.end()) {
        return it->second;
    }
    return PIDParameters(); // Return default parameters
}

void WeaponProfile::setPIDParameters(const std::string& movement_state, const PIDParameters& params) {
    pid_states[movement_state] = params;
}

bool WeaponProfile::hasPIDState(const std::string& movement_state) const {
    return pid_states.find(movement_state) != pid_states.end();
}

std::vector<std::string> WeaponProfile::getAvailablePIDStates() const {
    std::vector<std::string> states;
    for (const auto& pair : pid_states) {
        states.push_back(pair.first);
    }
    return states;
}

void WeaponProfile::addRecoilPoint(double x, double y) {
    recoil_pattern.emplace_back(x, y);
}

void WeaponProfile::addRecoilPoint(const RecoilPoint& point) {
    recoil_pattern.push_back(point);
}

void WeaponProfile::clearRecoilPattern() {
    recoil_pattern.clear();
}

RecoilPoint WeaponProfile::getRecoilOffset(int shot_number) const {
    if (recoil_pattern.empty()) {
        return RecoilPoint();
    }
    size_t index = static_cast<size_t>(shot_number) % recoil_pattern.size();
    return recoil_pattern[index];
}

size_t WeaponProfile::getRecoilPatternSize() const {
    return recoil_pattern.size();
}

void WeaponProfile::scaleRecoilPattern(double factor) {
    for (auto& point : recoil_pattern) {
        point.x *= factor;
        point.y *= factor;
    }
}

void WeaponProfile::normalizeRecoilPattern() {
    if (recoil_pattern.empty()) return;
    
    double max_magnitude = 0.0;
    for (const auto& point : recoil_pattern) {
        max_magnitude = std::max(max_magnitude, point.magnitude());
    }
    
    if (max_magnitude > 0.0) {
        scaleRecoilPattern(1.0 / max_magnitude);
    }
}

void WeaponProfile::updateAccuracy(double accuracy) {
    accuracy_rating = std::clamp(accuracy, 0.0, 1.0);
}

double WeaponProfile::getUsageScore() const {
    auto now = std::chrono::steady_clock::now();
    auto time_since_use = std::chrono::duration_cast<std::chrono::seconds>(now - last_used).count();
    
    // Decay factor based on time since last use
    double decay_factor = std::exp(-time_since_use / 3600.0); // 1 hour half-life
    
    return effectiveness_rating * decay_factor;
}

std::chrono::duration<double> WeaponProfile::getTimeSinceLastUse() const {
    auto now = std::chrono::steady_clock::now();
    return now - last_used;
}

std::string WeaponProfile::fireModeString() const {
    switch (fire_mode) {
        case FireMode::Single: return "Single";
        case FireMode::Controlled: return "Controlled";
        case FireMode::Automatic: return "Automatic";
        case FireMode::Rapid: return "Rapid";
        case FireMode::Tactical: return "Tactical";
        case FireMode::Custom: return "Custom";
        default: return "Unknown";
    }
}

void WeaponProfile::resetStatistics() {
    usage_count = 0;
    last_used = std::chrono::steady_clock::now();
    effectiveness_rating = 0.0;
    accuracy_rating = 0.0;
}

std::string WeaponProfile::toString() const {
    std::ostringstream ss;
    ss << "WeaponProfile{name='" << name << "', type=" << typeToString() 
       << ", sensitivity=" << sensitivity << ", smoothing=" << smoothing
       << ", usage_count=" << usage_count << ", effectiveness=" << effectiveness_rating
       << ", accuracy=" << accuracy_rating << "}";
    return ss.str();
}

bool WeaponProfile::operator==(const WeaponProfile& other) const {
    return name == other.name && type == other.type &&
           std::abs(sensitivity - other.sensitivity) < 1e-6 &&
           std::abs(smoothing - other.smoothing) < 1e-6 &&
           std::abs(prediction - other.prediction) < 1e-6;
}

bool WeaponProfile::operator!=(const WeaponProfile& other) const {
    return !(*this == other);
}