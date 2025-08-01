// gui.cpp - REFACTORED AND UPDATED VERSION v4.0.0
// description: GUI implementation using global functions as defined in the header.
//              The GUI communicates through the StateManager and EventSystem.
// developer: ingekastel & Asistente de Programación
// license: GNU General Public License v3.0
// version: 4.0.0 - Global function approach matching header file.
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

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

// Note: stringToWstring function removed as we're using ANSI Windows API functions throughout

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

void createControls(HWND hwndParent);

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

    // Create the main window
    HWND hMainWnd = CreateWindowExA(
        0, CLASS_NAME, "Tactical Aim Assist v4.0",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        420, 280, NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hMainWnd) {
        logError("GUI Error: Failed to create main window.");
        return false;
    }

    // Create controls
    createControls(hMainWnd);
    
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
    
    logMessage("GUI cleanup completed");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            createControls(hwnd);
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
}

void toggleAssistState() {
    // Toggle aim assist state
    onToggleButtonClicked();
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

// =============================================================================
// GUI EVENT HANDLERS
// =============================================================================

void onProfileSelectionChanged() {
    if (!g_hProfileComboBox) return;
    
    int selected_index = SendMessage(g_hProfileComboBox, CB_GETCURSEL, 0, 0);
    if (selected_index != CB_ERR) {
        // Handle profile change
        logMessage("Profile selection changed to index: " + std::to_string(selected_index));
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