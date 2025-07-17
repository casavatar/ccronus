// gui.h - REFACTORED
// description: Clean GUI header without duplicated definitions
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - Refactored
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include "common_defines.h"

// =============================================================================
// GUI CONTROL HANDLES
// =============================================================================
extern HWND g_hStatus;
extern HWND g_hProfileLabel;
extern HWND g_hToggleButton;
extern HWND g_hAnalyticsLabel;
extern HWND g_hAudioAlertLabel;
extern HWND g_hMovementStatusLabel;
extern HWND g_hProfileComboBox;

// =============================================================================
// GUI CORE FUNCTIONS
// =============================================================================
bool initializeGUI();
void cleanupGUI();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// =============================================================================
// GUI UPDATE FUNCTIONS - Thread-safe
// =============================================================================
void updateGUIStatus(const std::string& status);
void updateProfileDisplay(const std::string& profileName);
void updateAnalyticsDisplay(const std::string& analytics);
void updateAudioAlertDisplay(const std::string& alert);
void updateMovementStatusDisplay(const std::string& status);

// =============================================================================
// GUI UTILITY FUNCTIONS
// =============================================================================
void populateProfileComboBox();
void refreshGUIState();
void toggleAssistState();

// =============================================================================
// MESSAGE POSTING FUNCTIONS - For thread-safe GUI updates
// =============================================================================
void postUpdateAnalytics();
void postUpdateAudioAlert(const std::string& alert);
void postUpdateMovementStatus(const std::string& status);

// =============================================================================
// GUI EVENT HANDLERS
// =============================================================================
void onProfileSelectionChanged();
void onToggleButtonClicked();
void onWindowClose();