// gui.h - REFACTORED
// description: Clean GUI header without duplicated definitions
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 4.0.0 - Enhanced with weapon profiles, multi-monitor support, and movement system
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
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

// New weapon profile UI controls
extern HWND g_hWeaponProfileFrame;
extern HWND g_hActiveProfileLabel;
extern HWND g_hProfileEnhancementsFrame;
extern HWND g_hRecoilControlToggle;
extern HWND g_hAimAssistToggle;
extern HWND g_hSpreadControlToggle;
extern HWND g_hProfileStatsLabel;

// Multi-monitor awareness controls
extern HWND g_hMonitorInfoFrame;
extern HWND g_hCurrentMonitorLabel;
extern HWND g_hMonitorCountLabel;
extern HWND g_hMonitorResolutionLabel;
extern HWND g_hMonitorSwitchButton;

// Movement system controls
extern HWND g_hMovementFrame;
extern HWND g_hSlideKeyLabel;
extern HWND g_hMovementStateLabel;
extern HWND g_hMovementToggleButton;

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

// New weapon profile update functions
void updateWeaponProfileDisplay(const std::string& profileName);
void updateProfileEnhancements(bool recoilControl, bool aimAssist, bool spreadControl);
void updateProfileStats(const std::string& stats);

// Multi-monitor update functions
void updateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height);
void updateCurrentMonitorDisplay(int monitorIndex, int width, int height);

// Movement system update functions
void updateMovementStateDisplay(const std::string& state);
void updateSlideKeyStatus(bool isActive);

// =============================================================================
// GUI UTILITY FUNCTIONS
// =============================================================================
void populateProfileComboBox();
void refreshGUIState();
void toggleAssistState();

// New utility functions
void createWeaponProfileControls(HWND hwndParent);
void createMultiMonitorControls(HWND hwndParent);
void createMovementControls(HWND hwndParent);
void updateAllGUIElements();

// =============================================================================
// MESSAGE POSTING FUNCTIONS - For thread-safe GUI updates
// =============================================================================
void postUpdateAnalytics();
void postUpdateAudioAlert(const std::string& alert);
void postUpdateMovementStatus(const std::string& status);
void postUpdateWeaponProfile(const std::string& profileName);
void postUpdateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height);

// =============================================================================
// GUI EVENT HANDLERS
// =============================================================================
void onProfileSelectionChanged();
void onToggleButtonClicked();
void onWindowClose();

// New event handlers
void onWeaponProfileChanged();
void onEnhancementToggleClicked(int enhancementType);
void onMonitorSwitchClicked();
void onMovementToggleClicked();

// =============================================================================
// MULTI-MONITOR SUPPORT
// =============================================================================
struct MonitorInfo {
    int index;
    int x, y, width, height;
    bool isPrimary;
    std::string name;
};

std::vector<MonitorInfo> getConnectedMonitors();
int getCurrentMonitorIndex();
bool switchToMonitor(int monitorIndex);
MonitorInfo getMonitorInfo(int monitorIndex);

// =============================================================================
// WEAPON PROFILE ENHANCEMENT TYPES
// =============================================================================
enum class EnhancementType {
    RecoilControl = 0,
    AimAssist = 1,
    SpreadControl = 2
};