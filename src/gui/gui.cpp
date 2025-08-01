// gui.cpp - REFACTORED AND UPDATED VERSION v6.1.0
// --------------------------------------------------------------------------------------
// description: GUI implementation using global functions as defined in the header.
//              The GUI communicates through the StateManager and EventSystem.
//              Enhanced with weapon profiles, multi-monitor support, movement system,
//              real-time logging panel, console replacement, and validation integration.
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 6.1.0 - Added validation integration and error handling
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include "gui.h" // For GUI class
#include "state_manager.h" // For state manager
#include "event_system.h" // For event system
#include "common_defines.h" // For GUI control IDs
#include "globals.h" // For logging functions
#include "application_validator.h" // For validation system and error handling

#include <commctrl.h> // For ComboBox control
#include <richedit.h> // For rich edit controls
#include <sstream> // For stringstream
#include <iomanip> // For iomanip
#include <thread>
#include <algorithm>
#include <cctype>

// =============================================================================
// GLOBAL THREAD SAFETY VARIABLES
// =============================================================================

std::mutex g_guiMutex;
std::atomic<bool> g_guiInitialized{false};
std::atomic<bool> g_guiShutdownRequested{false};

// =============================================================================
// SAFETY VALIDATION FUNCTIONS
// =============================================================================

bool isValidWindowHandle(HWND hwnd) {
    if (!hwnd) return false;
    
    __try {
        return IsWindow(hwnd) && IsWindowVisible(hwnd);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception while validating window handle");
        return false;
    }
}

bool isGUIThread() {
    return GetCurrentThreadId() == GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
}

void validateGUIState() {
    if (!g_guiInitialized.load()) {
        throw GUIException("GUI not initialized");
    }
    
    if (g_guiShutdownRequested.load()) {
        throw GUIException("GUI shutdown requested");
    }
}

bool safeSetWindowText(HWND hwnd, const std::string& text) {
    if (!isValidWindowHandle(hwnd)) {
        logWarning("Attempted to set text on invalid window handle");
        return false;
    }
    
    __try {
        return SetWindowTextA(hwnd, text.c_str()) != 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception while setting window text");
        return false;
    }
}

bool safeCreateWindow(HWND& hwnd, const char* className, const char* windowName, 
                     DWORD style, int x, int y, int width, int height, 
                     HWND parent, HMENU menu, HINSTANCE instance, void* param) {
    __try {
        hwnd = CreateWindowExA(0, className, windowName, style, x, y, width, height,
                              parent, menu, instance, param);
        return hwnd != nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Exception while creating window: " + std::to_string(GetLastError()));
        hwnd = nullptr;
        return false;
    }
}

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

// Real-time logging panel controls
HWND g_hLoggingFrame = NULL;
HWND g_hLogDisplay = NULL;
HWND g_hLogClearButton = NULL;
HWND g_hLogLevelComboBox = NULL;
HWND g_hLogAutoScrollToggle = NULL;

// Console replacement controls
HWND g_hConsoleFrame = NULL;
HWND g_hConsoleOutput = NULL;
HWND g_hConsoleInput = NULL;
HWND g_hConsoleSendButton = NULL;

// Performance monitoring controls
HWND g_hPerformanceFrame = NULL;
HWND g_hCpuUsageLabel = NULL;
HWND g_hMemoryUsageLabel = NULL;
HWND g_hThreadCountLabel = NULL;
HWND g_hResponseTimeLabel = NULL;

// =============================================================================
// GLOBAL LOG MANAGER INSTANCE
// =============================================================================

LogManager g_logManager;

// =============================================================================
// GLOBAL GUI RENDER OPTIMIZER
// =============================================================================

// Global GUI render optimizer
static GUIRenderOptimizer g_renderOptimizer;

// Double buffering implementation
bool DoubleBuffer::initialize(HWND hwnd, int w, int h) {
    if (is_initialized) {
        cleanup();
    }
    
    width = w;
    height = h;
    
    // Get the front buffer (window DC)
    front_buffer = GetDC(hwnd);
    if (!front_buffer) {
        logError("Failed to get front buffer DC");
        return false;
    }
    
    // Create the back buffer
    back_buffer = CreateCompatibleDC(front_buffer);
    if (!back_buffer) {
        logError("Failed to create back buffer DC");
        ReleaseDC(hwnd, front_buffer);
        front_buffer = nullptr;
        return false;
    }
    
    // Create the back buffer bitmap
    back_bitmap = CreateCompatibleBitmap(front_buffer, width, height);
    if (!back_bitmap) {
        logError("Failed to create back buffer bitmap");
        DeleteDC(back_buffer);
        ReleaseDC(hwnd, front_buffer);
        back_buffer = nullptr;
        front_buffer = nullptr;
        return false;
    }
    
    // Select the bitmap into the back buffer DC
    old_bitmap = (HBITMAP)SelectObject(back_buffer, back_bitmap);
    
    is_initialized = true;
    logMessage("Double buffer initialized: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void DoubleBuffer::cleanup() {
    if (back_buffer && old_bitmap) {
        SelectObject(back_buffer, old_bitmap);
        old_bitmap = nullptr;
    }
    
    if (back_bitmap) {
        DeleteObject(back_bitmap);
        back_bitmap = nullptr;
    }
    
    if (back_buffer) {
        DeleteDC(back_buffer);
        back_buffer = nullptr;
    }
    
    if (front_buffer) {
        // Note: front_buffer is released by the window, not us
        front_buffer = nullptr;
    }
    
    is_initialized = false;
}

void DoubleBuffer::swap() {
    if (!is_initialized) return;
    
    // Copy back buffer to front buffer
    BitBlt(front_buffer, 0, 0, width, height, back_buffer, 0, 0, SRCCOPY);
}

// GUI render optimizer implementation
bool GUIRenderOptimizer::initialize(HWND hwnd, int width, int height) {
    if (!double_buffer.initialize(hwnd, width, height)) {
        return false;
    }
    
    last_frame_time = std::chrono::steady_clock::now();
    frame_count.store(0);
    needs_redraw.store(true);
    
    logMessage("GUI render optimizer initialized");
    return true;
}

void GUIRenderOptimizer::cleanup() {
    double_buffer.cleanup();
    logMessage("GUI render optimizer cleaned up");
}

void GUIRenderOptimizer::beginFrame() {
    if (!double_buffer.is_initialized) return;
    
    // Clear the back buffer
    RECT rect = {0, 0, double_buffer.width, double_buffer.height};
    HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    FillRect(double_buffer.getBackBuffer(), &rect, brush);
}

void GUIRenderOptimizer::endFrame() {
    if (!double_buffer.is_initialized) return;
    
    // Swap buffers
    double_buffer.swap();
    
    // Update FPS calculation
    auto now = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time);
    
    if (frame_time.count() > 0) {
        current_fps = 1000.0 / frame_time.count();
    }
    
    frame_count.fetch_add(1);
    last_frame_time = now;
    needs_redraw.store(false);
}

void GUIRenderOptimizer::requestRedraw() {
    needs_redraw.store(true);
}

double GUIRenderOptimizer::getCurrentFPS() const {
    return current_fps;
}

void GUIRenderOptimizer::setTargetFPS(double fps) {
    target_fps = fps;
}

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
void createLoggingControls(HWND hwndParent);
void createConsoleControls(HWND hwndParent);
void createPerformanceControls(HWND hwndParent);

// =============================================================================
// GUI CORE FUNCTIONS
// =============================================================================

bool initializeGUI() {
    try {
        const char CLASS_NAME[] = "TacticalAimAssistGUIClass";
        
        // Register the window class with safety checks
        WNDCLASSA wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        
        SAFE_WINDOWS_CALL(
            if (!RegisterClassA(&wc)) {
                logError("Failed to register window class");
                return false;
            },
            "RegisterClassA"
        );
        
        // Create the main window with safety checks
        HWND hMainWnd = nullptr;
        SAFE_WINDOWS_CALL(
            hMainWnd = CreateWindowExA(
                0,                          // Optional window styles
                CLASS_NAME,                 // Window class
                "Tactical Aim Assist v6.1.0", // Window text
                WS_OVERLAPPEDWINDOW,       // Window style
                CW_USEDEFAULT, CW_USEDEFAULT, 800, 700, // Size and position
                NULL,       // Parent window    
                NULL,       // Menu
                GetModuleHandle(NULL), // Instance handle
                NULL        // Additional application data
            ),
            "CreateWindowExA"
        );
        
        if (!hMainWnd) {
            logError("Failed to create main window");
            return false;
        }
        
        // Initialize render optimizer with double buffering
        if (!g_renderOptimizer.initialize(hMainWnd, 800, 700)) {
            logWarning("Failed to initialize render optimizer, falling back to single buffer");
        }
        
        // Create all GUI controls with safety checks
        __try {
            createControls(hMainWnd);
            createWeaponProfileControls(hMainWnd);
            createMultiMonitorControls(hMainWnd);
            createMovementControls(hMainWnd);
            createLoggingControls(hMainWnd);
            createConsoleControls(hMainWnd);
            createPerformanceControls(hMainWnd);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Exception while creating GUI controls");
            return false;
        }
        
        // Initial state refresh
        refreshGUIState();
        
        // Show the window
        SAFE_WINDOWS_CALL(
            ShowWindow(hMainWnd, SW_SHOW);
            UpdateWindow(hMainWnd);
            ,
            "ShowWindow/UpdateWindow"
        );
        
        // Mark GUI as initialized
        g_guiInitialized.store(true);
        g_guiShutdownRequested.store(false);
        
        logMessage("GUI system initialized successfully");
        return true;
        
    } catch (const GUIException& e) {
        logError("GUI initialization failed: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        logError("Unexpected error during GUI initialization: " + std::string(e.what()));
        return false;
    } catch (...) {
        logError("Unknown exception during GUI initialization");
        return false;
    }
}

void cleanupGUI() {
    GUI_THREAD_SAFE({
        // Mark GUI as shutting down
        g_guiShutdownRequested.store(true);
        g_guiInitialized.store(false);
        
        // Clean up window handles safely
        __try {
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
            
            g_hLoggingFrame = NULL;
            g_hLogDisplay = NULL;
            g_hLogClearButton = NULL;
            g_hLogLevelComboBox = NULL;
            g_hLogAutoScrollToggle = NULL;
            
            g_hConsoleFrame = NULL;
            g_hConsoleOutput = NULL;
            g_hConsoleInput = NULL;
            g_hConsoleSendButton = NULL;
            
            g_hPerformanceFrame = NULL;
            g_hCpuUsageLabel = NULL;
            g_hMemoryUsageLabel = NULL;
            g_hThreadCountLabel = NULL;
            g_hResponseTimeLabel = NULL;
            
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            logError("Exception during GUI cleanup");
        }
        
        logMessage("GUI cleanup completed");
    });
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    __try {
        // Validate window handle
        if (!isValidWindowHandle(hwnd)) {
            logWarning("Invalid window handle in WindowProc");
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        
        switch (uMsg) {
            case WM_CREATE:
                __try {
                    createControls(hwnd);
                    createWeaponProfileControls(hwnd);
                    createMultiMonitorControls(hwnd);
                    createMovementControls(hwnd);
                    createLoggingControls(hwnd);
                    createConsoleControls(hwnd);
                    createPerformanceControls(hwnd);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    logError("Exception while creating controls in WindowProc");
                }
                return 0;
                
            case WM_COMMAND:
                __try {
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
                            
                        case 5001: // Log clear button
                            onLogClearClicked();
                            return 0;
                            
                        case 5002: // Log level combo box
                            if (HIWORD(wParam) == CBN_SELCHANGE) {
                                onLogLevelChanged();
                            }
                            return 0;
                            
                        case 6001: // Console send button
                            onConsoleInputSubmitted();
                            return 0;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    logError("Exception in WM_COMMAND handler");
                }
                break;
                
            case WM_CLOSE:
                __try {
                    onWindowClose();
                    SAFE_WINDOWS_CALL(
                        DestroyWindow(hwnd);
                        ,
                        "DestroyWindow"
                    );
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    logError("Exception in WM_CLOSE handler");
                }
                return 0;
                
            case WM_DESTROY:
                SAFE_WINDOWS_CALL(
                    PostQuitMessage(0);
                    ,
                    "PostQuitMessage"
                );
                return 0;
        }
        
        SAFE_WINDOWS_CALL(
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
            ,
            "DefWindowProc"
        );
        
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Critical exception in WindowProc");
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// =============================================================================
// GUI UPDATE FUNCTIONS
// =============================================================================

void updateGUIStatus(const std::string& status) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hStatus)) {
            safeSetWindowText(g_hStatus, status);
        }
    });
}

void updateProfileDisplay(const std::string& profileName) {
    // This function can be implemented to update profile display
    // Currently handled by populateProfileComboBox()
    (void)profileName; // Suppress unused parameter warning
}

void updateAnalyticsDisplay(const std::string& analytics) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hAnalyticsLabel)) {
            safeSetWindowText(g_hAnalyticsLabel, analytics);
        }
    });
}

void updateAudioAlertDisplay(const std::string& alert) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hAudioAlertLabel)) {
            safeSetWindowText(g_hAudioAlertLabel, alert);
        }
    });
}

void updateMovementStatusDisplay(const std::string& status) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hMovementStatusLabel)) {
            safeSetWindowText(g_hMovementStatusLabel, status);
        }
    });
}

// New weapon profile update functions
void updateWeaponProfileDisplay(const std::string& profileName) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hActiveProfileLabel)) {
            safeSetWindowText(g_hActiveProfileLabel, profileName);
        }
    });
}

void updateProfileEnhancements(bool recoilControl, bool aimAssist, bool spreadControl) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hRecoilControlToggle)) {
            safeSetWindowText(g_hRecoilControlToggle, recoilControl ? "Recoil Control: ON" : "Recoil Control: OFF");
        }
        if (isValidWindowHandle(g_hAimAssistToggle)) {
            safeSetWindowText(g_hAimAssistToggle, aimAssist ? "Aim Assist: ON" : "Aim Assist: OFF");
        }
        if (isValidWindowHandle(g_hSpreadControlToggle)) {
            safeSetWindowText(g_hSpreadControlToggle, spreadControl ? "Spread Control: ON" : "Spread Control: OFF");
        }
    });
}

void updateProfileStats(const std::string& stats) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hProfileStatsLabel)) {
            safeSetWindowText(g_hProfileStatsLabel, stats);
        }
    });
}

// Multi-monitor update functions
void updateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hCurrentMonitorLabel)) {
            std::string monitorText = "Current Monitor: " + std::to_string(currentMonitor);
            safeSetWindowText(g_hCurrentMonitorLabel, monitorText);
        }
        if (isValidWindowHandle(g_hMonitorCountLabel)) {
            std::string countText = "Total Monitors: " + std::to_string(totalMonitors);
            safeSetWindowText(g_hMonitorCountLabel, countText);
        }
        if (isValidWindowHandle(g_hMonitorResolutionLabel)) {
            std::string resText = "Resolution: " + std::to_string(width) + "x" + std::to_string(height);
            safeSetWindowText(g_hMonitorResolutionLabel, resText);
        }
    });
}

void updateCurrentMonitorDisplay(int monitorIndex, int width, int height) {
    updateMonitorInfo(monitorIndex, getConnectedMonitors().size(), width, height);
}

// Movement system update functions
void updateMovementStateDisplay(const std::string& state) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hMovementStateLabel)) {
            safeSetWindowText(g_hMovementStateLabel, state);
        }
    });
}

void updateSlideKeyStatus(bool isActive) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hSlideKeyLabel)) {
            std::string status = "Slide Key (Z): " + std::string(isActive ? "ACTIVE" : "Inactive");
            safeSetWindowText(g_hSlideKeyLabel, status);
        }
    });
}

// Performance monitoring update functions
void updatePerformanceMetrics(double cpuUsage, size_t memoryMB, size_t threadCount, double responseTime) {
    GUI_THREAD_SAFE({
        if (isValidWindowHandle(g_hCpuUsageLabel)) {
            std::string cpuText = "CPU: " + std::to_string(static_cast<int>(cpuUsage)) + "%";
            safeSetWindowText(g_hCpuUsageLabel, cpuText);
        }
        
        if (isValidWindowHandle(g_hMemoryUsageLabel)) {
            std::string memText = "Memory: " + std::to_string(memoryMB) + " MB";
            safeSetWindowText(g_hMemoryUsageLabel, memText);
        }
        
        if (isValidWindowHandle(g_hThreadCountLabel)) {
            std::string threadText = "Threads: " + std::to_string(threadCount);
            safeSetWindowText(g_hThreadCountLabel, threadText);
        }
        
        if (isValidWindowHandle(g_hResponseTimeLabel)) {
            std::string responseText = "Response: " + std::to_string(static_cast<int>(responseTime)) + "ms";
            safeSetWindowText(g_hResponseTimeLabel, responseText);
        }
    });
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

void createLoggingControls(HWND hwndParent) {
    // Create logging frame
    g_hLoggingFrame = CreateWindowA("BUTTON", "Real-Time Logging", 
                                   WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                   10, 420, 570, 150, hwndParent, NULL, 
                                   GetModuleHandle(NULL), NULL);
    
    // Create log display
    g_hLogDisplay = CreateWindowA("EDIT", "", 
                                  WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 
                                  20, 440, 550, 100, hwndParent, (HMENU)0, // No specific menu ID for now
                                  GetModuleHandle(NULL), NULL);
    
    // Create log clear button
    g_hLogClearButton = CreateWindowA("BUTTON", "Clear Log", 
                                      WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                                      20, 550, 100, 30, hwndParent, (HMENU)5001, 
                                      GetModuleHandle(NULL), NULL);
    
    // Create log level combo box
    g_hLogLevelComboBox = CreateWindowA("COMBOBOX", "Log Level: All", 
                                        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 
                                        120, 440, 150, 100, hwndParent, (HMENU)5002, 
                                        GetModuleHandle(NULL), NULL);
    
    // Create log auto-scroll toggle
    g_hLogAutoScrollToggle = CreateWindowA("BUTTON", "Auto-Scroll: ON", 
                                           WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 
                                           280, 440, 100, 20, hwndParent, (HMENU)0, // No specific menu ID for now
                                           GetModuleHandle(NULL), NULL);
}

void createConsoleControls(HWND hwndParent) {
    // Create console frame
    g_hConsoleFrame = CreateWindowA("BUTTON", "Console Replacement", 
                                   WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 
                                   10, 580, 570, 100, hwndParent, NULL, 
                                   GetModuleHandle(NULL), NULL);
    
    // Create console output
    g_hConsoleOutput = CreateWindowA("EDIT", "", 
                                    WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY, 
                                    20, 600, 550, 40, hwndParent, (HMENU)0, // No specific menu ID for now
                                    GetModuleHandle(NULL), NULL);
    
    // Create console input
    g_hConsoleInput = CreateWindowA("EDIT", "", 
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 
                                   20, 650, 400, 20, hwndParent, (HMENU)0, // No specific menu ID for now
                                   GetModuleHandle(NULL), NULL);
    
    // Create console send button
    g_hConsoleSendButton = CreateWindowA("BUTTON", "Send", 
                                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 
                                         420, 650, 100, 20, hwndParent, (HMENU)6001, 
                                         GetModuleHandle(NULL), NULL);
}

void createPerformanceControls(HWND hwndParent) {
    // Create performance monitoring frame
    g_hPerformanceFrame = CreateWindowA("BUTTON", "Performance Monitor",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10, 500, 200, 120, hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // CPU usage label
    g_hCpuUsageLabel = CreateWindowA("STATIC", "CPU: 0%",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 520, 180, 20, hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Memory usage label
    g_hMemoryUsageLabel = CreateWindowA("STATIC", "Memory: 0 MB",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 540, 180, 20, hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Thread count label
    g_hThreadCountLabel = CreateWindowA("STATIC", "Threads: 0",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 560, 180, 20, hwndParent, NULL, GetModuleHandle(NULL), NULL);
    
    // Response time label
    g_hResponseTimeLabel = CreateWindowA("STATIC", "Response: 0ms",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 580, 180, 20, hwndParent, NULL, GetModuleHandle(NULL), NULL);
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
    if (!monitors.empty() && currentMonitor < static_cast<int>(monitors.size())) {
        const auto& monitor = monitors[currentMonitor];
        updateMonitorInfo(currentMonitor + 1, monitors.size(), monitor.width, monitor.height);
    }
    
    // Update movement system
    updateMovementStateDisplay("Stationary");
    updateSlideKeyStatus(false);
    
    // Initialize logging controls
    if (g_hLogLevelComboBox) {
        SendMessage(g_hLogLevelComboBox, CB_RESETCONTENT, 0, 0);
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"All");
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"Debug");
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"Info");
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"Warning");
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"Error");
        SendMessageA(g_hLogLevelComboBox, CB_ADDSTRING, 0, (LPARAM)"Fatal");
        SendMessage(g_hLogLevelComboBox, CB_SETCURSEL, 0, 0);
    }
    
    // Initialize console
    if (g_hConsoleOutput) {
        addConsoleOutput("Tactical Aim Assist v6.1 - Console Ready");
        addConsoleOutput("Type 'help' for available commands");
    }
    
    // Add initial log entries
    addLogEntry("GUI initialized successfully", LogLevel::Info);
    addLogEntry("Movement system ready", LogLevel::Debug);
    addLogEntry("Weapon profiles loaded", LogLevel::Info);
    addLogEntry("Validation system active", LogLevel::Info);
    
    // Update validation status
    updateValidationStatus();
}

void updateValidationStatus() {
    if (isApplicationHealthy()) {
        updateGUIStatus("Ready - System Healthy");
    } else {
        updateGUIStatus("Warning - System Issues Detected");
        addLogEntry("System health check failed", LogLevel::Warning);
    }
}

void toggleAssistState() {
    // Toggle aim assist state
    onToggleButtonClicked();
}

void updateAllGUIElements() {
    refreshGUIState();
}

// =============================================================================
// LOG MANAGER IMPLEMENTATION
// =============================================================================

void LogManager::addEntry(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    
    // Add new entry
    m_logEntries.emplace_back(message, level);
    
    // Remove old entries if we exceed max
    while (m_logEntries.size() > m_maxEntries) {
        m_logEntries.pop_front();
    }
}

void LogManager::clear() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logEntries.clear();
}

std::vector<LogEntry> LogManager::getEntries() const {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return std::vector<LogEntry>(m_logEntries.begin(), m_logEntries.end());
}

void LogManager::setMaxEntries(size_t max) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_maxEntries = max;
    
    // Remove excess entries
    while (m_logEntries.size() > m_maxEntries) {
        m_logEntries.pop_front();
    }
}

void LogManager::setLogLevel(LogLevel level) {
    m_currentLevel = level;
}

void LogManager::setAutoScroll(bool enabled) {
    m_autoScroll = enabled;
}

bool LogManager::getAutoScroll() const {
    return m_autoScroll;
}

LogLevel LogManager::getCurrentLevel() const {
    return m_currentLevel;
}

// =============================================================================
// LOGGING SYSTEM FUNCTIONS
// =============================================================================

void addLogEntry(const std::string& message, LogLevel level) {
    g_logManager.addEntry(message, level);
    
    // Update GUI if available
    if (g_hLogDisplay) {
        std::string timestamp = getCurrentTimeString();
        std::string levelStr;
        switch (level) {
            case LogLevel::Debug: levelStr = "[DEBUG]"; break;
            case LogLevel::Info: levelStr = "[INFO]"; break;
            case LogLevel::Warning: levelStr = "[WARN]"; break;
            case LogLevel::Error: levelStr = "[ERROR]"; break;
            case LogLevel::Critical: levelStr = "[FATAL]"; break;
        }
        
        std::string logLine = timestamp + " " + levelStr + " " + message + "\r\n";
        
        // Append to log display
        SendMessageA(g_hLogDisplay, EM_REPLACESEL, FALSE, (LPARAM)logLine.c_str());
        
        // Auto-scroll if enabled
        if (g_logManager.getAutoScroll()) {
            SendMessage(g_hLogDisplay, EM_SCROLLCARET, 0, 0);
        }
    }
}

void clearLogDisplay() {
    if (g_hLogDisplay) {
        SetWindowTextA(g_hLogDisplay, "");
    }
    g_logManager.clear();
}

void setLogLevel(LogLevel level) {
    g_logManager.setLogLevel(level);
}

void setAutoScroll(bool enabled) {
    g_logManager.setAutoScroll(enabled);
    if (g_hLogAutoScrollToggle) {
        SetWindowTextA(g_hLogAutoScrollToggle, 
                      enabled ? "Auto-Scroll: ON" : "Auto-Scroll: OFF");
    }
}

// =============================================================================
// CONSOLE REPLACEMENT FUNCTIONS
// =============================================================================

void addConsoleOutput(const std::string& message) {
    if (g_hConsoleOutput) {
        std::string timestamp = getCurrentTimeString();
        std::string outputLine = timestamp + " " + message + "\r\n";
        
        // Append to console output
        SendMessageA(g_hConsoleOutput, EM_REPLACESEL, FALSE, (LPARAM)outputLine.c_str());
        
        // Auto-scroll
        SendMessage(g_hConsoleOutput, EM_SCROLLCARET, 0, 0);
    }
}

void processConsoleInput(const std::string& command) {
    // Process console commands here
    if (command == "help") {
        addConsoleOutput("Available commands: help, status, clear, quit");
    } else if (command == "status") {
        addConsoleOutput("System status: Running");
    } else if (command == "clear") {
        clearConsoleOutput();
    } else if (command == "quit") {
        addConsoleOutput("Quit command received");
        // In a real application, you would trigger application shutdown
    } else {
        addConsoleOutput("Unknown command: " + command);
    }
}

void clearConsoleOutput() {
    if (g_hConsoleOutput) {
        SetWindowTextA(g_hConsoleOutput, "");
    }
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

void postLogEntry(const std::string& message, LogLevel level) {
    // Post a message to add log entry
    addLogEntry(message, level);
}

void postConsoleOutput(const std::string& message) {
    // Post a message to add console output
    addConsoleOutput(message);
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

void onLogClearClicked() {
    if (g_hLogDisplay) {
        SetWindowTextA(g_hLogDisplay, "");
    }
    logMessage("Log cleared");
}

void onLogLevelChanged() {
    int selected_index = SendMessage(g_hLogLevelComboBox, CB_GETCURSEL, 0, 0);
    if (selected_index != CB_ERR) {
        std::string logLevelText;
        switch (selected_index) {
            case 0: logLevelText = "Log Level: All"; break;
            case 1: logLevelText = "Log Level: Debug"; break;
            case 2: logLevelText = "Log Level: Info"; break;
            case 3: logLevelText = "Log Level: Warning"; break;
            case 4: logLevelText = "Log Level: Error"; break;
            case 5: logLevelText = "Log Level: Fatal"; break;
            default: logLevelText = "Log Level: All"; break;
        }
        SetWindowTextA(g_hLogLevelComboBox, logLevelText.c_str());
        logMessage("Log level changed to: " + logLevelText);
    }
}

void onConsoleInputSubmitted() {
    if (g_hConsoleInput && g_hConsoleSendButton) {
        int inputLength = GetWindowTextLength(g_hConsoleInput);
        char inputText[256];
        GetWindowTextA(g_hConsoleInput, inputText, inputLength + 1);
        
        // In a real application, you would send this input to a backend
        // For now, we'll just log it and display it in the console output
        logMessage("Console Input: " + std::string(inputText));
        
        // Append to console output
        if (g_hConsoleOutput) {
            SendMessageA(g_hConsoleOutput, EM_REPLACESEL, FALSE, (LPARAM)""); // Clear previous text
            SendMessageA(g_hConsoleOutput, EM_REPLACESEL, FALSE, (LPARAM)inputText);
            SendMessageA(g_hConsoleOutput, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
        }
        
        // Clear input field
        SetWindowTextA(g_hConsoleInput, "");
    }
}

void runGuiMessageLoop() {
    MSG msg = {};
    
    logMessage("GUI message loop started");
    
    __try {
        while (g_application_running.load() && !g_guiShutdownRequested.load()) {
            // Use PeekMessage instead of GetMessage to prevent blocking
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    logMessage("GUI received quit message");
                    break;
                }
                
                SAFE_WINDOWS_CALL(
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    ,
                    "TranslateMessage/DispatchMessage"
                );
            } else {
                // No messages available, perform periodic health checks
                VALIDATE_SYSTEM_HEALTH();
                
                // Small sleep to prevent busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        logError("Critical exception in GUI message loop");
        g_guiShutdownRequested.store(true);
    }
    
    logMessage("GUI message loop ended");
}

void onWindowClose() {
    logMessage("GUI window closing");
    
    // Perform final health check before closing
    VALIDATE_SYSTEM_HEALTH();
    
    // Show confirmation dialog if there are unsaved changes
    int result = MessageBoxA(nullptr, 
                           "Are you sure you want to close the application?",
                           "Confirm Exit", 
                           MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
    
    if (result == IDYES) {
        // Clean up and exit
        PostQuitMessage(0);
    }
}

// =============================================================================
// MULTI-MONITOR SUPPORT IMPLEMENTATION
// =============================================================================

std::vector<MonitorInfo> getConnectedMonitors() {
    std::vector<MonitorInfo> monitors;
    
    // Use EnumDisplayMonitors to get all connected monitors
    EnumDisplayMonitors(NULL, NULL, 
        [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
            (void)hdcMonitor;  // Suppress unused parameter warning
            (void)lprcMonitor; // Suppress unused parameter warning
            
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