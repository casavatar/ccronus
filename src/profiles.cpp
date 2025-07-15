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
#include <sstream> // For string stream operations
#include <iomanip> // For std::setprecision

// Include ComboBox definitions
#include <CommCtrl.h>

// These state variables are defined in movements.cpp and globals.cpp
extern std::atomic<bool> isRapidFiring; // Extern for the new Rapid firing mode
extern std::atomic<bool> isControlledAutoFiring; // Extern for the new Controlled automatic fire mode
extern std::atomic<bool> isTacticalFiring; // Extern for the new Tactical fire mode
extern std::atomic<bool> isAutomaticFiring; // Extern for the new mode
extern HWND hProfileComboBox; // Extern for the profile ComboBox handle

void switchProfile(int profileIndex) {
    if (profileIndex < 0 || static_cast<size_t>(profileIndex) >= g_weaponProfiles.size()) {
        logMessage("ERROR: Profile index out of range: " + std::to_string(profileIndex));
        return;
    }
    
    int previousProfile = g_activeProfileIndex.load();
    g_activeProfileIndex = profileIndex;
    
    const auto& profile = g_weaponProfiles[profileIndex];

    // FIX: Update PID controllers using the new pid_states structure.
    // When switching profiles, we'll default to the 'stationary' state's PID values.
    if (g_pidX && g_pidY && profile.pid_states.count("stationary")) {
        const auto& new_pid_params = profile.pid_states.at("stationary");
        g_pidX->updateParams(new_pid_params.kp, new_pid_params.ki, new_pid_params.kd);
        g_pidX->reset();
        
        g_pidY->updateParams(new_pid_params.kp, new_pid_params.ki, new_pid_params.kd);
        g_pidY->reset();
    }
    
    if (g_smoothingSystem) {
        double smooth = profile.smoothingFactor;
        g_smoothingSystem->updateProfile(smooth, smooth * 0.8, smooth * 1.1);
    }
    
    // Stop any active firing loops.
    isRapidFiring = false;
    isControlledAutoFiring = false;
    isTacticalFiring = false;
    
    std::ostringstream profileInfo;
    profileInfo << "Profile Switch: [" << previousProfile + 1 << "] " 
               << (static_cast<size_t>(previousProfile) < g_weaponProfiles.size() ? g_weaponProfiles[previousProfile].name : "Unknown") 
               << " -> [" << profileIndex + 1 << "] " << profile.name;
    logMessage(profileInfo.str());
    
    std::ostringstream profileDetails;
    profileDetails << profile.name << " | "
                  << "Mode: " << profile.fireMode << " | "
                  << "Smooth: " << std::fixed << std::setprecision(2) << profile.smoothingFactor;
    logMessage(profileDetails.str());
    
    if (guiReady.load() && hProfileComboBox) {
        SendMessage(hProfileComboBox, CB_SETCURSEL, (WPARAM)profileIndex, (LPARAM)0);
    }
}