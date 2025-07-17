// config.cpp - COMPLETE UPDATED VERSION v3.1.8
// description: Configuration management system with corrected ConfigValidationResult
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.8 - Fixed ConfigValidationResult structure and all related errors
// date: 2025-07-17
// project: Tactical Aim Assist

#include "config.h"
#include "globals.h"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <sstream>

// Include the JSON library
#include "../external/nlohmann/include/nlohmann/json.hpp"
// Use nlohmann::json namespace
using json = nlohmann::json;

// =============================================================================
// GLOBAL CONFIGURATION VARIABLES
// =============================================================================

ConfigValidationResult g_lastValidationResult;
std::string g_configFilePath = GlobalConstants::MAIN_CONFIG_FILE;
std::mutex g_configurationMutex;

// =============================================================================
// CONFIGURATION LOADING FUNCTIONS
// =============================================================================

bool loadConfiguration(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_configurationMutex);
    
    logMessage("📁 Loading configuration from: " + filename);
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            logWarning("Configuration file not found, using defaults: " + filename);
            applyDefaultConfiguration();
            return true; // Consider this success, we have defaults
        }
        
        // Parse JSON configuration
        json config;
        file >> config;
        
        // Load general settings
        if (config.contains("general")) {
            auto general = config["general"];
            g_aimAssistEnabled.store(general.value("assist_enabled", true));
            g_debugMode.store(general.value("debug_mode", false));
            g_adaptiveSensitivityEnabled.store(general.value("adaptive_smoothing", true));
            g_audioAlertsEnabled.store(general.value("audio_alerts", true));
            g_verboseLogging.store(general.value("verbose_logging", false));
            
            g_monitorOffsetX.store(general.value("monitor_offset_x", 0));
            g_monitorOffsetY.store(general.value("monitor_offset_y", 0));
            g_dpiScale.store(general.value("dpi_scale", 1.0));
        }
        
        // Load display settings
        if (config.contains("display")) {
            auto display = config["display"];
            g_monitorWidth.store(display.value("width", 1920));
            g_monitorHeight.store(display.value("height", 1080));
            g_refreshRate.store(display.value("refresh_rate", 60));
            g_targetMonitor.store(display.value("target_monitor", 1));
            g_vsyncEnabled.store(display.value("vsync", true));
        }
        
        // Load hotkeys
        if (config.contains("hotkeys")) {
            auto hotkeys = config["hotkeys"];
            g_toggleKey.store(hotkeys.value("toggle_key", 0x70)); // F1
            g_holdKey.store(hotkeys.value("hold_key", 0x71)); // F2
            g_mouseButton.store(hotkeys.value("mouse_button", 1)); // Left click
            g_holdMode.store(hotkeys.value("hold_mode", false));
        }
        
        // Load advanced settings
        if (config.contains("advanced")) {
            auto advanced = config["advanced"];
            g_enableSIMD.store(advanced.value("enable_simd", true));
            g_enableMultithreading.store(advanced.value("enable_multithreading", true));
            g_verboseLogging.store(advanced.value("verbose_logging", false));
            g_showMetrics.store(advanced.value("show_metrics", false));
        }
        
        // Load performance settings
        if (config.contains("performance")) {
            auto performance = config["performance"];
            g_performanceMonitoringEnabled.store(performance.value("enable_monitoring", true));
            g_profilingEnabled.store(performance.value("enable_profiling", true));
            g_memoryTrackingEnabled.store(performance.value("track_memory", true));
            g_cpuMonitoringEnabled.store(performance.value("monitor_cpu", true));
            g_fpsTrackingEnabled.store(performance.value("track_fps", true));
            g_fileOutputEnabled.store(performance.value("file_output", true));
            g_simdOptimizationEnabled.store(performance.value("simd_optimization", true));
            g_maxFps.store(performance.value("max_fps", 240));
        }
        
        // Load aim settings
        if (config.contains("aim")) {
            auto aim = config["aim"];
            g_globalSensitivityMultiplier.store(aim.value("sensitivity", GlobalConstants::DEFAULT_SENSITIVITY));
            g_aimSmoothingFactor.store(aim.value("smoothing_factor", GlobalConstants::DEFAULT_SMOOTHING_FACTOR));
            g_predictionTime.store(aim.value("prediction_time_ms", 50.0));
            g_triggerBotEnabled.store(aim.value("trigger_bot", false));
            g_silentAimEnabled.store(aim.value("silent_aim", false));
            g_aimFOV.store(aim.value("aim_fov", GlobalConstants::DEFAULT_AIM_FOV));
            g_triggerFOV.store(aim.value("trigger_fov", GlobalConstants::DEFAULT_TRIGGER_FOV));
            g_maxTargetDistance.store(aim.value("max_adjustment_distance", 500.0));
            
            if (aim.contains("anti_detection_level")) {
                g_antiDetectionLevel = aim["anti_detection_level"];
            }
        }
        
        // Load memory pool settings
        if (config.contains("memory_pools")) {
            auto memory_pools = config["memory_pools"];
            g_memoryPoolingEnabled.store(memory_pools.value("enable_pooling", true));
            g_initialPoolSize.store(memory_pools.value("initial_pool_size", 32));
            g_maxPoolSize.store(memory_pools.value("max_pool_size", 1024));
            g_poolGrowthFactor.store(memory_pools.value("growth_factor", 2));
            g_allowPoolExpansion.store(memory_pools.value("allow_expansion", true));
        }
        
        // Load input settings
        if (config.contains("input")) {
            auto input = config["input"];
            g_mouseSensitivity.store(input.value("mouse_sensitivity", 1.0));
            g_pollingRateHz.store(input.value("polling_rate_hz", 1000));
            g_rawInputEnabled.store(input.value("raw_input", true));
        }
        
        // Load GUI settings
        if (config.contains("gui")) {
            auto gui = config["gui"];
            g_guiUpdateIntervalMs.store(gui.value("update_interval_ms", 50));
            g_showPerformanceOverlay.store(gui.value("show_performance_overlay", false));
            g_guiTransparency.store(gui.value("transparency", 0.9));
        }
        
        // Sync duplicate variables for compatibility
        g_assistEnabled.store(g_aimAssistEnabled.load());
        g_sensitivity.store(g_globalSensitivityMultiplier.load());
        g_predictionTimeMs.store(g_predictionTime.load());
        g_smoothingFactor.store(g_aimSmoothingFactor.load());
        g_maxAdjustmentDistance.store(g_maxTargetDistance.load());
        
        file.close();
        
        // Validate loaded configuration
        if (!validateConfiguration()) {
            logError("Configuration validation failed, using defaults");
            applyDefaultConfiguration();
        }
        
        // Log successful loading
        logMessage("✅ Configuration loaded with " + std::to_string(getWeaponProfileCount()) + " weapon profiles");
        logMessage("  - Assist Enabled: " + std::string(g_aimAssistEnabled.load() ? "YES" : "NO"));
        logMessage("  - Debug Mode: " + std::string(g_debugMode.load() ? "YES" : "NO"));
        logMessage("  - Audio Alerts: " + std::string(g_audioAlertsEnabled.load() ? "YES" : "NO"));
        logMessage("  - Monitor: " + std::to_string(g_targetMonitor.load()) + " (" +
                   std::to_string(g_monitorWidth.load()) + "x" + std::to_string(g_monitorHeight.load()) + ")");
        
        return true;
        
    } catch (const json::parse_error& e) {
        logError("JSON parsing error: " + std::string(e.what()));
        applyDefaultConfiguration();
        return false;
    } catch (const json::type_error& e) {
        logError("JSON type error: " + std::string(e.what()));
        applyDefaultConfiguration();
        return false;
    } catch (const std::exception& e) {
        logError("Error loading configuration: " + std::string(e.what()));
        applyDefaultConfiguration();
        return false;
    }
}

bool saveConfiguration(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_configurationMutex);
    
    logMessage("💾 Saving configuration to: " + filename);
    
    try {
        json config;
        
        // Save general settings
        config["general"] = {
            {"assist_enabled", g_aimAssistEnabled.load()},
            {"debug_mode", g_debugMode.load()},
            {"adaptive_smoothing", g_adaptiveSensitivityEnabled.load()},
            {"audio_alerts", g_audioAlertsEnabled.load()},
            {"verbose_logging", g_verboseLogging.load()},
            {"monitor_offset_x", g_monitorOffsetX.load()},
            {"monitor_offset_y", g_monitorOffsetY.load()},
            {"dpi_scale", g_dpiScale.load()}
        };
        
        // Save display settings
        config["display"] = {
            {"target_monitor", g_targetMonitor.load()},
            {"width", g_monitorWidth.load()},
            {"height", g_monitorHeight.load()},
            {"refresh_rate", g_refreshRate.load()},
            {"vsync", g_vsyncEnabled.load()}
        };
        
        // Save hotkeys
        config["hotkeys"] = {
            {"toggle_key", g_toggleKey.load()},
            {"hold_key", g_holdKey.load()},
            {"mouse_button", g_mouseButton.load()},
            {"hold_mode", g_holdMode.load()}
        };
        
        // Save advanced settings
        config["advanced"] = {
            {"enable_simd", g_enableSIMD.load()},
            {"enable_multithreading", g_enableMultithreading.load()},
            {"verbose_logging", g_verboseLogging.load()},
            {"show_metrics", g_showMetrics.load()}
        };
        
        // Save performance settings
        config["performance"] = {
            {"enable_monitoring", g_performanceMonitoringEnabled.load()},
            {"enable_profiling", g_profilingEnabled.load()},
            {"track_memory", g_memoryTrackingEnabled.load()},
            {"monitor_cpu", g_cpuMonitoringEnabled.load()},
            {"track_fps", g_fpsTrackingEnabled.load()},
            {"file_output", g_fileOutputEnabled.load()},
            {"simd_optimization", g_simdOptimizationEnabled.load()},
            {"max_fps", g_maxFps.load()}
        };
        
        // Save aim settings
        config["aim"] = {
            {"sensitivity", g_globalSensitivityMultiplier.load()},
            {"smoothing_factor", g_aimSmoothingFactor.load()},
            {"prediction_time_ms", g_predictionTime.load()},
            {"trigger_bot", g_triggerBotEnabled.load()},
            {"silent_aim", g_silentAimEnabled.load()},
            {"aim_fov", g_aimFOV.load()},
            {"trigger_fov", g_triggerFOV.load()},
            {"max_adjustment_distance", g_maxTargetDistance.load()},
            {"anti_detection_level", g_antiDetectionLevel}
        };
        
        // Save memory pool settings
        config["memory_pools"] = {
            {"enable_pooling", g_memoryPoolingEnabled.load()},
            {"initial_pool_size", g_initialPoolSize.load()},
            {"max_pool_size", g_maxPoolSize.load()},
            {"growth_factor", g_poolGrowthFactor.load()},
            {"allow_expansion", g_allowPoolExpansion.load()}
        };
        
        // Save input settings
        config["input"] = {
            {"mouse_sensitivity", g_mouseSensitivity.load()},
            {"polling_rate_hz", g_pollingRateHz.load()},
            {"raw_input", g_rawInputEnabled.load()}
        };
        
        // Save GUI settings
        config["gui"] = {
            {"update_interval_ms", g_guiUpdateIntervalMs.load()},
            {"show_performance_overlay", g_showPerformanceOverlay.load()},
            {"transparency", g_guiTransparency.load()}
        };
        
        // Save metadata
        config["metadata"] = {
            {"version", "3.1.8"},
            {"saved_at", getCurrentTimeString()},
            {"session_time", g_sessionTime.load()},
            {"frame_count", g_frameCount.load()},
            {"system_state", getSystemStateString()}
        };
        
        // Write to file with pretty formatting
        std::ofstream file(filename);
        if (!file.is_open()) {
            logError("Failed to open configuration file for writing: " + filename);
            return false;
        }
        
        file << std::setw(4) << config << std::endl;
        file.close();
        
        logMessage("✅ Configuration saved successfully");
        return true;
        
    } catch (const json::type_error& e) {
        logError("JSON type error while saving: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        logError("Error saving configuration: " + std::string(e.what()));
        return false;
    }
}

bool validateConfiguration() {
    logMessage("🔍 Validating configuration...");
    
    ConfigValidationResult result;
    result.timestamp = getCurrentTimeString();
    
    // Validate sensitivity
    double sensitivity = g_globalSensitivityMultiplier.load();
    if (sensitivity < GlobalConstants::MIN_SENSITIVITY || sensitivity > GlobalConstants::MAX_SENSITIVITY) {
        result.addError("Sensitivity out of valid range (" + 
                        std::to_string(GlobalConstants::MIN_SENSITIVITY) + "-" + 
                        std::to_string(GlobalConstants::MAX_SENSITIVITY) + ")");
    }
    
    // Validate DPI scale
    double dpi_scale = g_dpiScale.load();
    if (dpi_scale < 0.1 || dpi_scale > 5.0) {
        result.addError("DPI scale out of valid range (0.1-5.0)");
    }
    
    // Validate monitor settings
    int monitor = g_targetMonitor.load();
    if (monitor < 1 || monitor > 4) {
        result.addError("Target monitor out of valid range (1-4)");
    }
    
    int width = g_monitorWidth.load();
    int height = g_monitorHeight.load();
    if (width < 640 || width > 7680 || height < 480 || height > 4320) {
        result.addError("Monitor resolution out of valid range");
    }
    
    // Validate FOV settings
    double aim_fov = g_aimFOV.load();
    if (aim_fov < GlobalConstants::MIN_AIM_FOV || aim_fov > GlobalConstants::MAX_AIM_FOV) {
        result.addError("Aim FOV out of valid range (" + 
                        std::to_string(GlobalConstants::MIN_AIM_FOV) + "-" + 
                        std::to_string(GlobalConstants::MAX_AIM_FOV) + ")");
    }
    
    // Validate smoothing
    double smoothing = g_aimSmoothingFactor.load();
    if (smoothing < GlobalConstants::MIN_SMOOTHING || smoothing > GlobalConstants::MAX_SMOOTHING) {
        result.addError("Smoothing factor out of valid range (" + 
                        std::to_string(GlobalConstants::MIN_SMOOTHING) + "-" + 
                        std::to_string(GlobalConstants::MAX_SMOOTHING) + ")");
    }
    
    // Validate FPS settings
    int max_fps = g_maxFps.load();
    if (max_fps < 30 || max_fps > 500) {
        result.addError("Max FPS out of valid range (30-500)");
    }
    
    // Additional validation checks (warnings)
    if (!g_enableSIMD.load() && !g_enableMultithreading.load()) {
        result.addWarning("Both SIMD and multithreading are disabled - performance may be poor");
    }
    
    if (g_maxFps.load() > 300) {
        result.addWarning("High FPS setting may cause performance issues");
    }
    
    // Store validation result
    g_lastValidationResult = result;
    
    // Log results
    if (result.is_valid) {
        logMessage("✅ Configuration validation passed");
    } else {
        logError("❌ Configuration validation failed with " + std::to_string(result.error_count) + " errors");
        for (const auto& issue : result.issues) {
            if (issue.find("[ERROR]") != std::string::npos) {
                logError("  - " + issue);
            } else {
                logWarning("  - " + issue);
            }
        }
    }
    
    return result.is_valid;
}

void applyDefaultConfiguration() {
    logMessage("🔄 Applying default configuration...");
    
    // Call the global default config function
    applyDefaultConfig();
    
    // Set specific defaults for config system
    g_targetMonitor.store(1);
    g_monitorWidth.store(1920);
    g_monitorHeight.store(1080);
    g_monitorOffsetX.store(0);
    g_monitorOffsetY.store(0);
    g_dpiScale.store(1.0);
    
    g_toggleKey.store(0x70); // F1
    g_holdKey.store(0x71);   // F2
    g_mouseButton.store(1);  // Left click
    g_holdMode.store(false);
    
    g_maxFps.store(240);
    g_performanceMonitoringEnabled.store(true);
    g_profilingEnabled.store(false);
    
    g_guiUpdateIntervalMs.store(50);
    g_showPerformanceOverlay.store(false);
    g_guiTransparency.store(0.9);
    
    // Sync duplicate variables
    g_assistEnabled.store(g_aimAssistEnabled.load());
    g_sensitivity.store(g_globalSensitivityMultiplier.load());
    g_predictionTimeMs.store(g_predictionTime.load());
    g_smoothingFactor.store(g_aimSmoothingFactor.load());
    g_maxAdjustmentDistance.store(g_maxTargetDistance.load());
    
    logMessage("✅ Default configuration applied");
}

// =============================================================================
// CONFIGURATION VALIDATION FUNCTIONS
// =============================================================================

ConfigValidationResult validateConfigurationDetailed() {
    logMessage("🔍 Performing detailed configuration validation...");
    
    ConfigValidationResult result;
    result.timestamp = getCurrentTimeString();
    
    // Validate core settings
    double sensitivity = g_globalSensitivityMultiplier.load();
    if (sensitivity < GlobalConstants::MIN_SENSITIVITY || sensitivity > GlobalConstants::MAX_SENSITIVITY) {
        result.addError("Sensitivity (" + std::to_string(sensitivity) + ") out of range [" +
                        std::to_string(GlobalConstants::MIN_SENSITIVITY) + "-" +
                        std::to_string(GlobalConstants::MAX_SENSITIVITY) + "]");
    }
    
    double smoothing = g_aimSmoothingFactor.load();
    if (smoothing < GlobalConstants::MIN_SMOOTHING || smoothing > GlobalConstants::MAX_SMOOTHING) {
        result.addError("Smoothing (" + std::to_string(smoothing) + ") out of range [" +
                        std::to_string(GlobalConstants::MIN_SMOOTHING) + "-" +
                        std::to_string(GlobalConstants::MAX_SMOOTHING) + "]");
    }
    
    double aim_fov = g_aimFOV.load();
    if (aim_fov < GlobalConstants::MIN_AIM_FOV || aim_fov > GlobalConstants::MAX_AIM_FOV) {
        result.addError("Aim FOV (" + std::to_string(aim_fov) + ") out of range [" +
                        std::to_string(GlobalConstants::MIN_AIM_FOV) + "-" +
                        std::to_string(GlobalConstants::MAX_AIM_FOV) + "]");
    }
    
    double prediction_time = g_predictionTime.load();
    if (prediction_time < GlobalConstants::MIN_PREDICTION_TIME || prediction_time > GlobalConstants::MAX_PREDICTION_TIME) {
        result.addError("Prediction time (" + std::to_string(prediction_time) + ") out of range [" +
                        std::to_string(GlobalConstants::MIN_PREDICTION_TIME) + "-" +
                        std::to_string(GlobalConstants::MAX_PREDICTION_TIME) + "]");
    }
    
    // Validate display settings
    int width = g_monitorWidth.load();
    int height = g_monitorHeight.load();
    if (width < 640 || width > 7680 || height < 480 || height > 4320) {
        result.addError("Monitor resolution (" + std::to_string(width) + "x" + std::to_string(height) + 
                        ") out of supported range");
    }
    
    int target_monitor = g_targetMonitor.load();
    if (target_monitor < 1 || target_monitor > 8) {
        result.addError("Target monitor (" + std::to_string(target_monitor) + ") out of range [1-8]");
    }
    
    double dpi_scale = g_dpiScale.load();
    if (dpi_scale < 0.1 || dpi_scale > 10.0) {
        result.addError("DPI scale (" + std::to_string(dpi_scale) + ") out of range [0.1-10.0]");
    }
    
    // Validate performance settings
    int max_fps = g_maxFps.load();
    if (max_fps < 10 || max_fps > 1000) {
        result.addError("Max FPS (" + std::to_string(max_fps) + ") out of range [10-1000]");
    } else if (max_fps > 300) {
        result.addWarning("High FPS setting (" + std::to_string(max_fps) + ") may impact performance");
    }
    
    // Validate memory pool settings
    int initial_pool = g_initialPoolSize.load();
    int max_pool = g_maxPoolSize.load();
    if (initial_pool > max_pool) {
        result.addError("Initial pool size (" + std::to_string(initial_pool) + 
                        ") cannot be larger than max pool size (" + std::to_string(max_pool) + ")");
    }
    
    // Validate input settings
    int polling_rate = g_pollingRateHz.load();
    if (polling_rate < 100 || polling_rate > 8000) {
        result.addWarning("Polling rate (" + std::to_string(polling_rate) + 
                          ") outside typical range [100-8000] Hz");
    }
    
    double mouse_sens = g_mouseSensitivity.load();
    if (mouse_sens < 0.1 || mouse_sens > 10.0) {
        result.addWarning("Mouse sensitivity (" + std::to_string(mouse_sens) + 
                          ") outside typical range [0.1-10.0]");
    }
    
    // Performance compatibility checks
    if (!g_enableSIMD.load() && !g_enableMultithreading.load()) {
        result.addWarning("Both SIMD and multithreading disabled - expect reduced performance");
    }
    
    if (g_showPerformanceOverlay.load() && max_fps > 240) {
        result.addWarning("Performance overlay with high FPS may cause frame drops");
    }
    
    // Check for logical inconsistencies
    if (g_triggerBotEnabled.load() && !g_aimAssistEnabled.load()) {
        result.addWarning("Trigger bot enabled but aim assist disabled - may not function correctly");
    }
    
    if (g_silentAimEnabled.load() && g_predictionTime.load() < 20.0) {
        result.addWarning("Silent aim with low prediction time may be detectable");
    }
    
    // Log detailed results
    if (result.is_valid) {
        if (result.warning_count == 0) {
            logMessage("✅ Detailed validation passed with no issues");
        } else {
            logMessage("⚠️ Detailed validation passed with " + std::to_string(result.warning_count) + " warnings");
            for (const auto& issue : result.issues) {
                if (issue.find("[WARNING]") != std::string::npos) {
                    logWarning(issue);
                }
            }
        }
    } else {
        logError("❌ Detailed validation failed with " + std::to_string(result.error_count) + " errors and " +
                std::to_string(result.warning_count) + " warnings");
        for (const auto& issue : result.issues) {
            if (issue.find("[ERROR]") != std::string::npos) {
                logError(issue);
            } else {
                logWarning(issue);
            }
        }
    }
    
    g_lastValidationResult = result;
    return result;
}

bool fixConfiguration() {
    logMessage("🔧 Attempting to fix configuration issues...");
    
    bool fixed = false;
    
    // Fix sensitivity if out of range
    double sensitivity = g_globalSensitivityMultiplier.load();
    if (sensitivity < GlobalConstants::MIN_SENSITIVITY) {
        g_globalSensitivityMultiplier.store(GlobalConstants::MIN_SENSITIVITY);
        g_sensitivity.store(GlobalConstants::MIN_SENSITIVITY);
        logMessage("Fixed sensitivity: set to minimum value " + std::to_string(GlobalConstants::MIN_SENSITIVITY));
        fixed = true;
    } else if (sensitivity > GlobalConstants::MAX_SENSITIVITY) {
        g_globalSensitivityMultiplier.store(GlobalConstants::MAX_SENSITIVITY);
        g_sensitivity.store(GlobalConstants::MAX_SENSITIVITY);
        logMessage("Fixed sensitivity: set to maximum value " + std::to_string(GlobalConstants::MAX_SENSITIVITY));
        fixed = true;
    }
    
    // Fix smoothing if out of range
    double smoothing = g_aimSmoothingFactor.load();
    if (smoothing < GlobalConstants::MIN_SMOOTHING) {
        g_aimSmoothingFactor.store(GlobalConstants::MIN_SMOOTHING);
        g_smoothingFactor.store(GlobalConstants::MIN_SMOOTHING);
        logMessage("Fixed smoothing: set to minimum value " + std::to_string(GlobalConstants::MIN_SMOOTHING));
        fixed = true;
    } else if (smoothing > GlobalConstants::MAX_SMOOTHING) {
        g_aimSmoothingFactor.store(GlobalConstants::MAX_SMOOTHING);
        g_smoothingFactor.store(GlobalConstants::MAX_SMOOTHING);
        logMessage("Fixed smoothing: set to maximum value " + std::to_string(GlobalConstants::MAX_SMOOTHING));
        fixed = true;
    }
    
    // Fix FOV if out of range
    double aim_fov = g_aimFOV.load();
    if (aim_fov < GlobalConstants::MIN_AIM_FOV) {
        g_aimFOV.store(GlobalConstants::MIN_AIM_FOV);
        logMessage("Fixed aim FOV: set to minimum value " + std::to_string(GlobalConstants::MIN_AIM_FOV));
        fixed = true;
    } else if (aim_fov > GlobalConstants::MAX_AIM_FOV) {
        g_aimFOV.store(GlobalConstants::MAX_AIM_FOV);
        logMessage("Fixed aim FOV: set to maximum value " + std::to_string(GlobalConstants::MAX_AIM_FOV));
        fixed = true;
    }
    
    // Fix prediction time if out of range
    double prediction_time = g_predictionTime.load();
    if (prediction_time < GlobalConstants::MIN_PREDICTION_TIME) {
        g_predictionTime.store(GlobalConstants::MIN_PREDICTION_TIME);
        g_predictionTimeMs.store(GlobalConstants::MIN_PREDICTION_TIME);
        logMessage("Fixed prediction time: set to minimum value " + std::to_string(GlobalConstants::MIN_PREDICTION_TIME));
        fixed = true;
    } else if (prediction_time > GlobalConstants::MAX_PREDICTION_TIME) {
        g_predictionTime.store(GlobalConstants::MAX_PREDICTION_TIME);
        g_predictionTimeMs.store(GlobalConstants::MAX_PREDICTION_TIME);
        logMessage("Fixed prediction time: set to maximum value " + std::to_string(GlobalConstants::MAX_PREDICTION_TIME));
        fixed = true;
    }
    
    // Fix monitor settings if invalid
    int width = g_monitorWidth.load();
    int height = g_monitorHeight.load();
    if (width < 640 || width > 7680) {
        g_monitorWidth.store(1920);
        logMessage("Fixed monitor width: set to default 1920");
        fixed = true;
    }
    if (height < 480 || height > 4320) {
        g_monitorHeight.store(1080);
        logMessage("Fixed monitor height: set to default 1080");
        fixed = true;
    }
    
    // Fix target monitor if invalid
    int target_monitor = g_targetMonitor.load();
    if (target_monitor < 1 || target_monitor > 8) {
        g_targetMonitor.store(1);
        logMessage("Fixed target monitor: set to default 1");
        fixed = true;
    }
    
    // Fix DPI scale if invalid
    double dpi_scale = g_dpiScale.load();
    if (dpi_scale < 0.1 || dpi_scale > 10.0) {
        g_dpiScale.store(1.0);
        logMessage("Fixed DPI scale: set to default 1.0");
        fixed = true;
    }
    
    // Fix FPS if invalid
    int max_fps = g_maxFps.load();
    if (max_fps < 10 || max_fps > 1000) {
        g_maxFps.store(240);
        logMessage("Fixed max FPS: set to default 240");
        fixed = true;
    }
    
    // Fix memory pool settings if invalid
    int initial_pool = g_initialPoolSize.load();
    int max_pool = g_maxPoolSize.load();
    if (initial_pool > max_pool) {
        g_initialPoolSize.store(32);
        g_maxPoolSize.store(1024);
        logMessage("Fixed memory pool settings: reset to defaults");
        fixed = true;
    }
    
    // Sync duplicate variables after fixes
    g_assistEnabled.store(g_aimAssistEnabled.load());
    g_maxAdjustmentDistance.store(g_maxTargetDistance.load());
    
    if (fixed) {
        logMessage("✅ Configuration issues fixed automatically");
        
        // Re-validate after fixes
        if (validateConfiguration()) {
            logMessage("✅ Configuration is now valid after fixes");
        } else {
            logError("❌ Configuration still has issues after attempted fixes");
        }
    } else {
        logMessage("ℹ️ No configuration issues found to fix");
    }
    
    return fixed;
}

// =============================================================================
// CONFIGURATION EXPORT/IMPORT FUNCTIONS
// =============================================================================

bool exportConfiguration(const std::string& filename) {
    logMessage("📤 Exporting configuration to: " + filename);
    
    try {
        json config;
        
        // Export metadata
        config["metadata"] = {
            {"title", "Tactical Aim Assist Configuration Export"},
            {"version", "3.1.8"},
            {"export_time", getCurrentTimeString()},
            {"system_active", g_systemActive.load()},
            {"debug_mode", g_debugMode.load()},
            {"session_time", g_sessionTime.load()},
            {"frame_count", g_frameCount.load()}
        };
        
        // Export aim assist settings
        config["aim_assist"] = {
            {"enabled", g_aimAssistEnabled.load()},
            {"sensitivity", g_globalSensitivityMultiplier.load()},
            {"smoothing", g_aimSmoothingFactor.load()},
            {"aim_fov", g_aimFOV.load()},
            {"trigger_fov", g_triggerFOV.load()},
            {"prediction_time", g_predictionTime.load()},
            {"trigger_bot", g_triggerBotEnabled.load()},
            {"silent_aim", g_silentAimEnabled.load()},
            {"recoil_compensation", g_recoilCompensationEnabled.load()},
            {"adaptive_sensitivity", g_adaptiveSensitivityEnabled.load()},
            {"max_target_distance", g_maxTargetDistance.load()},
            {"min_confidence_threshold", g_minConfidenceThreshold.load()},
            {"head_confidence_threshold", g_headConfidenceThreshold.load()},
            {"anti_detection_level", g_antiDetectionLevel}
        };
        
        // Export display settings
        config["display"] = {
            {"monitor_width", g_monitorWidth.load()},
            {"monitor_height", g_monitorHeight.load()},
            {"target_monitor", g_targetMonitor.load()},
            {"refresh_rate", g_refreshRate.load()},
            {"dpi_scale", g_dpiScale.load()},
            {"vsync", g_vsyncEnabled.load()},
            {"monitor_offset_x", g_monitorOffsetX.load()},
            {"monitor_offset_y", g_monitorOffsetY.load()}
        };
        
        // Export performance settings
        config["performance"] = {
            {"max_fps", g_maxFps.load()},
            {"enable_simd", g_enableSIMD.load()},
            {"enable_multithreading", g_enableMultithreading.load()},
            {"monitoring_enabled", g_performanceMonitoringEnabled.load()},
            {"profiling_enabled", g_profilingEnabled.load()},
            {"memory_tracking", g_memoryTrackingEnabled.load()},
            {"cpu_monitoring", g_cpuMonitoringEnabled.load()},
            {"fps_tracking", g_fpsTrackingEnabled.load()},
            {"file_output", g_fileOutputEnabled.load()},
            {"simd_optimization", g_simdOptimizationEnabled.load()}
        };
        
        // Export input settings
        config["input"] = {
            {"mouse_sensitivity", g_mouseSensitivity.load()},
            {"polling_rate_hz", g_pollingRateHz.load()},
            {"raw_input", g_rawInputEnabled.load()},
            {"toggle_key", g_toggleKey.load()},
            {"hold_key", g_holdKey.load()},
            {"mouse_button", g_mouseButton.load()},
            {"hold_mode", g_holdMode.load()}
        };
        
        // Export GUI settings
        config["gui"] = {
            {"update_interval_ms", g_guiUpdateIntervalMs.load()},
            {"show_performance_overlay", g_showPerformanceOverlay.load()},
            {"transparency", g_guiTransparency.load()},
            {"show_metrics", g_showMetrics.load()}
        };
        
        // Export memory pool settings
        config["memory_pools"] = {
            {"enable_pooling", g_memoryPoolingEnabled.load()},
            {"initial_pool_size", g_initialPoolSize.load()},
            {"max_pool_size", g_maxPoolSize.load()},
            {"growth_factor", g_poolGrowthFactor.load()},
            {"allow_expansion", g_allowPoolExpansion.load()}
        };
        
        // Export statistics
        config["statistics"] = {
            {"session_time", g_sessionTime.load()},
            {"frame_count", g_frameCount.load()},
            {"total_movements", g_totalMovements.load()},
            {"targets_detected", g_targetsDetected.load()},
            {"targets_acquired", g_targetsAcquired.load()},
            {"accuracy", calculateAccuracy()},
            {"efficiency", calculateEfficiency()},
            {"current_fps", g_currentFPS.load()},
            {"cpu_usage", g_cpuUsage.load()},
            {"memory_usage", g_memoryUsage.load()}
        };
        
        // Write to file with pretty formatting
        std::ofstream file(filename);
        if (!file.is_open()) {
            logError("Failed to create export file: " + filename);
            return false;
        }
        
        file << std::setw(4) << config << std::endl;
        file.close();
        
        logMessage("✅ Configuration exported successfully");
        return true;
        
    } catch (const std::exception& e) {
        logError("Error exporting configuration: " + std::string(e.what()));
        return false;
    }
}

bool importConfiguration(const std::string& filename) {
    logMessage("📥 Importing configuration from: " + filename);
    
    // For safety, backup current configuration first
    std::string backup_file = filename + ".backup." + std::to_string(getCurrentTimeMs());
    if (!exportConfiguration(backup_file)) {
        logWarning("Failed to create backup before import");
    }
    
    // Load the imported configuration
    bool result = loadConfiguration(filename);
    
    if (result) {
        logMessage("✅ Configuration imported successfully");
        
        // Validate the imported configuration
        ConfigValidationResult validation = validateConfigurationDetailed();
        if (!validation.is_valid) {
            logWarning("Imported configuration has validation issues - attempting to fix");
            fixConfiguration();
        }
    } else {
        logError("❌ Failed to import configuration");
    }
    
    return result;
}

// =============================================================================
// CONFIGURATION UTILITY FUNCTIONS
// =============================================================================

std::string getConfigurationSummary() {
    std::ostringstream oss;
    
    oss << "=== CONFIGURATION SUMMARY ===" << std::endl;
    oss << "System Active: " << (g_systemActive.load() ? "YES" : "NO") << std::endl;
    oss << "Debug Mode: " << (g_debugMode.load() ? "YES" : "NO") << std::endl;
    oss << std::endl;
    
    oss << "Aim Assist: " << (g_aimAssistEnabled.load() ? "ENABLED" : "DISABLED") << std::endl;
    oss << "Trigger Bot: " << (g_triggerBotEnabled.load() ? "ENABLED" : "DISABLED") << std::endl;
    oss << "Silent Aim: " << (g_silentAimEnabled.load() ? "ENABLED" : "DISABLED") << std::endl;
    oss << "Sensitivity: " << std::fixed << std::setprecision(2) << g_globalSensitivityMultiplier.load() << std::endl;
    oss << "Smoothing: " << std::fixed << std::setprecision(2) << g_aimSmoothingFactor.load() << std::endl;
    oss << "Aim FOV: " << std::fixed << std::setprecision(1) << g_aimFOV.load() << std::endl;
    oss << std::endl;
    
    oss << "Monitor: " << g_targetMonitor.load() << " (" 
        << g_monitorWidth.load() << "x" << g_monitorHeight.load() << ")" << std::endl;
    oss << "Max FPS: " << g_maxFps.load() << std::endl;
    oss << "SIMD: " << (g_enableSIMD.load() ? "ON" : "OFF") << std::endl;
    oss << "Multithreading: " << (g_enableMultithreading.load() ? "ON" : "OFF") << std::endl;
    oss << std::endl;
    
    oss << "Weapon Profiles: " << getWeaponProfileCount() << std::endl;
    oss << "Active Profile: " << getActiveProfileIndex() << std::endl;
    oss << std::endl;
    
    oss << "Last Validation: " << g_lastValidationResult.timestamp << std::endl;
    oss << "Validation Status: " << (g_lastValidationResult.is_valid ? "VALID" : "INVALID") << std::endl;
    if (!g_lastValidationResult.issues.empty()) {
        oss << "Issues: " << g_lastValidationResult.issues.size() << std::endl;
    }
    
    oss << "===========================";
    
    return oss.str();
}

ConfigValidationResult getLastValidationResult() {
    return g_lastValidationResult;
}

bool resetConfigurationToFactory() {
    logMessage("🏭 Resetting configuration to factory defaults...");
    
    try {
        // Apply factory defaults
        applyDefaultConfiguration();
        
        // Clear any custom settings
        g_antiDetectionLevel = "medium";
        
        // Reset all counters and statistics
        resetGlobalStatistics();
        
        // Validate factory configuration
        bool valid = validateConfiguration();
        
        if (valid) {
            logMessage("✅ Factory reset completed successfully");
        } else {
            logError("❌ Factory reset completed but configuration is invalid");
        }
        
        return valid;
        
    } catch (const std::exception& e) {
        logError("Error during factory reset: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> getAvailableConfigurationFiles() {
    std::vector<std::string> files;
    
    // In a real implementation, this would scan the config directory
    // For now, return common configuration file names
    files.push_back(GlobalConstants::MAIN_CONFIG_FILE);
    files.push_back(GlobalConstants::PROFILES_FILE);
    files.push_back(GlobalConstants::KEYBINDINGS_FILE);
    files.push_back("./config/user_config.json");
    files.push_back("./config/backup_config.json");
    files.push_back("./config/default_config.json");
    files.push_back("./config/export_config.json");
    files.push_back("./config/template_config.json");
    
    return files;
}

bool createConfigurationTemplate(const std::string& filename) {
    logMessage("📝 Creating configuration template: " + filename);
    
    try {
        json config;
        
        // Template header
        config["_template_info"] = {
            {"title", "Tactical Aim Assist Configuration Template"},
            {"version", "3.1.8"},
            {"description", "Copy this file and modify values as needed"},
            {"note", "All values shown are defaults"},
            {"created", getCurrentTimeString()}
        };
        
        config["general"] = {
            {"assist_enabled", false},
            {"debug_mode", false},
            {"adaptive_smoothing", true},
            {"audio_alerts", true},
            {"verbose_logging", false},
            {"monitor_offset_x", 0},
            {"monitor_offset_y", 0},
            {"dpi_scale", 1.0}
        };
        
        config["display"] = {
            {"width", 1920},
            {"height", 1080},
            {"target_monitor", 1},
            {"refresh_rate", 60},
            {"dpi_scale", 1.0},
            {"vsync", true}
        };
        
        config["aim"] = {
            {"sensitivity", 1.0},
            {"smoothing_factor", 0.75},
            {"aim_fov", 100.0},
            {"trigger_fov", 50.0},
            {"prediction_time_ms", 50.0},
            {"trigger_bot", false},
            {"silent_aim", false},
            {"max_adjustment_distance", 500.0},
            {"anti_detection_level", "medium"}
        };
        
        config["performance"] = {
            {"max_fps", 240},
            {"enable_simd", true},
            {"enable_multithreading", true},
            {"enable_monitoring", true},
            {"enable_profiling", false},
            {"track_memory", true},
            {"monitor_cpu", true},
            {"track_fps", true}
        };
        
        config["hotkeys"] = {
            {"toggle_key", "0x70 // F1"},
            {"hold_key", "0x71 // F2"},
            {"mouse_button", "1 // Left click"},
            {"hold_mode", false}
        };
        
        config["input"] = {
            {"mouse_sensitivity", 1.0},
            {"polling_rate_hz", 1000},
            {"raw_input", true}
        };
        
        config["gui"] = {
            {"update_interval_ms", 50},
            {"show_performance_overlay", false},
            {"transparency", 0.9}
        };
        
        config["memory_pools"] = {
            {"enable_pooling", true},
            {"initial_pool_size", 32},
            {"max_pool_size", 1024},
            {"growth_factor", 2},
            {"allow_expansion", true}
        };
        
        // Write to file with pretty formatting
        std::ofstream file(filename);
        if (!file.is_open()) {
            logError("Failed to create template file: " + filename);
            return false;
        }
        
        file << std::setw(4) << config << std::endl;
        file.close();
        
        logMessage("✅ Configuration template created successfully");
        return true;
        
    } catch (const std::exception& e) {
        logError("Error creating configuration template: " + std::string(e.what()));
        return false;
    }
}

bool loadConfigurationSection(const std::string& filename, const std::string& section) {
    logMessage("📖 Loading configuration section '" + section + "' from: " + filename);
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            logError("Cannot open configuration file: " + filename);
            return false;
        }
        
        json config;
        file >> config;
        
        if (!config.contains(section)) {
            logWarning("Section '" + section + "' not found in configuration file");
            return false;
        }
        
        auto sec = config[section];
        
        // Load specific section based on name
        if (section == "aim") {
            if (sec.contains("sensitivity")) g_globalSensitivityMultiplier.store(sec["sensitivity"]);
            if (sec.contains("smoothing_factor")) g_aimSmoothingFactor.store(sec["smoothing_factor"]);
            if (sec.contains("aim_fov")) g_aimFOV.store(sec["aim_fov"]);
            if (sec.contains("trigger_fov")) g_triggerFOV.store(sec["trigger_fov"]);
            if (sec.contains("prediction_time_ms")) g_predictionTime.store(sec["prediction_time_ms"]);
        } else if (section == "display") {
            if (sec.contains("width")) g_monitorWidth.store(sec["width"]);
            if (sec.contains("height")) g_monitorHeight.store(sec["height"]);
            if (sec.contains("target_monitor")) g_targetMonitor.store(sec["target_monitor"]);
            if (sec.contains("refresh_rate")) g_refreshRate.store(sec["refresh_rate"]);
        } else if (section == "performance") {
            if (sec.contains("max_fps")) g_maxFps.store(sec["max_fps"]);
            if (sec.contains("enable_simd")) g_enableSIMD.store(sec["enable_simd"]);
            if (sec.contains("enable_multithreading")) g_enableMultithreading.store(sec["enable_multithreading"]);
        }
        // Add more sections as needed
        
        logMessage("✅ Section '" + section + "' loaded successfully");
        return true;
        
    } catch (const std::exception& e) {
        logError("Error loading configuration section: " + std::string(e.what()));
        return false;
    }
}

bool saveConfigurationSection(const std::string& filename, const std::string& section) {
    logMessage("💾 Saving configuration section '" + section + "' to: " + filename);
    
    try {
        json config;
        
        // Load existing config if file exists
        std::ifstream infile(filename);
        if (infile.is_open()) {
            infile >> config;
            infile.close();
        }
        
        // Update specific section
        if (section == "aim") {
            config["aim"] = {
                {"sensitivity", g_globalSensitivityMultiplier.load()},
                {"smoothing_factor", g_aimSmoothingFactor.load()},
                {"aim_fov", g_aimFOV.load()},
                {"trigger_fov", g_triggerFOV.load()},
                {"prediction_time_ms", g_predictionTime.load()},
                {"trigger_bot", g_triggerBotEnabled.load()},
                {"silent_aim", g_silentAimEnabled.load()},
                {"max_adjustment_distance", g_maxTargetDistance.load()},
                {"anti_detection_level", g_antiDetectionLevel}
            };
        } else if (section == "display") {
            config["display"] = {
                {"width", g_monitorWidth.load()},
                {"height", g_monitorHeight.load()},
                {"target_monitor", g_targetMonitor.load()},
                {"refresh_rate", g_refreshRate.load()},
                {"vsync", g_vsyncEnabled.load()}
            };
        } else if (section == "performance") {
            config["performance"] = {
                {"max_fps", g_maxFps.load()},
                {"enable_simd", g_enableSIMD.load()},
                {"enable_multithreading", g_enableMultithreading.load()},
                {"enable_monitoring", g_performanceMonitoringEnabled.load()},
                {"enable_profiling", g_profilingEnabled.load()}
            };
        }
        // Add more sections as needed
        
        // Save updated config
        std::ofstream outfile(filename);
        if (!outfile.is_open()) {
            logError("Cannot create configuration file: " + filename);
            return false;
        }
        
        outfile << std::setw(4) << config << std::endl;
        outfile.close();
        
        logMessage("✅ Section '" + section + "' saved successfully");
        return true;
        
    } catch (const std::exception& e) {
        logError("Error saving configuration section: " + std::string(e.what()));
        return false;
    }
}