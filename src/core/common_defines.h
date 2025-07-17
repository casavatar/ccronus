// common_defines.h
// description: Centralized definitions to eliminate duplications across the project
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - Refactored
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

// =============================================================================
// CUSTOM WINDOW MESSAGES - SINGLE SOURCE OF TRUTH
// =============================================================================
#define WM_LOG_MESSAGE                  (WM_USER + 1)
#define WM_UPDATE_ANALYTICS             (WM_USER + 2)
#define WM_UPDATE_AUDIO_ALERT           (WM_USER + 3)
#define WM_UPDATE_MOVEMENT_STATUS       (WM_USER + 4)

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
// DEBUGGING MACROS
// =============================================================================
#ifndef DEBUG_LOG
    #ifdef DEBUG
        #define DEBUG_LOG(msg) // Debug implementation
    #else
        #define DEBUG_LOG(msg) // No-op in release
    #endif
#endif