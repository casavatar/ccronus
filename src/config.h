// description: This header file defines keybindings for the Tactical Aim Assist project.
// It includes a structure for keybindings and a function to load configuration settings from a file.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.3.0
// date: 2025-06-26
// project: Tactical Aim Assist

#pragma once
#include <string>

// Keybindings structure
struct Keybindings {
    int exit_vk;
    int exit_mod;
    int smart_sprint_left_vk;
    int smart_sprint_left_mod;
    int smart_sprint_right_vk;
    int smart_sprint_right_mod;
    int predictive_slide_vk;
    int predictive_slide_mod;
    int dive_back_vk;
    int dive_back_mod;
    int corner_bounce_left_vk;
    int corner_bounce_left_mod;
    int corner_bounce_right_vk;
    int corner_bounce_right_mod;
    int cutback_vk;
    int cutback_mod;
    int dropshot_supine_slide_vk;
    int dropshot_supine_slide_mod;
    int slide_cancel_directional_vk;
    int slide_cancel_directional_mod;
    int dive_directional_intelligent_vk;
    int dive_directional_intelligent_mod;
    int omnidirectional_slide_vk;
    int omnidirectional_slide_mod;
    int movement_test_vk;
    int movement_test_mod;
    int contextual_movement_assist_vk;
    int contextual_movement_assist_mod;
};

extern Keybindings g_keybindings;

bool loadConfiguration(const std::string& filename);