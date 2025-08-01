// gui.h - REFACTORED
// --------------------------------------------------------------------------------------
// description: Clean GUI header without duplicated definitions
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// --------------------------------------------------------------------------------------
// version: 5.0.0 - Enhanced with logging panel and console replacement
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <algorithm>
#include <cctype>
#include <functional>
#include <exception>
#include "common_defines.h" // For GUI control IDs

// =============================================================================
// SAFETY MACROS AND CONSTANTS
// =============================================================================

// Structured exception handling for Windows API calls
#define SAFE_WINDOWS_CALL(call, error_msg) \
    __try { \
        call; \
    } __except(EXCEPTION_EXECUTE_HANDLER) { \
        logError("Windows API exception in " error_msg ": " + std::to_string(GetLastError())); \
        return false; \
    }

// Safe window handle validation
#define SAFE_WINDOW_HANDLE(hwnd) \
    (hwnd && IsWindow(hwnd) && IsWindowVisible(hwnd))

// Thread-safe GUI operations
#define GUI_THREAD_SAFE(operation) \
    std::lock_guard<std::mutex> gui_lock(g_guiMutex); \
    operation

// =============================================================================
// GLOBAL THREAD SAFETY
// =============================================================================

extern std::mutex g_guiMutex;
extern std::atomic<bool> g_guiInitialized;

// =============================================================================
// SAFETY VALIDATION FUNCTIONS
// =============================================================================

bool isValidWindowHandle(HWND hwnd);
bool isGUIThread();
void validateGUIState();
bool safeSetWindowText(HWND hwnd, const std::string& text);
bool safeCreateWindow(HWND& hwnd, const char* className, const char* windowName, 
                     DWORD style, int x, int y, int width, int height, 
                     HWND parent, HMENU menu, HINSTANCE instance, void* param);

// =============================================================================
// STRUCTURED EXCEPTION HANDLING
// =============================================================================

class GUIException : public std::exception {
private:
    std::string m_message;
    DWORD m_errorCode;
    
public:
    GUIException(const std::string& message, DWORD errorCode = 0) 
        : m_message(message), m_errorCode(errorCode) {}
    
    const char* what() const noexcept override { return m_message.c_str(); }
    DWORD getErrorCode() const { return m_errorCode; }
};

// =============================================================================
// LOGGING SYSTEM
// =============================================================================
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

struct LogEntry {
    std::string message;
    LogLevel level;
    std::chrono::steady_clock::time_point timestamp;
    
    LogEntry(const std::string& msg, LogLevel lvl) 
        : message(msg), level(lvl), timestamp(std::chrono::steady_clock::now()) {}
};

class LogManager {
private:
    std::deque<LogEntry> m_logEntries;
    mutable std::mutex m_logMutex;
    size_t m_maxEntries = 1000;
    LogLevel m_currentLevel = LogLevel::Info;
    bool m_autoScroll = true;
    
public:
    void addEntry(const std::string& message, LogLevel level);
    void clear();
    std::vector<LogEntry> getEntries() const;
    void setMaxEntries(size_t max);
    void setLogLevel(LogLevel level);
    void setAutoScroll(bool enabled);
    bool getAutoScroll() const;
    LogLevel getCurrentLevel() const;
};

extern LogManager g_logManager;

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

// Real-time logging panel controls
extern HWND g_hLoggingFrame;
extern HWND g_hLogDisplay;
extern HWND g_hLogClearButton;
extern HWND g_hLogLevelComboBox;
extern HWND g_hLogAutoScrollToggle;

// Console replacement controls
extern HWND g_hConsoleFrame;
extern HWND g_hConsoleOutput;
extern HWND g_hConsoleInput;
extern HWND g_hConsoleSendButton;

// =============================================================================
// GUI CORE FUNCTIONS
// =============================================================================
bool initializeGUI();
void cleanupGUI();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void runGuiMessageLoop();

// =============================================================================
// GUI UPDATE FUNCTIONS - Thread-safe
// =============================================================================
void updateGUIStatus(const std::string& status);
void updateProfileDisplay(const std::string& profileName);
void updateAnalyticsDisplay(const std::string& analytics);
void updateAudioAlertDisplay(const std::string& alert);
void updateMovementStatusDisplay(const std::string& status);
void updateValidationStatus();

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

// Logging system functions
void addLogEntry(const std::string& message, LogLevel level = LogLevel::Info);
void clearLogDisplay();
void setLogLevel(LogLevel level);
void setAutoScroll(bool enabled);

// Console replacement functions
void addConsoleOutput(const std::string& message);
void processConsoleInput(const std::string& command);
void clearConsoleOutput();

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
void createLoggingControls(HWND hwndParent);
void createConsoleControls(HWND hwndParent);
void updateAllGUIElements();

// =============================================================================
// MESSAGE POSTING FUNCTIONS - For thread-safe GUI updates
// =============================================================================
void postUpdateAnalytics();
void postUpdateAudioAlert(const std::string& alert);
void postUpdateMovementStatus(const std::string& status);
void postUpdateWeaponProfile(const std::string& profileName);
void postUpdateMonitorInfo(int currentMonitor, int totalMonitors, int width, int height);
void postLogEntry(const std::string& message, LogLevel level);
void postConsoleOutput(const std::string& message);

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
void onLogClearClicked();
void onLogLevelChanged();
void onConsoleInputSubmitted();

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

// Performance monitoring functions
void updatePerformanceMetrics(double cpuUsage, size_t memoryMB, size_t threadCount, double responseTime);

// Double buffering structures for GUI optimization
struct DoubleBuffer {
    HDC front_buffer;
    HDC back_buffer;
    HBITMAP back_bitmap;
    HBITMAP old_bitmap;
    int width;
    int height;
    bool is_initialized;
    
    DoubleBuffer() : front_buffer(nullptr), back_buffer(nullptr), 
                     back_bitmap(nullptr), old_bitmap(nullptr),
                     width(0), height(0), is_initialized(false) {}
    
    bool initialize(HWND hwnd, int w, int h);
    void cleanup();
    void swap();
    HDC getBackBuffer() const { return back_buffer; }
    HDC getFrontBuffer() const { return front_buffer; }
};

// GUI rendering optimization
struct GUIRenderOptimizer {
    DoubleBuffer double_buffer;
    std::atomic<bool> needs_redraw{true};
    std::atomic<uint64_t> frame_count{0};
    std::chrono::steady_clock::time_point last_frame_time;
    double target_fps{60.0};
    double current_fps{0.0};
    
    bool initialize(HWND hwnd, int width, int height);
    void cleanup();
    void beginFrame();
    void endFrame();
    void requestRedraw();
    double getCurrentFPS() const;
    void setTargetFPS(double fps);
};