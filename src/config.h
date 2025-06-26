// Project structure: The project is organized with a src directory for source files and a config.h header file for configuration settings.
// The src directory contains various source files that implement the functionality of the Tactical Aim Assist project.
// The config.h file defines keybindings and provides a function to load configuration settings from a file.
// This structure allows for modular development and easy maintenance of the codebase.
//
// Keybindings configuration
// The config.h file defines a structure for keybindings used in the Tactical Aim Assist project.
// It includes various key actions such as exit, smart sprint, predictive slide, dive back, corner bounce, cutback, dropshot, slide cancel, dive directional, omnidirectional slide, and movement test.
// Each action has a virtual key code (vk) and a modifier key (mod) associated with it.
// The g_keybindings global variable holds the current keybindings configuration.
// The loadConfiguration function is used to load keybindings from a specified configuration file.
// The configuration file is expected to be in a specific format, allowing users to customize their keybindings for the Tactical Aim Assist project.
// The keybindings structure is designed to be easily extensible, allowing for future additions or modifications to the key actions.
// The project uses the C++17 standard and is developed with a focus on performance and user customization.
// 
// This file is distributed under the GNU General Public License v3.0.
// You can redistribute it and/or modify it under the terms of the GPL v3.0.
// See the LICENSE file for more details.
// src/config.h
//
// description: This header file defines keybindings for the Tactical Aim Assist project.
// It includes a structure for keybindings and a function to load configuration settings from a file.   
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
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
};

extern Keybindings g_keybindings;

bool loadConfiguration(const std::string& filename);