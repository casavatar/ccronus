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

// --- Weapon Profile Struct ---
class WeaponProfile {
public:
    std::string name;
    std::string fireMode; // "Rapid", "Controlled", "Single"
    std::vector<int> recoilPattern;
    int fireDelayBase;
    int fireDelayVariance;
    double pid_kp, pid_ki, pid_kd;
    double smoothingFactor;
};

void switchProfile(int profileIndex);