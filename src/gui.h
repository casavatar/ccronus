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
#include <string>   // Added for std::string

// --- New Custom Window Messages for thread-safe GUI updates ---
#define WM_UPDATE_ANALYTICS (WM_USER + 2) // Custom message for analytics updates
#define WM_UPDATE_AUDIO_ALERT (WM_USER + 3) // Custom message for audio alert updates
#define WM_UPDATE_MOVEMENT_STATUS (WM_USER + 4) // Custom message for movement status updates

void enhancedGuiThread(); // Function to start the GUI thread
void updateProfileLabel(); // Function to update the profile label in the GUI
// These functions will now post messages instead of directly updating the GUI
void postUpdateAnalytics(); // Function to post an update for analytics
void postUpdateAudioAlert(const std::string& alert_text); // Function to post an audio alert update
void postUpdateMovementStatus(const std::string& status_text); // Function to post a movement status update

extern std::atomic<bool> guiReady; // Flag to indicate if the GUI is ready