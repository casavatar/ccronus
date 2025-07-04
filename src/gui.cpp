// description: This code implements a GUI for a game assist tool, allowing users to toggle assists, view logs, and manage profiles. It uses Win32 API for the GUI and handles input events.
// It includes a logging mechanism to display messages in a list box and provides functionality to switch weapon profiles. The GUI is designed to be responsive and user-friendly, with real-time updates based on user interactions and system states.
//
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 1.2.0
// date: 2025-06-26
// project: Tactical Aim Assist

#include "gui.h"
#include "globals.h"
#include "input.h"
#include "profiles.h"
#include "systems.h"
#include <string>
#include <deque>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <ctime>
#include <iostream>

// --- GUI Specific Globals & Defines ---
#define ID_LISTBOX 1001
#define ID_STATUS_LABEL 1002
#define ID_PROFILE_LABEL 1003
#define ID_TOGGLE_BUTTON 1004
#define ID_ANALYTICS_LABEL 1005
#define ID_AUDIO_ALERT_LABEL 1006 // New ID for the audio alert label
#define WM_LOG_MESSAGE (WM_USER + 1)

// --- FIX: Declare GUI handles in the global scope of the file ---
HWND hListBox = NULL; // List box for displaying logs
HWND hStatusLabel = NULL; // Status label for the current state of the application
HWND hProfileLabel = NULL; // Profile label for current weapon profile
HWND hToggleButton = NULL; // Toggle button for assists
HWND hAnalyticsLabel = NULL; // Analytics label for performance metrics
HWND hAudioAlertLabel = NULL; // New handle for the audio alert label

std::mutex logMutex;
std::deque<std::string> logQueue;
std::atomic<bool> guiReady(false);
std::atomic<bool> shouldCloseGUI(false);

void logMessage(const std::string& message) {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_buf;
    localtime_s(&tm_buf, &t);

    std::ostringstream oss;
    oss << "[" << std::put_time(&tm_buf, "%H:%M:%S") << "] " << message;

    std::string timestampedMsg = oss.str();

    std::cout << timestampedMsg << std::endl;

    {
        std::lock_guard<std::mutex> lock(logMutex);
        logQueue.push_back(timestampedMsg);
        if (logQueue.size() > 100) {
            logQueue.pop_front();
        }
    }

    if (guiReady.load() && g_hWnd) {
        PostMessage(g_hWnd, WM_LOG_MESSAGE, 0, 0);
    }
}

LRESULT CALLBACK EnhancedWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            RAWINPUTDEVICE Rid[2];
            Rid[0].usUsagePage = 0x01; Rid[0].usUsage = 0x02; Rid[0].dwFlags = RIDEV_INPUTSINK; Rid[0].hwndTarget = hwnd;
            Rid[1].usUsagePage = 0x01; Rid[1].usUsage = 0x06; Rid[1].dwFlags = RIDEV_INPUTSINK; Rid[1].hwndTarget = hwnd;
            if (!RegisterRawInputDevices(Rid, 2, sizeof(Rid[0]))) { logMessage("Failed to register raw input devices!"); }

            // --- Control Creation ---
            hListBox = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 10, 110, 460, 230, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);
            hStatusLabel = CreateWindowW(L"STATIC", L"Status: Running", WS_CHILD | WS_VISIBLE, 10, 10, 200, 20, hwnd, (HMENU)ID_STATUS_LABEL, NULL, NULL);
            hProfileLabel = CreateWindowW(L"STATIC", L"Profile: Loading...", WS_CHILD | WS_VISIBLE, 220, 10, 250, 20, hwnd, (HMENU)ID_PROFILE_LABEL, NULL, NULL);
            hToggleButton = CreateWindowW(L"BUTTON", L"Assists: ON", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 40, 100, 25, hwnd, (HMENU)ID_TOGGLE_BUTTON, NULL, NULL);
            hAnalyticsLabel = CreateWindowW(L"STATIC", L"Analytics: Loading...", WS_CHILD | WS_VISIBLE, 120, 40, 350, 25, hwnd, (HMENU)ID_ANALYTICS_LABEL, NULL, NULL);
            
            // New Audio Alert Label
            hAudioAlertLabel = CreateWindowW(L"STATIC", L"Audio: OK", WS_CHILD | WS_VISIBLE, 10, 75, 460, 25, hwnd, (HMENU)ID_AUDIO_ALERT_LABEL, NULL, NULL);

            // --- Font Setup ---
            HFONT hFont = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS, L"Arial");
            SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hProfileLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hToggleButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hAnalyticsLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hAudioAlertLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            guiReady = true;
            break;
        }
        case WM_INPUT: {
            UINT dwSize;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            std::vector<BYTE> lpb(dwSize);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                RAWINPUT* raw = (RAWINPUT*)lpb.data();
                if (raw->header.dwType == RIM_TYPEMOUSE) HandleRawMouseInput(raw->data.mouse);
                else if (raw->header.dwType == RIM_TYPEKEYBOARD) HandleRawKeyboardInput(raw->data.keyboard);
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TOGGLE_BUTTON) {
                g_assistEnabled = !g_assistEnabled;
                SetWindowTextA(hToggleButton, g_assistEnabled ? "Assists: ON" : "Assists: OFF");
                logMessage(g_assistEnabled ? "Assists Enabled" : "Assists Disabled");
            }
            break;
        case WM_LOG_MESSAGE: {
            std::lock_guard<std::mutex> lock(logMutex);
            while (!logQueue.empty()) {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)logQueue.front().c_str());
                logQueue.pop_front();
            }
            int count = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
            if (count > 0) SendMessage(hListBox, LB_SETTOPINDEX, count - 1, 0);
            while (count > 200) {
                SendMessage(hListBox, LB_DELETESTRING, 0, 0);
                count--;
            }
            break;
        }
        case WM_CLOSE:
            if (MessageBox(hwnd, "Are you sure you want to exit?", "Confirm Exit", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                shouldCloseGUI = true;
                g_running = false;
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void enhancedGuiThread() {
    const wchar_t CLASS_NAME[] = L"TacticalSystemWindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = EnhancedWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    g_hWnd = CreateWindowExW(0, CLASS_NAME, L"Professional Tactical System", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (g_hWnd == NULL) { return; }
    ShowWindow(g_hWnd, SW_SHOW);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0 && !shouldCloseGUI) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    guiReady = false;
}

void updateProfileLabel() {
    if (hProfileLabel && guiReady && !g_weaponProfiles.empty()) {
        std::string profileText = "Profile: " + g_weaponProfiles[g_activeProfileIndex].name;
        SetWindowTextA(hProfileLabel, profileText.c_str());
    }
}

void updateAnalyticsLabel() {
    if (hAnalyticsLabel && guiReady.load() && g_predictiveAim && g_performanceOpt) {
        std::ostringstream analytics;
        analytics << "Pred: " << std::fixed << std::setprecision(0) << (g_predictiveAim->getPredictionConfidence() * 100) << "% | ";
        
        analytics << g_performanceOpt->getPerformanceMetrics();
        
        // Convertir std::string a std::wstring para SetWindowTextW
        std::string s = analytics.str();
        int len;
        int slength = (int)s.length() + 1;
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
        std::wstring wstr(buf);
        delete[] buf;

        SetWindowTextW(hAnalyticsLabel, wstr.c_str());
    }
}

// New function to update the audio alert label
void updateAudioAlertLabel(const std::string& alert_text) {
    if (hAudioAlertLabel && guiReady.load()) {
        SetWindowTextA(hAudioAlertLabel, alert_text.c_str());
    }
}