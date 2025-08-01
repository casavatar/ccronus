// common_defines.h
// description: Centralized definitions to eliminate duplications across the project
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 4.0.0 - Enhanced with new GUI control IDs and features
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

// =============================================================================
// GUI CONTROL IDs - SINGLE SOURCE OF TRUTH
// =============================================================================
#define ID_LISTBOX                      1001
#define ID_STATUS_LABEL                 1002
#define ID_PROFILE_LABEL                1003
#define ID_TOGGLE_BUTTON                1004
#define ID_ANALYTICS_LABEL              1005
#define ID_AUDIO_ALERT_LABEL            1006
#define ID_MOVEMENT_STATUS_LABEL        1007
#define ID_PROFILE_COMBOBOX             1008

// Weapon Profile UI Control IDs
#define ID_WEAPON_PROFILE_FRAME         2001
#define ID_ACTIVE_PROFILE_LABEL         2002
#define ID_PROFILE_ENHANCEMENTS_FRAME   2003
#define ID_RECOIL_CONTROL_TOGGLE        2004
#define ID_AIM_ASSIST_TOGGLE            2005
#define ID_SPREAD_CONTROL_TOGGLE        2006
#define ID_PROFILE_STATS_LABEL          2007

// Multi-Monitor UI Control IDs
#define ID_MONITOR_INFO_FRAME           3001
#define ID_CURRENT_MONITOR_LABEL        3002
#define ID_MONITOR_COUNT_LABEL          3003
#define ID_MONITOR_RESOLUTION_LABEL     3004
#define ID_MONITOR_SWITCH_BUTTON        3005

// Movement System UI Control IDs
#define ID_MOVEMENT_FRAME               4001
#define ID_SLIDE_KEY_LABEL              4002
#define ID_MOVEMENT_STATE_LABEL         4003
#define ID_MOVEMENT_TOGGLE_BUTTON       4004

// =============================================================================
// CUSTOM WINDOW MESSAGES - SINGLE SOURCE OF TRUTH
// =============================================================================
#define WM_LOG_MESSAGE                  (WM_USER + 1)
#define WM_UPDATE_ANALYTICS             (WM_USER + 2)
#define WM_UPDATE_AUDIO_ALERT           (WM_USER + 3)
#define WM_UPDATE_MOVEMENT_STATUS       (WM_USER + 4)
#define WM_UPDATE_WEAPON_PROFILE        (WM_USER + 5)
#define WM_UPDATE_MONITOR_INFO          (WM_USER + 6)
#define WM_UPDATE_SLIDE_STATUS          (WM_USER + 7)

// =============================================================================
// MATHEMATICAL CONSTANTS
// =============================================================================
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// SYSTEM CONSTANTS
// =============================================================================
#define DEFAULT_MONITOR_WIDTH           1920
#define DEFAULT_MONITOR_HEIGHT          1080
#define DEFAULT_SAMPLE_RATE             44100
#define DEFAULT_BUFFER_SIZE             512

// =============================================================================
// TIMING CONSTANTS (in milliseconds)
// =============================================================================
#define GUI_UPDATE_INTERVAL_MS          100
#define AUDIO_POLL_INTERVAL_MS          10
#define MAIN_LOOP_SLEEP_MS              10
#define ANALYTICS_UPDATE_INTERVAL_MS    100

// =============================================================================
// PERFORMANCE THRESHOLDS
// =============================================================================
#define MAX_PID_ERROR_THRESHOLD         100.0
#define MIN_PREDICTION_CONFIDENCE       0.30
#define DEFAULT_SMOOTHING_FACTOR        0.85

// =============================================================================
// MOVEMENT SYSTEM CONSTANTS
// =============================================================================
#define MOVEMENT_SYSTEM_ENABLED         true

// =============================================================================
// WEAPON PROFILE ENHANCEMENT FLAGS
// =============================================================================
#define ENHANCEMENT_RECOIL_CONTROL      0x01
#define ENHANCEMENT_AIM_ASSIST          0x02
#define ENHANCEMENT_SPREAD_CONTROL      0x04

// =============================================================================
// DEBUGGING MACROS
// =============================================================================
#ifndef DEBUG_LOG
    #ifdef DEBUG
        #define DEBUG_LOG(msg) // Debug implementation
    #else
        #define DEBUG_LOG(msg) // No-op in release
    #endif
#endif