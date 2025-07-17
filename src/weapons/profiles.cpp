// profiles.cpp - WEAPON PROFILES IMPLEMENTATION v3.1.9
// description: Complete weapon profiles management system - CORRECTED VERSION
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.9 - Fixed all compilation errors and member issues
// date: 2025-07-17
// project: Tactical Aim Assist

#include "profiles.h"
#include "../core/globals.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <cmath>

// Include nlohmann/json for JSON handling
#include "../external/nlohmann/include/nlohmann/json.hpp"
using json = nlohmann::json;
using namespace std::chrono;

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

WeaponProfileManager* WeaponProfileManager::instance_ = nullptr;

// =============================================================================
// FIRE MODE UTILITY FUNCTIONS
// =============================================================================

std::string fireModeToString(FireMode mode) {
    switch (mode) {
        case FireMode::Single: return "Single";
        case FireMode::Controlled: return "Controlled";
        case FireMode::Automatic: return "Automatic";
        case FireMode::Rapid: return "Rapid";
        case FireMode::Tactical: return "Tactical";
        case FireMode::Custom: return "Custom";
        default: return "Unknown";
    }
}

FireMode stringToFireMode(const std::string& mode_str) {
    if (mode_str == "Single") return FireMode::Single;
    if (mode_str == "Controlled") return FireMode::Controlled;
    if (mode_str == "Automatic") return FireMode::Automatic;
    if (mode_str == "Rapid") return FireMode::Rapid;
    if (mode_str == "Tactical") return FireMode::Tactical;
    if (mode_str == "Custom") return FireMode::Custom;
    return FireMode::Single; // Default fallback
}

bool isFireModeValid(FireMode mode) {
    return mode >= FireMode::Single && mode <= FireMode::Custom;
}

// =============================================================================
// WEAPON PROFILE IMPLEMENTATION
// =============================================================================

void WeaponProfile::initializeDefaultPIDStates() {
    // Initialize PID parameters for different movement states
    pid_states["stationary"] = PIDParameters(0.7, 0.3, 0.2, 50.0, 100.0, 25.0);
    pid_states["walking"] = PIDParameters(0.75, 0.22, 0.28, 45.0, 90.0, 23.0);
    pid_states["sprinting"] = PIDParameters(0.65, 0.18, 0.25, 40.0, 80.0, 20.0);
    pid_states["strafing"] = PIDParameters(0.85, 0.18, 0.35, 55.0, 110.0, 28.0);
    pid_states["sliding"] = PIDParameters(0.6, 0.15, 0.22, 35.0, 70.0, 18.0);
    pid_states["crouching"] = PIDParameters(0.8, 0.35, 0.25, 40.0, 80.0, 20.0);
    pid_states["jumping"] = PIDParameters(0.5, 0.1, 0.15, 30.0, 60.0, 15.0);
    pid_states["falling"] = PIDParameters(0.45, 0.08, 0.12, 25.0, 50.0, 12.0);
}

bool WeaponProfile::isValid() const {
    // Basic validation
    if (name.empty()) return false;
    if (!isFireModeValid(fire_mode)) return false;
    if (sensitivity < 0.1 || sensitivity > 10.0) return false;
    if (smoothing < 0.0 || smoothing > 1.0) return false;
    if (prediction < 0.0 || prediction > 1.0) return false;
    if (fire_delay_base < 0 || fire_delay_variance < 0) return false;
    if (smoothing_factor < 0.0 || smoothing_factor > 1.0) return false;
    if (prediction_aggressiveness < 0.0 || prediction_aggressiveness > 1.0) return false;
    
    // Validate PID states
    for (const auto& [state, params] : pid_states) {
        if (!params.isValid()) return false;
    }
    
    // Validate recoil pattern
    for (const auto& point : recoil_pattern) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
    }
    
    return true;
}

PIDParameters WeaponProfile::getPIDParameters(const std::string& movement_state) const {
    auto it = pid_states.find(movement_state);
    if (it != pid_states.end()) {
        return it->second;
    }
    // Return default stationary parameters if state not found
    auto default_it = pid_states.find("stationary");
    if (default_it != pid_states.end()) {
        return default_it->second;
    }
    return PIDParameters(); // Fallback to default constructor
}

void WeaponProfile::setPIDParameters(const std::string& movement_state, const PIDParameters& params) {
    if (params.isValid()) {
        pid_states[movement_state] = params;
    }
}

bool WeaponProfile::hasPIDState(const std::string& movement_state) const {
    return pid_states.find(movement_state) != pid_states.end();
}

std::vector<std::string> WeaponProfile::getAvailablePIDStates() const {
    std::vector<std::string> states;
    for (const auto& [state, _] : pid_states) {
        states.push_back(state);
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
    if (recoil_pattern.empty() || shot_number < 0) {
        return RecoilPoint(0.0, 0.0);
    }
    
    // Cycle through pattern if shot number exceeds pattern length
    size_t index = shot_number % recoil_pattern.size();
    return recoil_pattern[index];
}

size_t WeaponProfile::getRecoilPatternSize() const {
    return recoil_pattern.size();
}

void WeaponProfile::scaleRecoilPattern(double factor) {
    for (auto& point : recoil_pattern) {
        point = point * factor;
    }
}

void WeaponProfile::normalizeRecoilPattern() {
    if (recoil_pattern.empty()) return;
    
    // Find maximum magnitude
    double max_magnitude = 0.0;
    for (const auto& point : recoil_pattern) {
        max_magnitude = std::max(max_magnitude, point.magnitude());
    }
    
    // Normalize if max magnitude is greater than 0
    if (max_magnitude > 0.0) {
        double scale_factor = 1.0 / max_magnitude;
        scaleRecoilPattern(scale_factor);
    }
}

void WeaponProfile::incrementUsage() {
    usage_count++;
    last_used = steady_clock::now();
}

void WeaponProfile::updateEffectiveness(double rating) {
    // Exponential moving average
    double alpha = 0.1; // Learning rate
    effectiveness_rating = (1.0 - alpha) * effectiveness_rating + alpha * rating;
}

void WeaponProfile::updateAccuracy(double accuracy) {
    // Exponential moving average
    double alpha = 0.1; // Learning rate
    accuracy_rating = (1.0 - alpha) * accuracy_rating + alpha * accuracy;
}

double WeaponProfile::getUsageScore() const {
    // Combine usage count and recency
    auto time_since_use = getTimeSinceLastUse();
    double recency_factor = std::exp(-time_since_use.count() / 3600.0); // Decay over hours
    return static_cast<double>(usage_count) * recency_factor;
}

std::chrono::duration<double> WeaponProfile::getTimeSinceLastUse() const {
    return steady_clock::now() - last_used;
}

std::string WeaponProfile::fireModeString() const {
    return fireModeToString(fire_mode);
}

void WeaponProfile::resetToDefaults() {
    sensitivity = 1.0;
    smoothing = 0.75;
    prediction = 0.5;
    recoil_compensation = true;
    fire_delay_base = 50;
    fire_delay_variance = 5;
    smoothing_factor = 0.75;
    prediction_aggressiveness = 0.5;
    
    initializeDefaultPIDStates();
    recoil_pattern.clear();
    
    // Reset advanced settings
    advanced = AdvancedSettings();
}

void WeaponProfile::resetStatistics() {
    usage_count = 0;
    last_used = steady_clock::now();
    effectiveness_rating = 0.0;
    accuracy_rating = 0.0;
}

std::string WeaponProfile::toString() const {
    std::ostringstream oss;
    oss << "WeaponProfile{";
    oss << "name: " << name;
    oss << ", fire_mode: " << fireModeString();
    oss << ", sensitivity: " << sensitivity;
    oss << ", smoothing: " << smoothing;
    oss << ", prediction: " << prediction;
    oss << ", recoil_compensation: " << (recoil_compensation ? "true" : "false");
    oss << ", usage_count: " << usage_count;
    oss << ", effectiveness: " << effectiveness_rating;
    oss << ", accuracy: " << accuracy_rating;
    oss << "}";
    return oss.str();
}

bool WeaponProfile::operator==(const WeaponProfile& other) const {
    return name == other.name &&
           fire_mode == other.fire_mode &&
           std::abs(sensitivity - other.sensitivity) < 1e-6 &&
           std::abs(smoothing - other.smoothing) < 1e-6 &&
           std::abs(prediction - other.prediction) < 1e-6;
}

bool WeaponProfile::operator!=(const WeaponProfile& other) const {
    return !(*this == other);
}

// =============================================================================
// WEAPON PROFILE COLLECTION IMPLEMENTATION
// =============================================================================

WeaponProfileCollection::WeaponProfileCollection(const std::string& name)
    : collection_name_(name) {}

WeaponProfileCollection::WeaponProfileCollection(const WeaponProfileCollection& other)
    : collection_name_(other.collection_name_),
      file_path_(other.file_path_),
      auto_save_(other.auto_save_),
      modified_(other.modified_),
      active_profile_index_(other.active_profile_index_) {
    
    // Deep copy profiles
    for (const auto& profile : other.profiles_) {
        profiles_.push_back(std::make_unique<WeaponProfile>(*profile));
    }
}

WeaponProfileCollection& WeaponProfileCollection::operator=(const WeaponProfileCollection& other) {
    if (this != &other) {
        collection_name_ = other.collection_name_;
        file_path_ = other.file_path_;
        auto_save_ = other.auto_save_;
        modified_ = other.modified_;
        active_profile_index_ = other.active_profile_index_;
        
        // Clear and deep copy profiles
        profiles_.clear();
        for (const auto& profile : other.profiles_) {
            profiles_.push_back(std::make_unique<WeaponProfile>(*profile));
        }
    }
    return *this;
}

WeaponProfileCollection::WeaponProfileCollection(WeaponProfileCollection&& other) noexcept
    : profiles_(std::move(other.profiles_)),
      active_profile_index_(other.active_profile_index_),
      collection_name_(std::move(other.collection_name_)),
      file_path_(std::move(other.file_path_)),
      auto_save_(other.auto_save_),
      modified_(other.modified_) {
    
    other.active_profile_index_ = 0;
    other.auto_save_ = true;
    other.modified_ = false;
}

WeaponProfileCollection& WeaponProfileCollection::operator=(WeaponProfileCollection&& other) noexcept {
    if (this != &other) {
        profiles_ = std::move(other.profiles_);
        active_profile_index_ = other.active_profile_index_;
        collection_name_ = std::move(other.collection_name_);
        file_path_ = std::move(other.file_path_);
        auto_save_ = other.auto_save_;
        modified_ = other.modified_;
        
        other.active_profile_index_ = 0;
        other.auto_save_ = true;
        other.modified_ = false;
    }
    return *this;
}

WeaponProfileCollection::~WeaponProfileCollection() {
    if (auto_save_ && modified_ && !file_path_.empty()) {
        saveToFile(file_path_);
    }
}

bool WeaponProfileCollection::addProfile(std::unique_ptr<WeaponProfile> profile) {
    if (!profile || !profile->isValid()) {
        return false;
    }
    
    // Check for duplicate names
    for (const auto& existing : profiles_) {
        if (existing->name == profile->name) {
            return false; // Name already exists
        }
    }
    
    profiles_.push_back(std::move(profile));
    markModified();
    return true;
}

bool WeaponProfileCollection::addProfile(const WeaponProfile& profile) {
    return addProfile(std::make_unique<WeaponProfile>(profile));
}

bool WeaponProfileCollection::removeProfile(int index) {
    if (index < 0 || index >= static_cast<int>(profiles_.size())) {
        return false;
    }
    
    profiles_.erase(profiles_.begin() + index);
    
    // Adjust active profile index
    if (active_profile_index_ >= index && active_profile_index_ > 0) {
        active_profile_index_--;
    } else if (active_profile_index_ >= static_cast<int>(profiles_.size())) {
        active_profile_index_ = std::max(0, static_cast<int>(profiles_.size()) - 1);
    }
    
    markModified();
    return true;
}

bool WeaponProfileCollection::removeProfile(const std::string& name) {
    int index = findProfileIndex(name);
    if (index >= 0) {
        return removeProfile(index);
    }
    return false;
}

WeaponProfile* WeaponProfileCollection::getProfile(int index) {
    if (index >= 0 && index < static_cast<int>(profiles_.size())) {
        return profiles_[index].get();
    }
    return nullptr;
}

const WeaponProfile* WeaponProfileCollection::getProfile(int index) const {
    if (index >= 0 && index < static_cast<int>(profiles_.size())) {
        return profiles_[index].get();
    }
    return nullptr;
}

WeaponProfile* WeaponProfileCollection::getProfile(const std::string& name) {
    int index = findProfileIndex(name);
    return getProfile(index);
}

const WeaponProfile* WeaponProfileCollection::getProfile(const std::string& name) const {
    int index = findProfileIndex(name);
    return getProfile(index);
}

WeaponProfile* WeaponProfileCollection::getActiveProfile() {
    return getProfile(active_profile_index_);
}

const WeaponProfile* WeaponProfileCollection::getActiveProfile() const {
    return getProfile(active_profile_index_);
}

bool WeaponProfileCollection::setActiveProfile(int index) {
    if (index >= 0 && index < static_cast<int>(profiles_.size())) {
        active_profile_index_ = index;
        return true;
    }
    return false;
}

bool WeaponProfileCollection::setActiveProfile(const std::string& name) {
    int index = findProfileIndex(name);
    if (index >= 0) {
        active_profile_index_ = index;
        return true;
    }
    return false;
}

int WeaponProfileCollection::getActiveProfileIndex() const {
    return active_profile_index_;
}

std::string WeaponProfileCollection::getActiveProfileName() const {
    const auto* profile = getActiveProfile();
    return profile ? profile->name : "";
}

bool WeaponProfileCollection::hasActiveProfile() const {
    return getActiveProfile() != nullptr;
}

size_t WeaponProfileCollection::size() const {
    return profiles_.size();
}

bool WeaponProfileCollection::empty() const {
    return profiles_.empty();
}

std::vector<std::string> WeaponProfileCollection::getProfileNames() const {
    std::vector<std::string> names;
    for (const auto& profile : profiles_) {
        names.push_back(profile->name);
    }
    return names;
}

std::vector<int> WeaponProfileCollection::getProfileIndices() const {
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(profiles_.size()); ++i) {
        indices.push_back(i);
    }
    return indices;
}

bool WeaponProfileCollection::hasProfile(const std::string& name) const {
    return findProfileIndex(name) >= 0;
}

int WeaponProfileCollection::findProfileIndex(const std::string& name) const {
    for (int i = 0; i < static_cast<int>(profiles_.size()); ++i) {
        if (profiles_[i]->name == name) {
            return i;
        }
    }
    return -1;
}

bool WeaponProfileCollection::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            logError("Cannot open weapon profiles file: " + filename);
            return false;
        }
        
        json j;
        file >> j;
        
        if (!j.contains("weapon_profiles") || !j["weapon_profiles"].is_array()) {
            logError("Invalid weapon profiles file format");
            return false;
        }
        
        profiles_.clear();
        
        for (const auto& profile_json : j["weapon_profiles"]) {
            auto profile = std::make_unique<WeaponProfile>();
            
            // Load basic properties
            profile->name = profile_json.value("name", "Unnamed");
            profile->fire_mode = stringToFireMode(profile_json.value("fire_mode", "Single"));
            profile->sensitivity = profile_json.value("sensitivity", 1.0);
            profile->smoothing = profile_json.value("smoothing", 0.75);
            profile->prediction = profile_json.value("prediction", 0.5);
            profile->recoil_compensation = profile_json.value("recoil_compensation", true);
            profile->fire_delay_base = profile_json.value("fire_delay_base", 50);
            profile->fire_delay_variance = profile_json.value("fire_delay_variance", 5);
            profile->smoothing_factor = profile_json.value("smoothing_factor", 0.75);
            profile->prediction_aggressiveness = profile_json.value("prediction_aggressiveness", 0.5);
            
            // Load PID states
            if (profile_json.contains("pid_states")) {
                profile->pid_states.clear();
                for (const auto& [state_name, pid_json] : profile_json["pid_states"].items()) {
                    PIDParameters params;
                    params.kp = pid_json.value("kp", 0.7);
                    params.ki = pid_json.value("ki", 0.3);
                    params.kd = pid_json.value("kd", 0.2);
                    params.max_output = pid_json.value("max_output", 50.0);
                    params.integral_limit = pid_json.value("integral_limit", 100.0);
                    params.derivative_limit = pid_json.value("derivative_limit", 25.0);
                    profile->pid_states[state_name] = params;
                }
            }
            
            // Load recoil pattern
            if (profile_json.contains("recoil_pattern")) {
                profile->recoil_pattern.clear();
                for (const auto& point_json : profile_json["recoil_pattern"]) {
                    if (point_json.is_array() && point_json.size() >= 2) {
                        double x = point_json[0];
                        double y = point_json[1];
                        profile->recoil_pattern.emplace_back(x, y);
                    }
                }
            }
            
            // Validate and add profile
            if (profile->isValid()) {
                profiles_.push_back(std::move(profile));
            } else {
                logWarning("Skipping invalid profile: " + profile->name);
            }
        }
        
        file_path_ = filename;
        markSaved();
        
        logMessage("Loaded " + std::to_string(profiles_.size()) + " weapon profiles from " + filename);
        return true;
        
    } catch (const json::exception& e) {
        logError("JSON error loading weapon profiles: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        logError("Error loading weapon profiles: " + std::string(e.what()));
        return false;
    }
}

bool WeaponProfileCollection::saveToFile(const std::string& filename) const {
    try {
        json j;
        j["weapon_profiles"] = json::array();
        
        for (const auto& profile : profiles_) {
            json profile_json;
            
            // Save basic properties
            profile_json["name"] = profile->name;
            profile_json["fire_mode"] = profile->fireModeString();
            profile_json["sensitivity"] = profile->sensitivity;
            profile_json["smoothing"] = profile->smoothing;
            profile_json["prediction"] = profile->prediction;
            profile_json["recoil_compensation"] = profile->recoil_compensation;
            profile_json["fire_delay_base"] = profile->fire_delay_base;
            profile_json["fire_delay_variance"] = profile->fire_delay_variance;
            profile_json["smoothing_factor"] = profile->smoothing_factor;
            profile_json["prediction_aggressiveness"] = profile->prediction_aggressiveness;
            
            // Save PID states
            profile_json["pid_states"] = json::object();
            for (const auto& [state_name, params] : profile->pid_states) {
                profile_json["pid_states"][state_name] = {
                    {"kp", params.kp},
                    {"ki", params.ki},
                    {"kd", params.kd},
                    {"max_output", params.max_output},
                    {"integral_limit", params.integral_limit},
                    {"derivative_limit", params.derivative_limit}
                };
            }
            
            // Save recoil pattern
            profile_json["recoil_pattern"] = json::array();
            for (const auto& point : profile->recoil_pattern) {
                profile_json["recoil_pattern"].push_back({point.x, point.y});
            }
            
            j["weapon_profiles"].push_back(profile_json);
        }
        
        // Add metadata
        j["metadata"] = {
            {"version", "3.1.9"},
            {"created", getCurrentTimeString()},
            {"profile_count", profiles_.size()},
            {"active_profile", active_profile_index_}
        };
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            logError("Cannot create weapon profiles file: " + filename);
            return false;
        }
        
        file << std::setw(2) << j << std::endl;
        file.close();
        
        logMessage("Saved " + std::to_string(profiles_.size()) + " weapon profiles to " + filename);
        return true;
        
    } catch (const json::exception& e) {
        logError("JSON error saving weapon profiles: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        logError("Error saving weapon profiles: " + std::string(e.what()));
        return false;
    }
}

void WeaponProfileCollection::setAutoSave(bool enabled) {
    auto_save_ = enabled;
}

bool WeaponProfileCollection::isAutoSaveEnabled() const {
    return auto_save_;
}

void WeaponProfileCollection::setFilePath(const std::string& path) {
    file_path_ = path;
}

std::string WeaponProfileCollection::getFilePath() const {
    return file_path_;
}

bool WeaponProfileCollection::isModified() const {
    return modified_;
}

void WeaponProfileCollection::markModified() {
    modified_ = true;
    if (auto_save_ && !file_path_.empty()) {
        saveToFile(file_path_);
    }
}

void WeaponProfileCollection::markSaved() {
    modified_ = false;
}

void WeaponProfileCollection::clear() {
    profiles_.clear();
    active_profile_index_ = 0;
    markModified();
}

WeaponProfile* WeaponProfileCollection::getMostUsedProfile() {
    if (profiles_.empty()) return nullptr;
    
    auto max_it = std::max_element(profiles_.begin(), profiles_.end(),
        [](const auto& a, const auto& b) {
            return a->getUsageScore() < b->getUsageScore();
        });
    
    return max_it->get();
}

WeaponProfile* WeaponProfileCollection::getMostEffectiveProfile() {
    if (profiles_.empty()) return nullptr;
    
    auto max_it = std::max_element(profiles_.begin(), profiles_.end(),
        [](const auto& a, const auto& b) {
            return a->effectiveness_rating < b->effectiveness_rating;
        });
    
    return max_it->get();
}

std::string WeaponProfileCollection::getCollectionName() const {
    return collection_name_;
}

void WeaponProfileCollection::setCollectionName(const std::string& name) {
    collection_name_ = name;
    markModified();
}

// =============================================================================
// WEAPON PROFILE MANAGER IMPLEMENTATION
// =============================================================================

WeaponProfileManager& WeaponProfileManager::getInstance() {
    if (instance_ == nullptr) {
        instance_ = new WeaponProfileManager();
    }
    return *instance_;
}

void WeaponProfileManager::destroyInstance() {
    delete instance_;
    instance_ = nullptr;
}

bool WeaponProfileManager::initialize(const std::string& profiles_dir) {
    if (initialized_) return true;
    
    profiles_directory_ = profiles_dir;
    
    // Create profiles directory if it doesn't exist
    try {
        std::filesystem::create_directories(profiles_directory_);
    } catch (const std::exception& e) {
        logError("Failed to create profiles directory: " + std::string(e.what()));
        return false;
    }
    
    // Load default profiles
    std::string profiles_file = profiles_directory_ + "/weapon_profiles.json";
    if (std::filesystem::exists(profiles_file)) {
        if (!profiles_.loadFromFile(profiles_file)) {
            logWarning("Failed to load existing profiles, creating defaults");
            resetToDefaults();
        }
    } else {
        logMessage("No existing profiles found, creating defaults");
        resetToDefaults();
    }
    
    profiles_.setFilePath(profiles_file);
    profiles_.setAutoSave(true);
    
    initialized_ = true;
    logMessage("Weapon Profile Manager initialized with " + std::to_string(profiles_.size()) + " profiles");
    
    return true;
}

bool WeaponProfileManager::isInitialized() const {
    return initialized_;
}

void WeaponProfileManager::shutdown() {
    if (initialized_) {
        if (profiles_.isAutoSaveEnabled() && !profiles_.getFilePath().empty()) {
            profiles_.saveToFile(profiles_.getFilePath());
        }
        initialized_ = false;
        logMessage("Weapon Profile Manager shutdown");
    }
}

WeaponProfileCollection& WeaponProfileManager::getProfiles() {
    return profiles_;
}

const WeaponProfileCollection& WeaponProfileManager::getProfiles() const {
    return profiles_;
}

WeaponProfile* WeaponProfileManager::getActiveProfile() {
    return profiles_.getActiveProfile();
}

const WeaponProfile* WeaponProfileManager::getActiveProfile() const {
    return profiles_.getActiveProfile();
}

bool WeaponProfileManager::setActiveProfile(const std::string& name) {
    return profiles_.setActiveProfile(name);
}

bool WeaponProfileManager::setActiveProfile(int index) {
    return profiles_.setActiveProfile(index);
}

bool WeaponProfileManager::addProfile(const WeaponProfile& profile) {
    return profiles_.addProfile(profile);
}

bool WeaponProfileManager::removeProfile(const std::string& name) {
    return profiles_.removeProfile(name);
}

bool WeaponProfileManager::loadProfiles(const std::string& filename) {
    std::string file_path = filename.empty() ? 
        profiles_directory_ + "/weapon_profiles.json" : filename;
    return profiles_.loadFromFile(file_path);
}

bool WeaponProfileManager::saveProfiles(const std::string& filename) const {
    std::string file_path = filename.empty() ? 
        profiles_directory_ + "/weapon_profiles.json" : filename;
    return profiles_.saveToFile(file_path);
}

WeaponProfile WeaponProfileManager::createDefaultProfile(const std::string& name) {
    WeaponProfile profile;
    profile.name = name;
    profile.fire_mode = FireMode::Single;
    profile.sensitivity = 1.0;
    profile.smoothing = 0.75;
    profile.prediction = 0.5;
    profile.recoil_compensation = false;
    return profile;
}

WeaponProfile WeaponProfileManager::createARProfile(const std::string& name) {
    WeaponProfile profile;
    profile.name = name;
    profile.fire_mode = FireMode::Automatic;
    profile.sensitivity = 1.0;
    profile.smoothing = 0.85;
    profile.prediction = 0.75;
    profile.recoil_compensation = true;
    profile.fire_delay_base = 85;
    profile.fire_delay_variance = 12;
    
    // Add typical AR recoil pattern
    profile.addRecoilPoint(0, 8);
    profile.addRecoilPoint(-2, 9);
    profile.addRecoilPoint(-2, 10);
    profile.addRecoilPoint(-1, 4);
    profile.addRecoilPoint(1, 6);
    profile.addRecoilPoint(2, 7);
    profile.addRecoilPoint(0, 5);
    profile.addRecoilPoint(-1, 3);
    
    return profile;
}

WeaponProfile WeaponProfileManager::createSMGProfile(const std::string& name) {
    WeaponProfile profile;
    profile.name = name;
    profile.fire_mode = FireMode::Rapid;
    profile.sensitivity = 1.2;
    profile.smoothing = 0.9;
    profile.prediction = 0.9;
    profile.recoil_compensation = true;
    profile.fire_delay_base = 15;
    profile.fire_delay_variance = 5;
    
    // Add typical SMG recoil pattern
    profile.addRecoilPoint(0, 2);
    profile.addRecoilPoint(1, 3);
    profile.addRecoilPoint(0, 2);
    profile.addRecoilPoint(-1, 4);
    profile.addRecoilPoint(0, 3);
    profile.addRecoilPoint(1, 2);
    profile.addRecoilPoint(-1, 3);
    profile.addRecoilPoint(0, 1);
    
    return profile;
}

WeaponProfile WeaponProfileManager::createSniperProfile(const std::string& name) {
    WeaponProfile profile;
    profile.name = name;
    profile.fire_mode = FireMode::Single;
    profile.sensitivity = 0.7;
    profile.smoothing = 0.9;
    profile.prediction = 0.4;
    profile.recoil_compensation = false;
    profile.fire_delay_base = 0;
    profile.fire_delay_variance = 0;
    
    // Minimal recoil pattern for sniper
    profile.addRecoilPoint(0, 0);
    profile.addRecoilPoint(0, -2);
    
    return profile;
}

WeaponProfile WeaponProfileManager::createPistolProfile(const std::string& name) {
    WeaponProfile profile;
    profile.name = name;
    profile.fire_mode = FireMode::Single;
    profile.sensitivity = 1.5;
    profile.smoothing = 0.95;
    profile.prediction = 0.95;
    profile.recoil_compensation = false;
    profile.fire_delay_base = 0;
    profile.fire_delay_variance = 0;
    
    // Light recoil pattern for pistol
    profile.addRecoilPoint(-1, 3);
    profile.addRecoilPoint(-1, 4);
    profile.addRecoilPoint(-1, 5);
    profile.addRecoilPoint(-2, 3);
    
    return profile;
}

void WeaponProfileManager::resetToDefaults() {
    profiles_.clear();
    
    // Create default profiles
    profiles_.addProfile(createDefaultProfile("Default"));
    profiles_.addProfile(createARProfile("Dynamic AR"));
    profiles_.addProfile(createARProfile("Controlled AR"));
    profiles_.addProfile(createSMGProfile("Rapid SMG"));
    profiles_.addProfile(createARProfile("Tactical AR"));
    profiles_.addProfile(createPistolProfile("Single-Fire Pistol"));
    profiles_.addProfile(createSniperProfile("Sniper Rifle"));
    
    profiles_.setActiveProfile(0);
    profiles_.markModified();
    
    logMessage("Reset to default weapon profiles");
}

std::string WeaponProfileManager::getProfilesDirectory() const {
    return profiles_directory_;
}

void WeaponProfileManager::setProfilesDirectory(const std::string& dir) {
    profiles_directory_ = dir;
}

std::vector<std::string> WeaponProfileManager::getAvailableProfileFiles() const {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(profiles_directory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        logError("Error scanning profiles directory: " + std::string(e.what()));
    }
    
    return files;
}

// =============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

std::string movementStateToString(PlayerMovementState state) {
    switch (state) {
        case PlayerMovementState::Stationary: return "stationary";
        case PlayerMovementState::Walking: return "walking";
        case PlayerMovementState::Sprinting: return "sprinting";
        case PlayerMovementState::Strafing: return "strafing";
        case PlayerMovementState::Sliding: return "sliding";
        case PlayerMovementState::Crouching: return "crouching";
        case PlayerMovementState::Jumping: return "jumping";
        case PlayerMovementState::Falling: return "falling";
        default: return "unknown";
    }
}

PlayerMovementState stringToMovementState(const std::string& state_str) {
    if (state_str == "stationary") return PlayerMovementState::Stationary;
    if (state_str == "walking") return PlayerMovementState::Walking;
    if (state_str == "sprinting") return PlayerMovementState::Sprinting;
    if (state_str == "strafing") return PlayerMovementState::Strafing;
    if (state_str == "sliding") return PlayerMovementState::Sliding;
    if (state_str == "crouching") return PlayerMovementState::Crouching;
    if (state_str == "jumping") return PlayerMovementState::Jumping;
    if (state_str == "falling") return PlayerMovementState::Falling;
    return PlayerMovementState::Stationary;
}

double calculateProfileSimilarity(const WeaponProfile& p1, const WeaponProfile& p2) {
    double similarity = 0.0;
    int factors = 0;
    
    // Compare basic parameters
    similarity += 1.0 - std::abs(p1.sensitivity - p2.sensitivity) / 10.0;
    similarity += 1.0 - std::abs(p1.smoothing - p2.smoothing);
    similarity += 1.0 - std::abs(p1.prediction - p2.prediction);
    factors += 3;
    
    // Compare fire mode
    if (p1.fire_mode == p2.fire_mode) {
        similarity += 1.0;
    }
    factors++;
    
    return similarity / factors;
}