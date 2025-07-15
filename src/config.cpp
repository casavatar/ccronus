// description: This code is part of a C++ project that loads configuration settings from a JSON file, including keybindings and weapon profiles.
// It uses the nlohmann/json library for JSON parsing and Windows API for message boxes, with support for multi-monitor setups.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 2.8.2
// date: 2025-06-26
// project: Tactical Aim Assist

#include "config.h"
#include "globals.h"
#include "profiles.h"
#include "json.hpp" // nlohmann/json library
#include <fstream>
#include <windows.h>
#include <iostream>

using json = nlohmann::json;

Keybindings g_keybindings;

// --- New Monitor Detection Logic ---
struct MonitorInfo {
    RECT rect;
};

// Callback function for EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, &mi)) {
        monitors->push_back({mi.rcMonitor});
    }
    return TRUE;
}


// Helper function to parse keybindings like "Alt+A" or "Ctrl+Mouse4"
void parseKeybinding(const std::string& keyStr, int& vk, int& mod) {
    vk = 0;
    mod = 0; // 0 means no modifier
    std::string key = keyStr;
    size_t pos = 0;

    if ((pos = key.find("Ctrl+")) != std::string::npos) {
        mod = VK_CONTROL;
        key.erase(pos, 5);
    } else if ((pos = key.find("Alt+")) != std::string::npos) {
        mod = VK_LMENU;
        key.erase(pos, 4);
    } else if ((pos = key.find("Shift+")) != std::string::npos) {
        mod = VK_SHIFT;
        key.erase(pos, 6);
    }

    if (key.length() == 1 && isalnum(key[0])) {
        vk = toupper(key[0]);
    } else if (key == "Mouse4") {
        vk = VK_XBUTTON1;
    } else if (key == "Mouse5") {
        vk = VK_XBUTTON2;
    } else if (key == "Escape") {
        vk = VK_ESCAPE;
    } else if (key == "Space") {
        vk = VK_SPACE;
    }
}

bool loadConfiguration(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        MessageBoxW(NULL, L"Could not open config.json!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    try {
        json data = json::parse(f);

        // --- Screen Configuration with Multi-Monitor Support ---
        int target_monitor_index = data["settings"].value("target_monitor", 1);

        std::vector<MonitorInfo> monitors;
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

        if (target_monitor_index > 0 && static_cast<size_t>(target_monitor_index) <= monitors.size()) {
            logMessage("Monitors detected: " + std::to_string(monitors.size()) + ". Using monitor " + std::to_string(target_monitor_index));
            const auto& target_monitor = monitors[target_monitor_index - 1];
            
            // FIX: Corrected member access to use the 'rect' member of the MonitorInfo struct.
            g_monitorOffsetX = target_monitor.rect.left;
            g_monitorOffsetY = target_monitor.rect.top;
            MONITOR_WIDTH = target_monitor.rect.right - target_monitor.rect.left;
            MONITOR_HEIGHT = target_monitor.rect.bottom - target_monitor.rect.top;
        } else {
            logMessage("Failed to find target monitor " + std::to_string(target_monitor_index) + ". Falling back to primary monitor/config values.");
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY), &mi);
            g_monitorOffsetX = mi.rcMonitor.left;
            g_monitorOffsetY = mi.rcMonitor.top;
            MONITOR_WIDTH = data["settings"].value("monitor_width", 1920);
            MONITOR_HEIGHT = data["settings"].value("monitor_height", 1080);
        }

        SCREEN_CENTER_X = g_monitorOffsetX + (MONITOR_WIDTH / 2);
        SCREEN_CENTER_Y = g_monitorOffsetY + (MONITOR_HEIGHT / 2);


        // Load Keybindings
        parseKeybinding(data["keybindings"]["exit"], g_keybindings.exit_vk, g_keybindings.exit_mod);
        parseKeybinding(data["keybindings"]["smart_sprint_left"], g_keybindings.smart_sprint_left_vk, g_keybindings.smart_sprint_left_mod);
        parseKeybinding(data["keybindings"]["smart_sprint_right"], g_keybindings.smart_sprint_right_vk, g_keybindings.smart_sprint_right_mod);
        parseKeybinding(data["keybindings"]["predictive_slide"], g_keybindings.predictive_slide_vk, g_keybindings.predictive_slide_mod);
        parseKeybinding(data["keybindings"]["contextual_movement_assist"], g_keybindings.contextual_movement_assist_vk, g_keybindings.contextual_movement_assist_mod);
        parseKeybinding(data["keybindings"]["dive_back"], g_keybindings.dive_back_vk, g_keybindings.dive_back_mod);
        parseKeybinding(data["keybindings"]["corner_bounce_left"], g_keybindings.corner_bounce_left_vk, g_keybindings.corner_bounce_left_mod);
        parseKeybinding(data["keybindings"]["corner_bounce_right"], g_keybindings.corner_bounce_right_vk, g_keybindings.corner_bounce_right_mod);
        parseKeybinding(data["keybindings"]["cutback"], g_keybindings.cutback_vk, g_keybindings.cutback_mod);
        parseKeybinding(data["keybindings"]["dropshot_supine_slide"], g_keybindings.dropshot_supine_slide_vk, g_keybindings.dropshot_supine_slide_mod);
        parseKeybinding(data["keybindings"]["slide_cancel_directional"], g_keybindings.slide_cancel_directional_vk, g_keybindings.slide_cancel_directional_mod);
        parseKeybinding(data["keybindings"]["dive_directional_intelligent"], g_keybindings.dive_directional_intelligent_vk, g_keybindings.dive_directional_intelligent_mod);
        parseKeybinding(data["keybindings"]["omnidirectional_slide"], g_keybindings.omnidirectional_slide_vk, g_keybindings.omnidirectional_slide_mod);
        parseKeybinding(data["keybindings"]["movement_test"], g_keybindings.movement_test_vk, g_keybindings.movement_test_mod);
        
        // Load weapon profiles
        g_weaponProfiles.clear();
        for (const auto& profile_json : data["weapon_profiles"]) {
            WeaponProfile p;
            p.name = profile_json["name"];
            p.fireMode = profile_json["fire_mode"];
            p.recoilPattern = profile_json["recoil_pattern"].get<std::vector<int>>();
            p.fireDelayBase = profile_json["fire_delay_base"];
            p.fireDelayVariance = profile_json["fire_delay_variance"];
            p.smoothingFactor = profile_json["smoothing_factor"];
            p.prediction_aggressiveness = profile_json.value("prediction_aggressiveness", 0.7); // NEW

            if (profile_json.contains("pid_states")) {
                for (auto& [state, params] : profile_json["pid_states"].items()) {
                    p.pid_states[state] = {params["kp"], params["ki"], params["kd"]};
                }
            }
            g_weaponProfiles.push_back(p);
        }

    } catch (json::parse_error& e) {
        std::string err_msg = "JSON parse error in " + filename + ":\n" + e.what();
        MessageBoxA(NULL, err_msg.c_str(), "Config Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}