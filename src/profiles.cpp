// description: This code implements the functionality to switch between different weapon profiles.
// It updates PID parameters, smoothing factors, and ensures firing modes are stopped on profile change.
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.1.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "profiles.h"
#include "globals.h"
#include "gui.h" // For updateProfileLabel
#include "systems.h" // For system classes
#include <sstream>
#include <iomanip>

// These state variables are defined in movements.cpp and globals.cpp
extern std::atomic<bool> isRapidFiring;
extern std::atomic<bool> isControlledAutoFiring;
extern std::atomic<bool> isTacticalFiring; // Extern for the new Tactical fire mode

// Function to switch weapon profiles
void switchProfile(int profileIndex) {
    if (profileIndex < 0 || static_cast<size_t>(profileIndex) >= g_weaponProfiles.size()) {
        logMessage("ERROR: Profile index out of range: " + std::to_string(profileIndex));
        return;
    }
    
    int previousProfile = g_activeProfileIndex.load();
    g_activeProfileIndex = profileIndex;
    
    const auto& profile = g_weaponProfiles[profileIndex];

    if (g_pidX && g_pidY) {
        g_pidX->updateParams(profile.pid_kp, profile.pid_ki, profile.pid_kd);
        g_pidX->reset();
        
        g_pidY->updateParams(profile.pid_kp, profile.pid_ki, profile.pid_kd);
        g_pidY->reset();
    }
    
    if (g_smoothingSystem) {
        double smooth = profile.smoothingFactor;
        g_smoothingSystem->updateProfile(smooth, smooth * 0.8, smooth * 1.1);
    }
    
    // Stop any active firing loops. This is a "soft" stop.
    // The loops themselves check this atomic bool.
    isRapidFiring = false;
    isControlledAutoFiring = false;
    isTacticalFiring = false; // Stop tactical firing when switching profiles
    
    std::ostringstream profileInfo;
    profileInfo << "Profile Switch: [" << previousProfile + 1 << "] " 
               << (static_cast<size_t>(previousProfile) < g_weaponProfiles.size() ? g_weaponProfiles[previousProfile].name : "Unknown") 
               << " -> [" << profileIndex + 1 << "] " << profile.name;
    logMessage(profileInfo.str());
    
    std::ostringstream profileDetails;
    profileDetails << profile.name << " | "
                  << "Mode: " << profile.fireMode << " | "
                  << "Delay: " << profile.fireDelayBase << "ms | "
                  << "Smooth: " << std::fixed << std::setprecision(2) << profile.smoothingFactor;
    logMessage(profileDetails.str());
    
    updateProfileLabel();
}