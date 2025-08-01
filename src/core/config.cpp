// config.cpp - FINAL REFACTORED VERSION v5.2.0
// description: Implementation of the configuration management system.
// version: 5.2.0 - Implemented full loading/saving for detailed weapon profiles.
// date: 2025-07-21
// project: Tactical Aim Assist

#include "config.h"
#include "state_manager.h"
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../external/nlohmann/include/nlohmann/json.hpp"
using json = nlohmann::json;

// Helper to convert string to WeaponType enum
WeaponType stringToWeaponType(const std::string& str) {
    if (str == "Assault Rifle") return WeaponType::ASSAULT_RIFLE;
    if (str == "Sniper Rifle") return WeaponType::SNIPER_RIFLE;
    if (str == "SMG") return WeaponType::SMG;
    // ... other types
    return WeaponType::UNKNOWN;
}

// Helper to convert WeaponType enum to string
std::string weaponTypeToString(WeaponType type) {
    switch (type) {
        case WeaponType::ASSAULT_RIFLE: return "Assault Rifle";
        case WeaponType::SNIPER_RIFLE: return "Sniper Rifle";
        case WeaponType::SMG: return "SMG";
        // ... other types
        default: return "Unknown";
    }
}

AppSettings ConfigManager::loadConfiguration(const std::string& filename) {
    AppSettings settings;
    logMessage("ConfigManager: Loading configuration from: " + filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        logWarning("ConfigManager: Config file not found. Using default settings.");
        return settings;
    }

    json config = json::parse(file, nullptr, false);
    if (config.is_discarded()) {
        logError("ConfigManager: JSON parsing error in " + filename + ". Using default settings.");
        return AppSettings{};
    }

    // --- General Settings ---
    if (config.contains("general")) {
        settings.assist_enabled = config["general"].value("assist_enabled", false);
        settings.active_profile_index = config["general"].value("active_profile_index", 0);
    }
    
    // --- Weapon Profiles section ---
    if (config.contains("weapon_profiles") && config["weapon_profiles"].is_array()) {
        settings.weapon_profiles.clear(); // Clear default profiles
        for (const auto& profile_json : config["weapon_profiles"]) {
            WeaponProfile p;
            p.name = profile_json.value("name", "Unnamed Profile");
            p.type = stringToWeaponType(profile_json.value("type", "Unknown"));
            p.sensitivity = profile_json.value("sensitivity", 1.0);
            p.smoothing = profile_json.value("smoothing", 0.75);
            p.recoil_compensation = profile_json.value("recoil_compensation", 0.8);
            p.aim_fov = profile_json.value("aim_fov", 100.0);
            p.trigger_fov = profile_json.value("trigger_fov", 50.0);
            p.max_range = profile_json.value("max_range", 1000.0);
            p.enable_prediction = profile_json.value("enable_prediction", true);
            p.enable_trigger_bot = profile_json.value("enable_trigger_bot", false);
            settings.weapon_profiles.push_back(p);
        }
    }

    logMessage("ConfigManager: Configuration loaded successfully.");
    return settings;
}

bool ConfigManager::saveConfiguration(const std::string& filename, const StateManager& stateMgr) {
    logMessage("ConfigManager: Saving configuration to: " + filename);
    if (!stateMgr.isInitialized()) {
        logError("ConfigManager: StateManager not initialized. Cannot save.");
        return false;
    }

    json config;
    const AppSettings& settings = stateMgr.getAppSettings();

    // --- General Settings ---
    config["general"] = {
        {"assist_enabled", settings.assist_enabled},
        {"active_profile_index", settings.active_profile_index}
    };
    
    // --- Weapon Profiles section ---
    json profiles_array = json::array();
    for (const auto& p : settings.weapon_profiles) {
        json profile_json;
        profile_json["name"] = p.name;
        profile_json["type"] = weaponTypeToString(p.type);
        profile_json["sensitivity"] = p.sensitivity;
        profile_json["smoothing"] = p.smoothing;
        profile_json["recoil_compensation"] = p.recoil_compensation;
        profile_json["aim_fov"] = p.aim_fov;
        profile_json["trigger_fov"] = p.trigger_fov;
        profile_json["max_range"] = p.max_range;
        profile_json["enable_prediction"] = p.enable_prediction;
        profile_json["enable_trigger_bot"] = p.enable_trigger_bot;
        profiles_array.push_back(profile_json);
    }
    config["weapon_profiles"] = profiles_array;

    std::ofstream file(filename);
    if (!file.is_open()) {
        logError("ConfigManager: Failed to open file for writing: " + filename);
        return false;
    }

    file << std::setw(4) << config << std::endl;
    logMessage("ConfigManager: Configuration saved successfully.");
    return true;
}

void ConfigManager::applyDefaultConfiguration(StateManager& stateMgr) {
    logMessage("ConfigManager: Applying default configuration to StateManager.");
    AppSettings default_settings;
    // Create a default profile if none exist
    if (default_settings.weapon_profiles.empty()) {
        WeaponProfile default_profile;
        default_profile.name = "Default AK-47";
        default_profile.type = WeaponType::ASSAULT_RIFLE;
        default_settings.weapon_profiles.push_back(default_profile);
    }
    stateMgr.setAppSettings(default_settings);
}

// Global C-style API
bool loadConfiguration(const std::string& filename) {
    StateManager* stateMgr = getStateManager();
    if (!stateMgr) {
        logError("Global loadConfiguration: StateManager not initialized.");
        return false;
    }
    ConfigManager configMgr;
    AppSettings settings = configMgr.loadConfiguration(filename);
    stateMgr->setAppSettings(settings);
    logMessage("Global loadConfiguration: Settings applied to StateManager.");
    return true;
}

bool saveConfiguration(const std::string& filename) {
    StateManager* stateMgr = getStateManager();
    if (!stateMgr) {
        logError("Global saveConfiguration: StateManager not initialized.");
        return false;
    }
    ConfigManager configMgr;
    return configMgr.saveConfiguration(filename, *stateMgr);
}

void applyDefaultConfiguration() {
    StateManager* stateMgr = getStateManager();
    if (!stateMgr) {
        logError("Global applyDefaultConfiguration: StateManager not initialized.");
        return;
    }
    ConfigManager configMgr;
    configMgr.applyDefaultConfiguration(*stateMgr);
}

ConfigValidationResult validateConfiguration(const AppSettings& settings) {
    ConfigValidationResult result;
    const auto& aim = settings.aim_config;

    if (aim.global_sensitivity < 0.1 || aim.global_sensitivity > 10.0) {
        result.addError("Global sensitivity out of valid range (0.1-10.0).");
    }

    if (aim.aim_fov < 1.0 || aim.aim_fov > 500.0) {
        result.addError("Aim FOV out of valid range (1.0-500.0).");
    }

    logMessage("Validation complete: " + result.getSummary());
    return result;
}