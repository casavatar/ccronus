// config.h - FINAL CORRECTED VERSION v5.2.1
// description: Header for the configuration management system.
// version: 5.2.1 - Removed duplicate member declarations in AppSettings.
// date: 2025-07-21
// project: Tactical Aim Assist

#pragma once

#include <string>
#include <vector>
#include <mutex>

#include "globals.h"
#include "assist_optimized.h"

struct ConfigValidationResult {
    bool is_valid = true;
    std::vector<std::string> issues;
    void addError(const std::string& error) { is_valid = false; issues.push_back("[ERROR] " + error); }
    void addWarning(const std::string& warning) { issues.push_back("[WARN] " + warning); }
    std::string getSummary() const { return is_valid ? "Valid" : "Invalid (" + std::to_string(issues.size()) + " issues)"; }
};

struct AppSettings {
    // General
    bool assist_enabled = false;
    bool debug_mode_enabled = false;
    bool adaptive_smoothing_enabled = true;
    bool audio_alerts_enabled = true;
    bool verbose_logging_enabled = false;
    int monitor_offset_x = 0;
    int monitor_offset_y = 0;
    double dpi_scale = 1.0;

    // Display
    int screen_width = 1920;
    int screen_height = 1080;
    int refresh_rate = 60;
    int target_monitor = 1;
    bool vsync_enabled = true;

    // Hotkeys
    Keybindings keybindings;

    // Advanced
    bool simd_enabled = true;
    bool multithreading_enabled = true;
    bool show_metrics_enabled = false;

    // Performance
    bool performance_monitoring_enabled = true;
    bool profiling_enabled = false;
    bool memory_tracking_enabled = true;
    bool cpu_monitoring_enabled = true;
    bool fps_tracking_enabled = true;
    bool file_output_enabled = true;
    bool simd_optimization_enabled = true;
    int max_fps = 240;

    // Aim Assist
    AimAssistConfig aim_config;
    TargetPriorityConfig priority_config;
    std::string anti_detection_level = "medium";

    // Weapon Profiles
    std::vector<WeaponProfile> weapon_profiles;
    int active_profile_index = 0;

    // Memory Pools
    bool memory_pooling_enabled = true;
    int initial_pool_size = 32;
    int max_pool_size = 1024;
    int pool_growth_factor = 2;
    bool allow_pool_expansion = true;

    // Input
    double mouse_sensitivity = 1.0;
    int polling_rate_hz = 1000;
    bool raw_input_enabled = true;

    // GUI
    int gui_update_interval_ms = 50;
    bool show_performance_overlay = false;
    double gui_transparency = 0.9;
};

class ConfigManager {
public:
    AppSettings loadConfiguration(const std::string& filename);
    bool saveConfiguration(const std::string& filename, const StateManager& stateMgr);
    void applyDefaultConfiguration(StateManager& stateMgr);
};

// Global C-style API
bool loadConfiguration(const std::string& filename);
bool saveConfiguration(const std::string& filename);
void applyDefaultConfiguration();
ConfigValidationResult validateConfiguration(const AppSettings& settings);