// event_system.h
// description: Event-driven system for efficient GUI updates and inter-component communication
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.0.0 - New Module
// date: 2025-07-16
// project: Tactical Aim Assist

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <typeindex>
#include <any>
#include <stdexcept>

#include "common_defines.h"

// =============================================================================
// EVENT SYSTEM CORE TYPES
// =============================================================================
enum class EventPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

enum class EventType {
    // System events
    SystemStateChanged,
    ConfigurationChanged,
    
    // Player events
    PlayerMovementChanged,
    WeaponProfileChanged,
    StatsUpdated,
    
    // GUI events
    GUIUpdateRequested,
    WindowResized,
    ControlValueChanged,
    
    // Audio events
    AudioAlertGenerated,
    AudioStateChanged,
    
    // Input events
    KeybindingTriggered,
    MouseActivityDetected,
    
    // Performance events
    PerformanceMetricsUpdated,
    MemoryUsageChanged,
    
    // Custom events
    Custom
};

struct BaseEvent {
    EventType type;
    EventPriority priority;
    std::chrono::steady_clock::time_point timestamp;
    std::string source_component;
    std::any data;
    bool consumed = false;
    
    BaseEvent(EventType t, EventPriority p = EventPriority::Normal, 
              const std::string& source = "")
        : type(t), priority(p), timestamp(std::chrono::steady_clock::now()), 
          source_component(source) {}
    
    template<typename T>
    void setData(const T& value) {
        data = value;
    }
    
    template<typename T>
    T getData() const {
        try {
            return std::any_cast<T>(data);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid event data type requested");
        }
    }
    
    template<typename T>
    bool hasDataType() const {
        return std::type_index(data.type()) == std::type_index(typeid(T));
    }
};

// =============================================================================
// SPECIALIZED EVENT STRUCTURES
// =============================================================================
struct GUIUpdateEvent {
    std::string component_name;
    std::unordered_map<std::string, std::string> updated_values;
    bool force_refresh = false;
    
    GUIUpdateEvent(const std::string& component = "") 
        : component_name(component) {}
};

struct StateChangeEvent {
    std::string state_category;
    std::string old_value;
    std::string new_value;
    std::chrono::steady_clock::time_point change_time;
    
    StateChangeEvent(const std::string& category, const std::string& old_val, const std::string& new_val)
        : state_category(category), old_value(old_val), new_value(new_val),
          change_time(std::chrono::steady_clock::now()) {}
};

struct PerformanceMetricsEvent {
    double cpu_usage;
    size_t memory_usage_mb;
    uint32_t fps;
    std::chrono::milliseconds response_time;
    
    PerformanceMetricsEvent(double cpu = 0.0, size_t mem = 0, uint32_t f = 0, 
                           std::chrono::milliseconds rt = std::chrono::milliseconds(0))
        : cpu_usage(cpu), memory_usage_mb(mem), fps(f), response_time(rt) {}
};

// =============================================================================
// EVENT HANDLER TYPES
// =============================================================================
using EventHandler = std::function<void(const BaseEvent&)>;
using EventFilter = std::function<bool(const BaseEvent&)>;

// =============================================================================
// EVENT SYSTEM CLASS
// =============================================================================
class EventSystem {
public:
    // Constructor/Destructor
    EventSystem();
    ~EventSystem();
    
    // Core functionality
    void initialize();
    void shutdown();
    bool isRunning() const;
    
    // Event publishing
    void publishEvent(const BaseEvent& event);
    void publishEvent(EventType type, EventPriority priority = EventPriority::Normal,
                     const std::string& source = "");
    
    template<typename T>
    void publishEventWithData(EventType type, const T& data, 
                             EventPriority priority = EventPriority::Normal,
                             const std::string& source = "");
    
    // Specialized event publishers
    void publishGUIUpdate(const std::string& component, 
                         const std::unordered_map<std::string, std::string>& values,
                         bool force_refresh = false);
    void publishStateChange(const std::string& category, const std::string& old_value, 
                           const std::string& new_value);
    void publishPerformanceMetrics(const PerformanceMetricsEvent& metrics);
    
    // Event subscription
    void subscribe(EventType type, const std::string& handler_name, EventHandler handler);
    void subscribe(EventType type, const std::string& handler_name, EventHandler handler, EventFilter filter);
    void unsubscribe(EventType type, const std::string& handler_name);
    void unsubscribeAll(const std::string& handler_name);
    
    // Batch operations
    void publishBatch(const std::vector<BaseEvent>& events);
    std::vector<BaseEvent> consumePendingEvents(EventType type);
    
    // Queue management
    size_t getQueueSize() const;
    size_t getQueueSize(EventType type) const;
    void clearQueue();
    void clearQueue(EventType type);
    
    // Performance and diagnostics
    std::vector<std::string> getDiagnosticInfo() const;
    void setMaxQueueSize(size_t max_size);
    void setEventTimeoutMs(uint32_t timeout_ms);
    
    // Priority processing
    void setHighPriorityMode(bool enable);
    bool isHighPriorityMode() const;
    
    // Thread control
    void pauseProcessing();
    void resumeProcessing();
    bool isProcessingPaused() const;
    
private:
    // Core state
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_high_priority_mode{false};
    
    // Configuration
    size_t m_max_queue_size = 1000;
    uint32_t m_event_timeout_ms = 5000;
    
    // Event queue and processing
    std::priority_queue<std::shared_ptr<BaseEvent>, 
                       std::vector<std::shared_ptr<BaseEvent>>,
                       std::function<bool(const std::shared_ptr<BaseEvent>&, 
                                        const std::shared_ptr<BaseEvent>&)>> m_event_queue;
    mutable std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    
    // Event handlers
    std::unordered_map<EventType, std::unordered_map<std::string, std::pair<EventHandler, EventFilter>>> m_handlers;
    mutable std::shared_mutex m_handlers_mutex;
    
    // Processing thread
    std::unique_ptr<std::thread> m_processing_thread;
    std::atomic<bool> m_should_stop{false};
    
    // Statistics
    mutable std::mutex m_stats_mutex;
    std::unordered_map<EventType, uint64_t> m_event_counts;
    std::unordered_map<EventType, std::chrono::milliseconds> m_processing_times;
    
    // Helper methods
    void processingLoop();
    void processEvent(std::shared_ptr<BaseEvent> event);
    bool isEventExpired(const BaseEvent& event) const;
    void updateStatistics(EventType type, std::chrono::milliseconds processing_time);
    void cleanupExpiredEvents();
    
    // Priority queue comparator
    static bool eventPriorityComparator(const std::shared_ptr<BaseEvent>& a, 
                                       const std::shared_ptr<BaseEvent>& b);
};

// =============================================================================
// TEMPLATE IMPLEMENTATIONS
// =============================================================================
template<typename T>
void EventSystem::publishEventWithData(EventType type, const T& data, 
                                       EventPriority priority, const std::string& source) {
    BaseEvent event(type, priority, source);
    event.setData(data);
    publishEvent(event);
}

// =============================================================================
// GLOBAL EVENT SYSTEM INSTANCE
// =============================================================================
extern std::unique_ptr<EventSystem> g_eventSystem;

// =============================================================================
// CONVENIENCE MACROS
// =============================================================================
#define EVENT_SYS() (g_eventSystem.get())
#define PUBLISH_EVENT(type, priority, source) if(EVENT_SYS()) EVENT_SYS()->publishEvent(type, priority, source)
#define PUBLISH_EVENT_DATA(type, data, priority, source) if(EVENT_SYS()) EVENT_SYS()->publishEventWithData(type, data, priority, source)
#define SUBSCRIBE_EVENT(type, name, handler) if(EVENT_SYS()) EVENT_SYS()->subscribe(type, name, handler)
#define UNSUBSCRIBE_EVENT(type, name) if(EVENT_SYS()) EVENT_SYS()->unsubscribe(type, name)

// =============================================================================
// HELPER FUNCTIONS FOR COMMON EVENT PATTERNS
// =============================================================================
namespace EventHelpers {
    // GUI update helpers
    void requestGUIRefresh(const std::string& component = "");
    void notifyControlValueChanged(const std::string& control_name, const std::string& new_value);
    
    // State change helpers
    void notifyMovementStateChanged(const std::string& old_state, const std::string& new_state);
    void notifyWeaponProfileChanged(int old_index, int new_index);
    void notifySystemStateChanged(const std::string& old_state, const std::string& new_state);
    
    // Performance helpers
    void reportPerformanceMetrics(double cpu_usage, size_t memory_mb, uint32_t fps);
    void reportResponseTime(std::chrono::milliseconds response_time);
    
    // Audio helpers
    void reportAudioAlert(const std::string& alert_message, const std::string& alert_type);
    
    // Input helpers
    void reportKeybindingTriggered(const std::string& keybinding_name);
}