// description: This header file defines the WeaponProfile class and a function to switch profiles.
// It includes necessary libraries and provides a structure for weapon profiles used in the Tactical Aim Assist project.
// 
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist

#pragma once

#include <string>
#include <vector>
#include <map> // Added for pid_states

// NEW: Structure to hold PID parameters
struct PIDParams {
    double kp, ki, kd;
};

class WeaponProfile {
public:
    std::string name;
    std::string fireMode;
    std::vector<int> recoilPattern;
    int fireDelayBase;
    int fireDelayVariance;
    double smoothingFactor; // Smoothing factor for the profile
    double prediction_aggressiveness; // New field for prediction aggressiveness
    std::map<std::string, PIDParams> pid_states;
};

void switchProfile(int profileIndex);