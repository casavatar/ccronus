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

// --- Custom Window Messages and Control IDs ---
#define ID_LISTBOX 1001
#define ID_STATUS_LABEL 1002
#define ID_TOGGLE_BUTTON 1004
#define ID_ANALYTICS_LABEL 1005
#define ID_AUDIO_ALERT_LABEL 1006
#define ID_MOVEMENT_STATUS_LABEL 1007
#define ID_PROFILE_COMBOBOX 1008 // NEW: ID for our profile selection ComboBox

// --- New Custom Window Messages for thread-safe GUI updates ---
#define WM_UPDATE_ANALYTICS (WM_USER + 2) // Custom message for analytics updates
#define WM_UPDATE_AUDIO_ALERT (WM_USER + 3) // Custom message for audio alert updates
#define WM_UPDATE_MOVEMENT_STATUS (WM_USER + 4) // Custom message for movement status updates

void enhancedGuiThread(); // Function to start the GUI thread
void updateProfileLabel(); // Function to update the profile label in the GUI
void populateProfileComboBox(); // Function to populate the profile ComboBox with available profiles

// These functions will now post messages instead of directly updating the GUI
void postUpdateAnalytics(); // Function to post an update for analytics
void postUpdateAudioAlert(const std::string& alert_text); // Function to post an audio alert update
void postUpdateMovementStatus(const std::string& status_text); // Function to post a movement status update

extern std::atomic<bool> guiReady; // Flag to indicate if the GUI is ready