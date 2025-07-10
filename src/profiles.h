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
    // DEPRECATED: pid_kp, pid_ki, pid_kd are replaced by pid_states
    // We keep them for now to avoid breaking existing profiles, but new logic will ignore them.
    double pid_kp, pid_ki, pid_kd; 
    std::map<std::string, PIDParams> pid_states; // NEW: Holds PID settings for different movement states
    double smoothingFactor;
};

void switchProfile(int profileIndex);