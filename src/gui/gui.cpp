// gui.cpp - REFACTORED
// description: Clean GUI implementation using centralized definitions
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - Refactored
// date: 2025-07-16
// project: Tactical Aim Assist

#include "gui.h"
#include "globals.h"
#include "profiles.h"
#include "common_defines.h"
#include <commctrl.h>
#include <sstream>

// =============================================================================
// GUI CONTROL HANDLES - DEFINITIONS
// =============================================================================
HWND g_hStatus = NULL;
HWND g_hProfileLabel = NULL;
HWND g_hToggleButton = NULL;
HWND g_hAnalyticsLabel = NULL;
HWND g_hAudioAlertLabel = NULL;
HWND g_hMovementStatusLabel = NULL;
HWND g_hProfileComboBox = NULL;

// =============================================================================
// PRIVATE HELPER FUNCTIONS
// =============================================================================
namespace {
    void createGUIControls(HWND hwnd) {
        // Status label
        g_hStatus = CreateWindow(L"STATIC", L"System Status: Initializing...", 
                                WS_VISIBLE | WS_CHILD,
                                10, 10, 300, 20, hwnd, 
                                reinterpret_cast<HMENU>(ID_STATUS_LABEL), 
                                NULL, NULL);

        // Profile label
        g_hProfileLabel = CreateWindow(L"STATIC", L"Profile: Loading...", 
                                      WS_VISIBLE | WS_CHILD,
                                      10, 40, 200, 20, hwnd, 
                                      reinterpret_cast<HMENU>(ID_PROFILE_LABEL), 
                                      NULL, NULL);

        // Profile ComboBox
        g_hProfileComboBox = CreateWindow(L"COMBOBOX", L"", 
                                         WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
                                         220, 38, 150, 100, hwnd, 
                                         reinterpret_cast<HMENU>(ID_PROFILE_COMBOBOX), 
                                         NULL, NULL);

        // Toggle button
        g_hToggleButton = CreateWindow(L"BUTTON", L"Toggle Assist", 
                                      WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                      10, 70, 100, 30, hwnd, 
                                      reinterpret_cast<HMENU>(ID_TOGGLE_BUTTON), 
                                      NULL, NULL);

        // Analytics label
        g_hAnalyticsLabel = CreateWindow(L"STATIC", L"Analytics: Ready", 
                                        WS_VISIBLE | WS_CHILD,
                                        10, 110, 350, 20, hwnd, 
                                        reinterpret_cast<HMENU>(ID_ANALYTICS_LABEL), 
                                        NULL, NULL);

        // Audio alert label
        g_hAudioAlertLabel = CreateWindow(L"STATIC", L"Audio: No alerts", 
                                         WS_VISIBLE | WS_CHILD,
                                         10, 140, 350, 20, hwnd, 
                                         reinterpret_cast<HMENU>(ID_AUDIO_ALERT_LABEL), 
                                         NULL, NULL);

        // Movement status label
        g_hMovementStatusLabel = CreateWindow(L"STATIC", L"Movement: Stationary", 
                                             WS_VISIBLE | WS_CHILD,
                                             10, 170, 350, 20, hwnd, 
                                             reinterpret_cast<HMENU>(ID_MOVEMENT_STATUS_LABEL), 
                                             NULL, NULL);
    }

    std::wstring stringToWstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
}

// =============================================================================
// GUI CORE FUNCTIONS - IMPLEMENTATIONS
// =============================================================================
bool initializeGUI() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TacticalAimAssistGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        logMessage("ERROR: Failed to register window class");
        return false;
    }

    g_hWnd = CreateWindowEx(0, L"TacticalAimAssistGUI", L"Tactical Aim Assist v3.0",
                           WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           400, 250, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!g_hWnd) {
        logMessage("ERROR: Failed to create main window");
        return false;
    }

    createGUIControls(g_hWnd);
    populateProfileComboBox();
    refreshGUIState();
    
    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);
    
    logMessage("GUI initialized successfully");
    return true;
}

void cleanupGUI() {
    if (g_hWnd && IsWindow(g_hWnd)) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
    UnregisterClass(L"TacticalAimAssistGUI", GetModuleHandle(NULL));
    logMessage("GUI cleanup completed");
}

// =============================================================================
// WINDOW PROCEDURE - IMPLEMENTATION
// =============================================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND: {
            int controlId = LOWORD(wParam);
            int notifyCode = HIWORD(wParam);
            
            switch (controlId) {
                case ID_TOGGLE_BUTTON:
                    if (notifyCode == BN_CLICKED) {
                        onToggleButtonClicked();
                    }
                    break;
                    
                case ID_PROFILE_COMBOBOX:
                    if (notifyCode == CBN_SELCHANGE) {
                        onProfileSelectionChanged();
                    }
                    break;
            }
            break;
        }
        
        case WM_LOG_MESSAGE: {
            auto* message = reinterpret_cast<std::string*>(lParam);
            if (message) {
                updateGUIStatus(*message);
                delete message;
            }
            break;
        }
        
        case WM_UPDATE_ANALYTICS: {
            postUpdateAnalytics();
            break;
        }
        
        case WM_UPDATE_AUDIO_ALERT: {
            auto* alert = reinterpret_cast<std::string*>(lParam);
            if (alert) {
                updateAudioAlertDisplay(*alert);
                delete alert;
            }
            break;
        }
        
        case WM_UPDATE_MOVEMENT_STATUS: {
            auto* status = reinterpret_cast<std::string*>(lParam);
            if (status) {
                updateMovementStatusDisplay(*status);
                delete status;
            }
            break;
        }
        
        case WM_CLOSE:
            onWindowClose();
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// =============================================================================
// GUI UPDATE FUNCTIONS - IMPLEMENTATIONS
// =============================================================================
void updateGUIStatus(const std::string& status) {
    if (g_hStatus && IsWindow(g_hStatus)) {
        std::wstring wstatus = stringToWstring("Status: " + status);
        SetWindowText(g_hStatus, wstatus.c_str());
    }
}

void updateProfileDisplay(const std::string& profileName) {
    if (g_hProfileLabel && IsWindow(g_hProfileLabel)) {
        std::wstring wprofile = stringToWstring("Profile: " + profileName);
        SetWindowText(g_hProfileLabel, wprofile.c_str());
    }
}

void updateAnalyticsDisplay(const std::string& analytics) {
    if (g_hAnalyticsLabel && IsWindow(g_hAnalyticsLabel)) {
        std::wstring wanalytics = stringToWstring("Analytics: " + analytics);
        SetWindowText(g_hAnalyticsLabel, wanalytics.c_str());
    }
}

void updateAudioAlertDisplay(const std::string& alert) {
    if (g_hAudioAlertLabel && IsWindow(g_hAudioAlertLabel)) {
        std::wstring walert = stringToWstring("Audio: " + (alert.empty() ? "No alerts" : alert));
        SetWindowText(g_hAudioAlertLabel, walert.c_str());
    }
}

void updateMovementStatusDisplay(const std::string& status) {
    if (g_hMovementStatusLabel && IsWindow(g_hMovementStatusLabel)) {
        std::wstring wstatus = stringToWstring("Movement: " + status);
        SetWindowText(g_hMovementStatusLabel, wstatus.c_str());
    }
}

// =============================================================================
// GUI UTILITY FUNCTIONS - IMPLEMENTATIONS
// =============================================================================
void populateProfileComboBox() {
    if (!g_hProfileComboBox || !IsWindow(g_hProfileComboBox)) return;
    
    // Clear existing items
    SendMessage(g_hProfileComboBox, CB_RESETCONTENT, 0, 0);
    
    // Add weapon profiles
    for (size_t i = 0; i < g_weaponProfiles.size(); ++i) {
        std::wstring wname = stringToWstring(g_weaponProfiles[i].name);
        SendMessage(g_hProfileComboBox, CB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>(wname.c_str()));
    }
    
    // Select current active profile
    int activeIndex = GlobalState::getActiveProfileIndex();
    SendMessage(g_hProfileComboBox, CB_SETCURSEL, activeIndex, 0);
}

void refreshGUIState() {
    // Update assist status
    bool assistEnabled = GlobalState::isAssistEnabled();
    updateGUIStatus(assistEnabled ? "Active" : "Disabled");
    
    // Update toggle button text
    if (g_hToggleButton && IsWindow(g_hToggleButton)) {
        SetWindowText(g_hToggleButton, assistEnabled ? L"Disable Assist" : L"Enable Assist");
    }
    
    // Update profile display
    int activeIndex = GlobalState::getActiveProfileIndex();
    if (activeIndex >= 0 && static_cast<size_t>(activeIndex) < g_weaponProfiles.size()) {
        updateProfileDisplay(g_weaponProfiles[activeIndex].name);
    }
    
    // Update movement status
    PlayerMovementState currentState = GlobalState::getCurrentMovementState();
    updateMovementStatusDisplay(playerMovementStateToString(currentState));
}

// =============================================================================
// MESSAGE POSTING FUNCTIONS - IMPLEMENTATIONS
// =============================================================================
void postUpdateAnalytics() {
    if (!g_hWnd || !IsWindow(g_hWnd)) return;
    
    std::stringstream analytics;
    analytics << "Profiles: " << g_weaponProfiles.size() 
              << " | Active: " << GlobalState::getActiveProfileIndex()
              << " | State: " << playerMovementStateToString(GlobalState::getCurrentMovementState());
    
    updateAnalyticsDisplay(analytics.str());
}

void postUpdateAudioAlert(const std::string& alert) {
    if (!g_hWnd || !IsWindow(g_hWnd)) return;
    
    auto* alertCopy = new std::string(alert);
    PostMessage(g_hWnd, WM_UPDATE_AUDIO_ALERT, 0, reinterpret_cast<LPARAM>(alertCopy));
}

void postUpdateMovementStatus(const std::string& status) {
    if (!g_hWnd || !IsWindow(g_hWnd)) return;
    
    auto* statusCopy = new std::string(status);
    PostMessage(g_hWnd, WM_UPDATE_MOVEMENT_STATUS, 0, reinterpret_cast<LPARAM>(statusCopy));
}

// =============================================================================
// GUI EVENT HANDLERS - IMPLEMENTATIONS
// =============================================================================
void onProfileSelectionChanged() {
    if (!g_hProfileComboBox || !IsWindow(g_hProfileComboBox)) return;
    
    int selectedIndex = SendMessage(g_hProfileComboBox, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR && selectedIndex >= 0 && 
        static_cast<size_t>(selectedIndex) < g_weaponProfiles.size()) {
        
        GlobalState::setActiveProfile(selectedIndex);
        updateProfileDisplay(g_weaponProfiles[selectedIndex].name);
        logMessage("Profile changed via GUI to: " + g_weaponProfiles[selectedIndex].name);
    }
}

void onToggleButtonClicked() {
    toggleAssistState();
}

void toggleAssistState() {
    bool currentState = GlobalState::isAssistEnabled();
    GlobalState::setAssistEnabled(!currentState);
    refreshGUIState();
    
    logMessage(std::string("Assist ") + (!currentState ? "enabled" : "disabled") + " via GUI");
}

void onWindowClose() {
    logMessage("GUI close requested");
    GlobalState::setAssistEnabled(false);
    g_running.store(false, std::memory_order_release);
}