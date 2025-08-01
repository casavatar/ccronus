// gui.cpp - REFACTORED AND UPDATED VERSION v5.0.0
// description: GUI implementation using global functions as defined in the header.
//              The GUI communicates through the StateManager and EventSystem.
//              Enhanced with weapon profiles, multi-monitor support, and movement system.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 5.0.0 - Enhanced with new gameplay and system features
// date: 2025-07-20
// project: Tactical Aim Assist

#include "gui.h"
#include "state_manager.h"
#include "event_system.h"
#include "common_defines.h"
#include "globals.h" // For logging functions

#include <commctrl.h> // For ComboBox control
#include <sstream>
#include <iomanip>

// =============================================================================
// GLOBAL GUI HANDLES
// =============================================================================

HWND g_hStatus = NULL;
HWND g_hProfileLabel = NULL;
HWND g_hToggleButton = NULL;
HWND g_hAnalyticsLabel = NULL;
HWND g_hAudioAlertLabel = NULL;
HWND g_hMovementStatusLabel = NULL;
HWND g_hProfileComboBox = NULL;

// New weapon profile UI controls
HWND g_hWeaponProfileFrame = NULL;
HWND g_hActiveProfileLabel = NULL;
HWND g_hProfileEnhancementsFrame = NULL;
HWND g_hRecoilControlToggle = NULL;
HWND g_hAimAssistToggle = NULL;
HWND g_hSpreadControlToggle = NULL;
HWND g_hProfileStatsLabel = NULL;

// Multi-monitor awareness controls
HWND g_hMonitorInfoFrame = NULL;
HWND g_hCurrentMonitorLabel = NULL;
HWND g_hMonitorCountLabel = NULL;
HWND g_hMonitorResolutionLabel = NULL;
HWND g_hMonitorSwitchButton = NULL;

// Movement system controls
HWND g_hMovementFrame = NULL;
HWND g_hSlideKeyLabel = NULL;
HWND g_hMovementStateLabel = NULL;
HWND g_hMovementToggleButton = NULL;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

// Note: stringToWstring function removed as we're using ANSI Windows API functions throughout

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

void createControls(HWND hwndParent);
void createWeaponProfileControls(HWND hwndParent);
void createMultiMonitorControls(HWND hwndParent);
void createMovementControls(HWND hwndParent);

// =============================================================================
// GUI CORE FUNCTIONS
// =============================================================================

bool initializeGUI() {
    const char CLASS_NAME[] = "TacticalAimAssistGUIClass";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        logError("GUI Error: Failed to register window class.");
        return false;
    }

    // Create the main window with increased size for new controls
    HWND hMainWnd = CreateWindowExA(
        0, CLASS_NAME, "Tactical Aim Assist v5.0",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        600, 500, NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hMainWnd) {
        logError("GUI Error: Failed to create main window.");
        return false;
    }

    // Create controls
    createControls(hMainWnd);
    createWeaponProfileControls(hMainWnd);
    createMultiMonitorControls(hMainWnd);
    createMovementControls(hMainWnd);
    
    // Initial state refresh
    refreshGUIState();
    
    // Show the window
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);
    
    logMessage("GUI initialized successfully");
    return true;
}

void cleanupGUI() {
    // Clean up window handles
    g_hStatus = NULL;
    g_hProfileLabel = NULL;
    g_hToggleButton = NULL;
    g_hAnalyticsLabel = NULL;
    g_hAudioAlertLabel = NULL;
    g_hMovementStatusLabel = NULL;
    g_hProfileComboBox = NULL;
    
    // Clean up new controls
    g_hWeaponProfileFrame = NULL;
    g_hActiveProfileLabel = NULL;
    g_hProfileEnhancementsFrame = NULL;
    g_hRecoilControlToggle = NULL;
    g_hAimAssistToggle = NULL;
    g_hSpreadControlToggle = NULL;
    g_hProfileStatsLabel = NULL;
    
    g_hMonitorInfoFrame = NULL;
    g_hCurrentMonitorLabel = NULL;
    g_hMonitorCountLabel = NULL;
    g_hMonitorResolutionLabel = NULL;
    g_hMonitorSwitchButton = NULL;
    
    g_hMovementFrame = NULL;
    g_hSlideKeyLabel = NULL;
    g_hMovementStateLabel = NULL;
    g_hMovementToggleButton = NULL;
    
    logMessage("GUI cleanup completed");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            createControls(hwnd);
            createWeaponProfileControls(hwnd);
            createMultiMonitorControls(hwnd);
            createMovementControls(hwnd);
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1001: // Toggle button
                    onToggleButtonClicked();
                    return 0;
                    
                case 1002: // Profile combo box
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        onProfileSelectionChanged();
                    }
                    return 0;
                    
                case 2001: // Recoil control toggle
                    onEnhancementToggleClicked(static_cast<int>(EnhancementType::RecoilControl));
                    return 0;
                    
                case 2002: // Aim assist toggle
                    onEnhancementToggleClicked(static_cast<int>(EnhancementType::AimAssist));
                    return 0;
                    
                case 2003: // Spread control toggle
                    onEnhancementToggleClicked(static_cast<int>(EnhancementType::SpreadControl));
                    return 0;
                    
                case 3001: // Monitor switch button
                    onMonitorSwitchClicked();
                    return 0;
                    
                case 4001: // Movement toggle button
                    onMovementToggleClicked();
                    return 0;
            }
            break;
            
        case WM_CLOSE:
            onWindowClose();
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// =============================================================================
// GUI UPDATE FUNCTIONS
// =============================================================================

void updateGUIStatus(const std::string& status) {
    if (g_hStatus) {
        SetWindowTextA(g_hStatus, status.c_str());
    }
}

void updateProfileDisplay(const std::string& profileName) {
    // This function can be implemented to update profile display
    // Currently handled by populateProfileComboBox()
    (void)profileName; // Suppress unused parameter warning
}

void updateAnalyticsDisplay(const std::string& analytics) {
    if (g_hAnalyticsLabel) {
        SetWindowTextA(g_hAnalyticsLabel, analytics.c_str());
    }
}

void updateAudioAlertDisplay(const std::string& alert) {
    if (g_hAudioAlertLabel) {
        SetWindowTextA(g_hAudioAlertLabel, alert.c_str());
    }
}

void updateMovementStatusDisplay(const std::string& status) {
    if (g_hMovementStatusLabel) {
        SetWindowTextA(g_hMovementStatusLabel, status.c_str());
    }
}

// New weapon profile update functions
void updateWeaponProfileDisplay(const std::string& profileName) {
    if (g_hActiveProfileLabel) {
        SetWindowTextA(g_hActiveProfileLabel, profileName.c_str());
    }
}

void updateProfileEnhancements(bool recoilControl, bool aimAssist, bool spreadControl) {
    if (g_hRecoilControlToggle) {
        SetWindowTextA(g_hRecoilControlToggle, recoilControl ? "Recoil Control: ON" : "Recoil Control: OFF");
    }
    if (g_hAimAssistToggle) {
        SetWindowTextA(g_hAimAssistToggle, aimAssist ? "Aim Assist: ON" : "Aim Assist: OFF");
    }
    if (g_hSpreadControlToggle) {
        SetWindowTextA(g_hSpreadControlToggle, spreadControl ? "Spread Control: ON" : "Spread Control: OFF");
    }
}

void updateProfileStats(const std::string& stats) {
    if (g_hProfileStatsLabel) {
        SetWindowTextA(g_hProfileStatsLabel, stats.c_str());
    }
}

// Multi-monitor update functions
void updateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height) {
    if (g_hCurrentMonitorLabel) {
        std::string monitorText = "Current Monitor: " + std::to_string(currentMonitor);
        SetWindowTextA(g_hCurrentMonitorLabel, monitorText.c_str());
    }
    if (g_hMonitorCountLabel) {
        std::string countText = "Total Monitors: " + std::to_string(totalMonitors);
        SetWindowTextA(g_hMonitorCountLabel, countText.c_str());
    }
    if (g_hMonitorResolutionLabel) {
        std::string resText = "Resolution: " + std::to_string(width) + "x" + std::to_string(height);
        SetWindowTextA(g_hMonitorResolutionLabel, resText.c_str());
    }
}

void updateCurrentMonitorDisplay(int monitorIndex, int width, int height) {
    updateMonitorInfo(monitorIndex, getConnectedMonitors().size(), width, height);
}

// Movement system update functions
void updateMovementStateDisplay(const std::string& state) {
    if (g_hMovementStateLabel) {
        SetWindowTextA(g_hMovementStateLabel, state.c_str());
    }
}

void updateSlideKeyStatus(bool isActive) {
    if (g_hSlideKeyLabel) {
        std::string status = "Slide Key (Z): " + std::string(isActive ? "ACTIVE" : "Inactive");
        SetWindowTextA(g_hSlideKeyLabel, status.c_str());
    }
}

// =============================================================================
// GUI UTILITY FUNCTIONS
// =============================================================================

void createControls(HWND hwndParent) {
    // Create status label
    g_hStatus = CreateWindowA("STATIC", "Status: Initializing...", 
                             WS_VISIBLE | WS_CHILD, 10, 10, 400, 20, 
                             hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create profile label
    g_hProfileLabel = CreateWindowA("STATIC", "Profile:", 
                                   WS_VISIBLE | WS_CHILD, 10, 40, 60, 20, 
                                   hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create profile combo box
    g_hProfileComboBox = CreateWindowA("COMBOBOX", "", 
                                      WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 
                                      80, 40, 150, 200, hwndParent, (HMENU)1002, 
                                      GetModuleHandle(NULL), NULL);
    
    // Create toggle button
    g_hToggleButton = CreateWindowA("BUTTON", "Enable Assist", 
                                   WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                                   10, 70, 100, 30, hwndParent, (HMENU)1001, 
                                   GetModuleHandle(NULL), NULL);
    
    // Create analytics label
    g_hAnalyticsLabel = CreateWindowA("STATIC", "Analytics: Awaiting data...", 
                                     WS_VISIBLE | WS_CHILD, 10, 110, 400, 20, 
                                     hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create movement status label
    g_hMovementStatusLabel = CreateWindowA("STATIC", "Movement: Stationary", 
                                          WS_VISIBLE | WS_CHILD, 10, 140, 400, 20, 
                                          hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create audio alert label
    g_hAudioAlertLabel = CreateWindowA("STATIC", "Audio: No alerts", 
                                      WS_VISIBLE | WS_CHILD, 10, 170, 400, 20, 
                                      hwndParent, NULL, GetModuleHandle(NULL), NULL);
}

void createWeaponProfileControls(HWND hwndParent) {
    // Create weapon profile frame
    g_hWeaponProfileFrame = CreateWindowA("BUTTON", "Weapon Profile", 
                                         WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                         10, 200, 280, 120, hwndParent, NULL, 
                                         GetModuleHandle(NULL), NULL);
    
    // Create active profile label
    g_hActiveProfileLabel = CreateWindowA("STATIC", "Active Profile: Default", 
                                         WS_VISIBLE | WS_CHILD, 20, 220, 260, 20, 
                                         hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create enhancements frame
    g_hProfileEnhancementsFrame = CreateWindowA("BUTTON", "Enhancements", 
                                               WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                               20, 245, 260, 70, hwndParent, NULL, 
                                               GetModuleHandle(NULL), NULL);
    
    // Create enhancement toggles
    g_hRecoilControlToggle = CreateWindowA("BUTTON", "Recoil Control: OFF", 
                                          WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                          30, 265, 80, 20, hwndParent, (HMENU)2001, 
                                          GetModuleHandle(NULL), NULL);
    
    g_hAimAssistToggle = CreateWindowA("BUTTON", "Aim Assist: OFF", 
                                       WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                       120, 265, 80, 20, hwndParent, (HMENU)2002, 
                                       GetModuleHandle(NULL), NULL);
    
    g_hSpreadControlToggle = CreateWindowA("BUTTON", "Spread Control: OFF", 
                                          WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                          210, 265, 70, 20, hwndParent, (HMENU)2003, 
                                          GetModuleHandle(NULL), NULL);
}

void createMultiMonitorControls(HWND hwndParent) {
    // Create monitor info frame
    g_hMonitorInfoFrame = CreateWindowA("BUTTON", "Multi-Monitor Info", 
                                        WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                        300, 200, 280, 120, hwndParent, NULL, 
                                        GetModuleHandle(NULL), NULL);
    
    // Create monitor info labels
    g_hCurrentMonitorLabel = CreateWindowA("STATIC", "Current Monitor: 1", 
                                          WS_VISIBLE | WS_CHILD, 310, 220, 260, 20, 
                                          hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    g_hMonitorCountLabel = CreateWindowA("STATIC", "Total Monitors: 1", 
                                        WS_VISIBLE | WS_CHILD, 310, 240, 260, 20, 
                                        hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    g_hMonitorResolutionLabel = CreateWindowA("STATIC", "Resolution: 1920x1080", 
                                             WS_VISIBLE | WS_CHILD, 310, 260, 260, 20, 
                                             hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create monitor switch button
    g_hMonitorSwitchButton = CreateWindowA("BUTTON", "Switch Monitor", 
                                          WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                                          310, 285, 100, 25, hwndParent, (HMENU)3001, 
                                          GetModuleHandle(NULL), NULL);
}

void createMovementControls(HWND hwndParent) {
    // Create movement frame
    g_hMovementFrame = CreateWindowA("BUTTON", "Movement System", 
                                     WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                     10, 330, 570, 80, hwndParent, NULL, 
                                     GetModuleHandle(NULL), NULL);
    
    // Create slide key label
    g_hSlideKeyLabel = CreateWindowA("STATIC", "Slide Key (Z): Inactive", 
                                     WS_VISIBLE | WS_CHILD, 20, 350, 200, 20, 
                                     hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create movement state label
    g_hMovementStateLabel = CreateWindowA("STATIC", "Movement State: Stationary", 
                                         WS_VISIBLE | WS_CHILD, 20, 370, 200, 20, 
                                         hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Create movement toggle button
    g_hMovementToggleButton = CreateWindowA("BUTTON", "Enable Movement System", 
                                           WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                                           230, 350, 150, 30, hwndParent, (HMENU)4001, 
                                           GetModuleHandle(NULL), NULL);
}

void populateProfileComboBox() {
    if (!g_hProfileComboBox) return;
    
    // Clear existing items
    SendMessage(g_hProfileComboBox, CB_RESETCONTENT, 0, 0);
    
    // Add default profiles
    SendMessageA(g_hProfileComboBox, CB_ADDSTRING, 0, (LPARAM)"Default");
    SendMessageA(g_hProfileComboBox, CB_ADDSTRING, 0, (LPARAM)"AR Profile");
    SendMessageA(g_hProfileComboBox, CB_ADDSTRING, 0, (LPARAM)"SMG Profile");
    SendMessageA(g_hProfileComboBox, CB_ADDSTRING, 0, (LPARAM)"Sniper Profile");
    
    // Set default selection
    SendMessage(g_hProfileComboBox, CB_SETCURSEL, 0, 0);
}

void refreshGUIState() {
    // Update all GUI elements with current state
    updateGUIStatus("Ready");
    populateProfileComboBox();
    updateMovementStatusDisplay("Stationary");
    updateAnalyticsDisplay("No targets detected");
    updateAudioAlertDisplay("No alerts");
    
    // Update new GUI elements
    updateWeaponProfileDisplay("Default");
    updateProfileEnhancements(false, false, false);
    updateProfileStats("Accuracy: 0% | Shots: 0 | Hits: 0");
    
    // Update monitor info
    auto monitors = getConnectedMonitors();
    int currentMonitor = getCurrentMonitorIndex();
    if (!monitors.empty() && currentMonitor < monitors.size()) {
        const auto& monitor = monitors[currentMonitor];
        updateMonitorInfo(currentMonitor + 1, monitors.size(), monitor.width, monitor.height);
    }
    
    // Update movement system
    updateMovementStateDisplay("Stationary");
    updateSlideKeyStatus(false);
}

void toggleAssistState() {
    // Toggle aim assist state
    onToggleButtonClicked();
}

void updateAllGUIElements() {
    refreshGUIState();
}

// =============================================================================
// MESSAGE POSTING FUNCTIONS
// =============================================================================

void postUpdateAnalytics() {
    // Post a message to update analytics display
    // This would typically be called from another thread
    updateAnalyticsDisplay("Target detected - updating...");
}

void postUpdateAudioAlert(const std::string& alert) {
    // Post a message to update audio alert display
    updateAudioAlertDisplay(alert);
}

void postUpdateMovementStatus(const std::string& status) {
    // Post a message to update movement status display
    updateMovementStatusDisplay(status);
}

void postUpdateWeaponProfile(const std::string& profileName) {
    // Post a message to update weapon profile display
    updateWeaponProfileDisplay(profileName);
}

void postUpdateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height) {
    // Post a message to update monitor info
    updateMonitorInfo(currentMonitor, totalMonitors, width, height);
}

// =============================================================================
// GUI EVENT HANDLERS
// =============================================================================

void onProfileSelectionChanged() {
    if (!g_hProfileComboBox) return;
    
    int selected_index = SendMessage(g_hProfileComboBox, CB_GETCURSEL, 0, 0);
    if (selected_index != CB_ERR) {
        // Handle profile change
        logMessage("Profile selection changed to index: " + std::to_string(selected_index));
        
        // Update weapon profile display
        char profileName[256];
        SendMessageA(g_hProfileComboBox, CB_GETLBTEXT, selected_index, (LPARAM)profileName);
        updateWeaponProfileDisplay(profileName);
    }
}

void onToggleButtonClicked() {
    // Toggle aim assist state
    static bool enabled = false;
    enabled = !enabled;
    
    if (g_hToggleButton) {
        SetWindowTextA(g_hToggleButton, enabled ? "Disable Assist" : "Enable Assist");
    }
    
    if (g_hStatus) {
        SetWindowTextA(g_hStatus, enabled ? "Status: Aim Assist Enabled" : "Status: Aim Assist Disabled");
    }
    
    logMessage("Aim assist " + std::string(enabled ? "enabled" : "disabled"));
}

void onWeaponProfileChanged() {
    // Handle weapon profile change
    logMessage("Weapon profile changed");
}

void onEnhancementToggleClicked(int enhancementType) {
    EnhancementType type = static_cast<EnhancementType>(enhancementType);
    std::string enhancementName;
    
    switch (type) {
        case EnhancementType::RecoilControl:
            enhancementName = "Recoil Control";
            break;
        case EnhancementType::AimAssist:
            enhancementName = "Aim Assist";
            break;
        case EnhancementType::SpreadControl:
            enhancementName = "Spread Control";
            break;
    }
    
    logMessage("Enhancement toggle clicked: " + enhancementName);
}

void onMonitorSwitchClicked() {
    auto monitors = getConnectedMonitors();
    int currentMonitor = getCurrentMonitorIndex();
    
    if (monitors.size() > 1) {
        int nextMonitor = (currentMonitor + 1) % monitors.size();
        if (switchToMonitor(nextMonitor)) {
            logMessage("Switched to monitor " + std::to_string(nextMonitor + 1));
            updateCurrentMonitorDisplay(nextMonitor, monitors[nextMonitor].width, monitors[nextMonitor].height);
        }
    }
}

void onMovementToggleClicked() {
    static bool movementEnabled = false;
    movementEnabled = !movementEnabled;
    
    if (g_hMovementToggleButton) {
        SetWindowTextA(g_hMovementToggleButton, 
                      movementEnabled ? "Disable Movement System" : "Enable Movement System");
    }
    
    logMessage("Movement system " + std::string(movementEnabled ? "enabled" : "disabled"));
}

void runGuiMessageLoop() {
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void onWindowClose() {
    logMessage("GUI window closing");
    // Clean up and exit
    PostQuitMessage(0);
}

// =============================================================================
// MULTI-MONITOR SUPPORT IMPLEMENTATION
// =============================================================================

std::vector<MonitorInfo> getConnectedMonitors() {
    std::vector<MonitorInfo> monitors;
    
    // Use EnumDisplayMonitors to get all connected monitors
    EnumDisplayMonitors(NULL, NULL, 
        [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
            auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
            
            MONITORINFOEX monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            
            if (GetMonitorInfo(hMonitor, &monitorInfo)) {
                MonitorInfo info;
                info.index = monitors->size();
                info.x = monitorInfo.rcMonitor.left;
                info.y = monitorInfo.rcMonitor.top;
                info.width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
                info.height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
                info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
                info.name = monitorInfo.szDevice;
                
                monitors->push_back(info);
            }
            
            return TRUE;
        }, reinterpret_cast<LPARAM>(&monitors));
    
    return monitors;
}

int getCurrentMonitorIndex() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    
    auto monitors = getConnectedMonitors();
    for (size_t i = 0; i < monitors.size(); ++i) {
        if (cursorPos.x >= monitors[i].x && cursorPos.x < monitors[i].x + monitors[i].width &&
            cursorPos.y >= monitors[i].y && cursorPos.y < monitors[i].y + monitors[i].height) {
            return static_cast<int>(i);
        }
    }
    
    return 0; // Default to first monitor
}

bool switchToMonitor(int monitorIndex) {
    auto monitors = getConnectedMonitors();
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size())) {
        return false;
    }
    
    const auto& monitor = monitors[monitorIndex];
    
    // Move cursor to center of target monitor
    int centerX = monitor.x + monitor.width / 2;
    int centerY = monitor.y + monitor.height / 2;
    SetCursorPos(centerX, centerY);
    
    return true;
}

MonitorInfo getMonitorInfo(int monitorIndex) {
    auto monitors = getConnectedMonitors();
    if (monitorIndex >= 0 && monitorIndex < static_cast<int>(monitors.size())) {
        return monitors[monitorIndex];
    }
    
    // Return default monitor info
    return MonitorInfo{0, 0, 0, 1920, 1080, true, "Default"};
}