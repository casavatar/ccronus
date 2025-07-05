// description: This header file declares functions for managing the GUI thread and updating labels in a C++ project.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.0.0
// date: 2025-06-25
// project: Tactical Aim Assist
//

#pragma once // Tactical Aim Assist GUI Header
#include <atomic> // Added for std::atomic
#include <string> // Added for std::string

void enhancedGuiThread(); // Function to run the enhanced GUI thread
void updateProfileLabel(); // Function to update the profile label in the GUI
void updateAnalyticsLabel(); // Function to update the analytics label in the GUI
void updateAudioAlertLabel(const std::string& alert_text); // New function for audio alerts
void updateMovementStatusLabel(const std::string& status_text); // New movement status label function

extern std::atomic<bool> guiReady; // Atomic variable to indicate if the GUI is ready