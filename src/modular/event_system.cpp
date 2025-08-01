// event_system.cpp - CORRECTED VERSION MATCHING HEADER v3.1.3
// --------------------------------------------------------------------------------------
// description: Event system implementation - Matches header API exactly
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// license: GNU General Public License v3.0
// version: 3.1.3 - Corrected to match header API
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "event_system.h"
#include "globals.h"
#include "core/config.h"

// =============================================================================
// RESOLVE WINDOWS.H CONFLICTS
// =============================================================================
#ifdef ERROR
#undef ERROR  // Undefine Windows ERROR macro that conflicts with enum
#endif

// =============================================================================
// GLOBAL EVENT SYSTEM INSTANCE
// =============================================================================
std::unique_ptr<EventSystem> g_eventSystem;

// =============================================================================
// EVENT SYSTEM IMPLEMENTATION
// =============================================================================
EventSystem::EventSystem() 
    : m_event_queue(EventPriorityComparator()) {
    logDebug("EventSystem constructor called");
}

EventSystem::~EventSystem() {
    shutdown();
}

void EventSystem::initialize() {
    logMessage("🎯 Initializing Event System...");
    
    if (m_initialized.load()) {
        logMessage("Event System already initialized");
        return;
    }
    
    // Clear statistics
    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_event_counts.clear();
        m_processing_times.clear();
    }
    
    // Start processing thread
    m_running.store(true);
    m_should_stop.store(false);
    m_processing_thread = std::make_unique<std::thread>(&EventSystem::processingLoop, this);
    
    m_initialized.store(true);
    logMessage("✅ Event System initialized successfully");
}

void EventSystem::shutdown() {
    if (!m_initialized.load()) return;
    
    logMessage("🛑 Shutting down Event System...");
    
    // Stop processing
    m_running.store(false);
    m_should_stop.store(true);
    m_paused.store(false);
    
    // Wake up processing thread
    m_queue_cv.notify_all();
    
    // Wait for processing thread to finish
    if (m_processing_thread && m_processing_thread->joinable()) {
        m_processing_thread->join();
    }
    m_processing_thread.reset();
    
    // Clear handlers and queue
    {
        std::lock_guard<std::shared_mutex> lock(m_handlers_mutex);
        m_handlers.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        while (!m_event_queue.empty()) {
            m_event_queue.pop();
        }
    }
    
    m_initialized.store(false);
    logMessage("✅ Event System shutdown complete");
}

bool EventSystem::isRunning() const {
    return m_running.load() && m_initialized.load();
}

void EventSystem::publishEvent(EventType type, EventPriority priority, const std::string& source) {
    BaseEvent event(type, priority, source);
    publishEvent(std::make_shared<BaseEvent>(event));
}

void EventSystem::publishEvent(std::shared_ptr<BaseEvent> event) {
    if (!event) return;
    
    if (m_use_lockfree_queue.load()) {
        // Use lock-free queue
        m_lockfree_event_queue.push(event);
    } else {
        // Fallback to legacy queue
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_event_queue.push(event);
        m_queue_cv.notify_one();
    }
}

void EventSystem::publishEvent(const BaseEvent& event) {
    publishEvent(std::make_shared<BaseEvent>(event));
}

void EventSystem::publishGUIUpdate(const std::string& component, 
                                  const std::unordered_map<std::string, std::string>& values,
                                  bool force_refresh) {
    GUIUpdateEvent guiData(component);
    guiData.updated_values = values;
    guiData.force_refresh = force_refresh;
    
    BaseEvent event(EventType::GUIUpdateRequested, EventPriority::Normal, "GUISystem");
    event.setData(guiData);
    publishEvent(std::make_shared<BaseEvent>(event));
}

void EventSystem::publishStateChange(const std::string& category, const std::string& old_value, 
                                    const std::string& new_value) {
    StateChangeEvent stateData(category, old_value, new_value);
    
    BaseEvent event(EventType::SystemStateChanged, EventPriority::High, "StateManager");
    event.setData(stateData);
    publishEvent(std::make_shared<BaseEvent>(event));
}

void EventSystem::publishPerformanceMetrics(const PerformanceMetricsEvent& metrics) {
    BaseEvent event(EventType::PerformanceMetricsUpdated, EventPriority::Low, "PerformanceMonitor");
    event.setData(metrics);
    publishEvent(std::make_shared<BaseEvent>(event));
}

void EventSystem::subscribe(EventType type, const std::string& handler_name, EventHandler handler) {
    subscribe(type, handler_name, handler, nullptr);
}

void EventSystem::subscribe(EventType type, const std::string& handler_name, 
                           EventHandler handler, EventFilter filter) {
    std::lock_guard<std::shared_mutex> lock(m_handlers_mutex);
    
    // Remove existing handler with same name if it exists
    auto& handlerMap = m_handlers[type];
    handlerMap.erase(handler_name);
    
    // Add new handler
    handlerMap[handler_name] = std::make_pair(handler, filter);
    
    logDebug("Subscribed '" + handler_name + "' to event type " + std::to_string(static_cast<int>(type)));
}

void EventSystem::unsubscribe(EventType type, const std::string& handler_name) {
    std::lock_guard<std::shared_mutex> lock(m_handlers_mutex);
    
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        auto& handlerMap = it->second;
        auto removed = handlerMap.erase(handler_name);
        
        if (removed > 0) {
            logDebug("Unsubscribed '" + handler_name + "' from event type " + std::to_string(static_cast<int>(type)));
        }
    }
}

void EventSystem::unsubscribeAll(const std::string& handler_name) {
    std::lock_guard<std::shared_mutex> lock(m_handlers_mutex);
    
    size_t totalRemoved = 0;
    
    for (auto& [type, handlerMap] : m_handlers) {
        totalRemoved += handlerMap.erase(handler_name);
    }
    
    if (totalRemoved > 0) {
        logDebug("Unsubscribed '" + handler_name + "' from " + std::to_string(totalRemoved) + " event handlers");
    }
}

void EventSystem::publishBatch(const std::vector<BaseEvent>& events) {
    for (const auto& event : events) {
        publishEvent(std::make_shared<BaseEvent>(event));
    }
}

std::vector<BaseEvent> EventSystem::consumePendingEvents(EventType type) {
    std::vector<BaseEvent> events;
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    
    // Note: This is a simplified implementation
    // A full implementation would need to extract specific event types from the priority queue
    logDebug("consumePendingEvents called for type: " + std::to_string(static_cast<int>(type)));
    
    return events;
}

size_t EventSystem::getQueueSize() const {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return m_event_queue.size();
}

size_t EventSystem::getQueueSize(EventType type) const {
    // Note: This is a simplified implementation
    // A full implementation would count specific event types in the queue
    logDebug("getQueueSize called for type: " + std::to_string(static_cast<int>(type)));
    return 0;
}

void EventSystem::clearQueue() {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    while (!m_event_queue.empty()) {
        m_event_queue.pop();
    }
    logDebug("Event queue cleared");
}

void EventSystem::clearQueue(EventType type) {
    // Note: This is a simplified implementation
    // A full implementation would remove only specific event types
    logDebug("clearQueue called for type: " + std::to_string(static_cast<int>(type)));
}

std::vector<std::string> EventSystem::getDiagnosticInfo() const {
    std::vector<std::string> info;
    
    {
        std::shared_lock<std::shared_mutex> handlers_lock(m_handlers_mutex);
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        
        info.push_back("=== Event System Diagnostics ===");
        info.push_back("Status: " + std::string(isRunning() ? "Running" : "Stopped"));
        info.push_back("Initialized: " + std::string(m_initialized.load() ? "Yes" : "No"));
        info.push_back("Paused: " + std::string(m_paused.load() ? "Yes" : "No"));
        info.push_back("High Priority Mode: " + std::string(m_high_priority_mode.load() ? "Yes" : "No"));
        info.push_back("Queue Size: " + std::to_string(getQueueSize()) + "/" + std::to_string(m_max_queue_size));
        info.push_back("Event Timeout: " + std::to_string(m_event_timeout_ms) + "ms");
        
        info.push_back("\n=== Event Handlers ===");
        for (const auto& [type, handlerMap] : m_handlers) {
            std::string type_name = "EventType_" + std::to_string(static_cast<int>(type));
            info.push_back(type_name + ": " + std::to_string(handlerMap.size()) + " handlers");
            
            for (const auto& [handlerName, handlerPair] : handlerMap) {
                info.push_back("  - " + handlerName);
            }
        }
        
        info.push_back("\n=== Event Statistics ===");
        for (const auto& [type, count] : m_event_counts) {
            std::string type_name = "EventType_" + std::to_string(static_cast<int>(type));
            info.push_back(type_name + ": " + std::to_string(count) + " events processed");
        }
    }
    
    return info;
}

void EventSystem::setMaxQueueSize(size_t max_size) {
    m_max_queue_size = max_size;
    logDebug("Max queue size set to: " + std::to_string(max_size));
}

void EventSystem::setEventTimeoutMs(uint32_t timeout_ms) {
    m_event_timeout_ms = timeout_ms;
    logDebug("Event timeout set to: " + std::to_string(timeout_ms) + "ms");
}

void EventSystem::setHighPriorityMode(bool enable) {
    m_high_priority_mode.store(enable);
    logDebug("High priority mode: " + std::string(enable ? "enabled" : "disabled"));
}

bool EventSystem::isHighPriorityMode() const {
    return m_high_priority_mode.load();
}

void EventSystem::pauseProcessing() {
    m_paused.store(true);
    logDebug("Event processing paused");
}

void EventSystem::resumeProcessing() {
    m_paused.store(false);
    m_queue_cv.notify_all();
    logDebug("Event processing resumed");
}

bool EventSystem::isProcessingPaused() const {
    return m_paused.load();
}

void EventSystem::processingLoop() {
    logDebug("Event processing loop started");
    
    // Pre-allocate event vector to avoid repeated allocations
    std::vector<std::shared_ptr<BaseEvent>> events_to_process;
    events_to_process.reserve(100); // Reserve space for batch processing
    
    while (!m_should_stop.load()) {
        if (m_use_lockfree_queue.load()) {
            // Process lock-free queue
            processLockFreeQueue(events_to_process);
        } else {
            // Process legacy queue
            processLegacyQueue(events_to_process);
        }
        
        // Process events in batch
        for (auto& event : events_to_process) {
            if (!event) continue;
            
            // Check if event is expired
            if (isEventExpired(*event)) {
                logDebug("Skipping expired event");
                continue;
            }
            
            auto start_time = std::chrono::high_resolution_clock::now();
            
            processEvent(event);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Update statistics
            updateStatistics(event->type, processing_time);
        }
        
        // Clear the batch for next iteration
        events_to_process.clear();
    }
    
    logDebug("Event processing loop ended");
}

void EventSystem::processLockFreeQueue(std::vector<std::shared_ptr<BaseEvent>>& events_to_process) {
    // Check if processing is paused
    if (m_paused.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }
    
    // Process up to 10 events from lock-free queue
    size_t batch_size = 0;
    std::shared_ptr<BaseEvent> event;
    
    while (batch_size < 10 && m_lockfree_event_queue.pop(event)) {
        events_to_process.push_back(event);
        batch_size++;
    }
    
    // Small sleep if no events to prevent busy waiting
    if (batch_size == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void EventSystem::processLegacyQueue(std::vector<std::shared_ptr<BaseEvent>>& events_to_process) {
    // Batch process events to reduce lock contention
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        
        // Wait for events with timeout to prevent indefinite blocking
        if (!m_queue_cv.wait_for(lock, std::chrono::milliseconds(50), [this] {
            return !m_event_queue.empty() || m_should_stop.load();
        })) {
            // Timeout occurred, continue loop
            return;
        }
        
        if (m_should_stop.load()) return;
        
        // Check if processing is paused
        if (m_paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
        
        // Batch collect events (up to 10 at a time)
        size_t batch_size = 0;
        while (!m_event_queue.empty() && batch_size < 10) {
            events_to_process.push_back(m_event_queue.top());
            m_event_queue.pop();
            batch_size++;
        }
    }
}

void EventSystem::processEvent(std::shared_ptr<BaseEvent> event) {
    if (!event) return;
    
    std::shared_lock<std::shared_mutex> lock(m_handlers_mutex);
    
    auto it = m_handlers.find(event->type);
    if (it == m_handlers.end()) {
        if (g_debugMode.load()) {
            logDebug("No handlers for event type: " + std::to_string(static_cast<int>(event->type)));
        }
        return;
    }
    
    const auto& handlerMap = it->second;
    
    for (const auto& [handlerName, handlerPair] : handlerMap) {
        const auto& [handler, filter] = handlerPair;
        
        // Apply custom filter if present
        if (filter && !filter(*event)) continue;
        
        // Execute handler
        if (handler) {
            handler(*event);
        } else {
            logMessage("ERROR: Null handler for '" + handlerName + "'");
        }
        
        // If event is consumed, stop processing
        if (event->consumed) break;
    }
}

bool EventSystem::isEventExpired(const BaseEvent& event) const {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - event.timestamp);
    return age.count() > static_cast<int64_t>(m_event_timeout_ms);
}

void EventSystem::updateStatistics(EventType type, std::chrono::milliseconds processing_time) {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    m_event_counts[type]++;
    m_processing_times[type] = processing_time;
}

void EventSystem::cleanupExpiredEvents() {
    // Note: This would require a more complex queue implementation
    // to efficiently remove expired events from the middle of the queue
    logDebug("cleanupExpiredEvents called");
}

bool EventSystem::eventPriorityComparator(const std::shared_ptr<BaseEvent>& a, 
                                         const std::shared_ptr<BaseEvent>& b) {
    // Higher priority events should be processed first
    // Note: priority_queue is a max-heap, but we want higher priority (larger number) first
    // So we return true if a has lower priority than b (so b comes first)
    if (a->priority != b->priority) {
        return static_cast<int>(a->priority) < static_cast<int>(b->priority);
    }
    
    // If priorities are equal, older events come first
    return a->timestamp > b->timestamp;
}

// =============================================================================
// GLOBAL FUNCTIONS IMPLEMENTATION
// =============================================================================
bool initializeEventSystem() {
    logMessage("🎯 Initializing Global Event System...");
    
    if (g_eventSystem) {
        logMessage("Event System already exists");
        return g_eventSystem->isRunning();
    }
    
    g_eventSystem = std::make_unique<EventSystem>();
    if (!g_eventSystem) {
        logMessage("❌ Failed to create EventSystem");
        return false;
    }
    
    g_eventSystem->initialize();
    
    logMessage("✅ Global Event System initialized successfully");
    return true;
}

void shutdownEventSystem() {
    logMessage("🛑 Shutting down Global Event System...");
    
    if (g_eventSystem) {
        g_eventSystem->shutdown();
        g_eventSystem.reset();
    }
    
    logMessage("✅ Global Event System shutdown complete");
}

EventSystem* getEventSystem() {
    return g_eventSystem.get();
}

// =============================================================================
// HELPER FUNCTIONS IMPLEMENTATION
// =============================================================================
namespace EventHelpers {
    
    void requestGUIRefresh(const std::string& component) {
        if (EVENT_SYS()) {
            std::unordered_map<std::string, std::string> empty_values;
            EVENT_SYS()->publishGUIUpdate(component, empty_values, true);
        }
    }
    
    void notifyControlValueChanged(const std::string& control_name, const std::string& new_value) {
        if (EVENT_SYS()) {
            std::unordered_map<std::string, std::string> values;
            values[control_name] = new_value;
            EVENT_SYS()->publishGUIUpdate("", values, false);
        }
    }
    
    void notifyMovementStateChanged(const std::string& old_state, const std::string& new_state) {
        if (EVENT_SYS()) {
            EVENT_SYS()->publishStateChange("movement", old_state, new_state);
        }
    }
    
    void notifyWeaponProfileChanged(int old_index, int new_index) {
        if (EVENT_SYS()) {
            EVENT_SYS()->publishStateChange("weapon_profile", 
                                           std::to_string(old_index), 
                                           std::to_string(new_index));
        }
    }
    
    void notifySystemStateChanged(const std::string& old_state, const std::string& new_state) {
        if (EVENT_SYS()) {
            EVENT_SYS()->publishStateChange("system", old_state, new_state);
        }
    }
    
    void reportPerformanceMetrics(double cpu_usage, size_t memory_mb, uint32_t fps) {
        if (EVENT_SYS()) {
            PerformanceMetricsEvent metrics(cpu_usage, memory_mb, fps);
            EVENT_SYS()->publishPerformanceMetrics(metrics);
        }
    }
    
    void reportResponseTime(std::chrono::milliseconds response_time) {
        if (EVENT_SYS()) {
            PerformanceMetricsEvent metrics(0.0, 0, 0, response_time);
            EVENT_SYS()->publishPerformanceMetrics(metrics);
        }
    }
    
    void reportAudioAlert(const std::string& alert_message, const std::string& alert_type) {
        if (EVENT_SYS()) {
            BaseEvent event(EventType::AudioAlertGenerated, EventPriority::High, "AudioSystem");
            
            std::unordered_map<std::string, std::string> alertData;
            alertData["message"] = alert_message;
            alertData["type"] = alert_type;
            event.setData(alertData);
            
            EVENT_SYS()->publishEvent(event);
        }
    }
    
    void reportKeybindingTriggered(const std::string& keybinding_name) {
        if (EVENT_SYS()) {
            BaseEvent event(EventType::KeybindingTriggered, EventPriority::Normal, "InputSystem");
            event.setData(keybinding_name);
            EVENT_SYS()->publishEvent(event);
        }
    }
}

// =============================================================================
// CONVENIENCE PUBLISHING FUNCTIONS
// =============================================================================
void publishGameStateChange(int newState, int previousState, const std::string& source) {
    EventHelpers::notifySystemStateChanged(std::to_string(previousState), std::to_string(newState));
    logDebug("Published game state change: " + std::to_string(newState) + 
             " (prev: " + std::to_string(previousState) + ") from " + source);
}

void publishPlayerAction(int action, bool starting, const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::PlayerMovementChanged, EventPriority::Normal, source);
        
        std::unordered_map<std::string, std::string> actionData;
        actionData["action"] = std::to_string(action);
        actionData["starting"] = starting ? "true" : "false";
        event.setData(actionData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logDebug("Published player action: " + std::to_string(action) + (starting ? " (start)" : " (stop)") + " from " + source);
}

void publishTargetAcquired(int x, int y, double confidence, const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::Custom, EventPriority::High, source);
        
        std::unordered_map<std::string, std::string> targetData;
        targetData["x"] = std::to_string(x);
        targetData["y"] = std::to_string(y);
        targetData["confidence"] = std::to_string(confidence);
        targetData["type"] = "target_acquired";
        event.setData(targetData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logDebug("Published target acquired at (" + std::to_string(x) + ", " + std::to_string(y) + 
            ") with confidence " + std::to_string(confidence) + " from " + source);
}

void publishTargetLost(const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::Custom, EventPriority::High, source);
        
        std::unordered_map<std::string, std::string> targetData;
        targetData["type"] = "target_lost";
        event.setData(targetData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logDebug("Published target lost from " + source);
}

void publishMovementChange(int movement, double velocity, const std::string& source) {
    EventHelpers::notifyMovementStateChanged("moving", "movement_" + std::to_string(movement));
    logDebug("Published movement change: " + std::to_string(movement) + 
             " with velocity " + std::to_string(velocity) + " from " + source);
}

void publishAudioDetected(double volume, double frequency, const std::string& direction, 
                         const std::string& type, const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::AudioStateChanged, EventPriority::High, source);
        
        std::unordered_map<std::string, std::string> audioData;
        audioData["volume"] = std::to_string(volume);
        audioData["frequency"] = std::to_string(frequency);
        audioData["direction"] = direction;
        audioData["type"] = type;
        event.setData(audioData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logDebug("Published audio detected: " + type + " from " + direction + 
             " (vol: " + std::to_string(volume) + ", freq: " + std::to_string(frequency) + ") from " + source);
}

void publishSystemError(const std::string& message, const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::Custom, EventPriority::Critical, source);
        
        std::unordered_map<std::string, std::string> errorData;
        errorData["type"] = "system_error";
        errorData["message"] = message;
        event.setData(errorData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logMessage("System Error: " + message + " (from " + source + ")");
}

void publishSystemWarning(const std::string& message, const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::Custom, EventPriority::Normal, source);
        
        std::unordered_map<std::string, std::string> warningData;
        warningData["type"] = "system_warning";
        warningData["message"] = message;
        event.setData(warningData);
        
        EVENT_SYS()->publishEvent(event);
    }
    logMessage("System Warning: " + message + " (from " + source + ")");
}

void publishPerformanceUpdate(const std::unordered_map<std::string, std::string>& metrics, 
                             const std::string& source) {
    if (EVENT_SYS()) {
        BaseEvent event(EventType::PerformanceMetricsUpdated, EventPriority::Low, source);
        event.setData(metrics);
        EVENT_SYS()->publishEvent(event);
    }
    logDebug("Published performance update with " + std::to_string(metrics.size()) + " metrics from " + source);
}

// =============================================================================
// CONVENIENCE SUBSCRIPTION FUNCTIONS  
// =============================================================================
void subscribeToGameEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::SystemStateChanged, handler_name, handler);
}

void subscribeToPlayerEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::PlayerMovementChanged, handler_name, handler);
}

void subscribeToTargetEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::Custom, handler_name, handler);
}

void subscribeToMovementEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::PlayerMovementChanged, handler_name, handler);
}

void subscribeToAudioEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::AudioStateChanged, handler_name, handler);
    SUBSCRIBE_EVENT(EventType::AudioAlertGenerated, handler_name, handler);
}

void subscribeToSystemEvents(const std::string& handler_name, const EventHandler& handler) {
    SUBSCRIBE_EVENT(EventType::SystemStateChanged, handler_name, handler);
    SUBSCRIBE_EVENT(EventType::PerformanceMetricsUpdated, handler_name, handler);
}

// =============================================================================
// TESTING FUNCTIONS
// =============================================================================
void testEventSystem() {
    logMessage("🧪 Testing Event System...");
    
    if (!EVENT_SYS() || !EVENT_SYS()->isRunning()) {
        logMessage("❌ Event System not running");
        return;
    }
    
    // Test event handler
    EVENT_SYS()->subscribe(EventType::Custom, "test_handler", 
        [](const BaseEvent& event) {
            logMessage("✅ Test event received: " + std::to_string(static_cast<int>(event.type)));
        });
    
    // Publish test event
    PUBLISH_EVENT(EventType::Custom, EventPriority::Normal, "test_system");
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test various event types
    publishGameStateChange(1, 0, "test");
    publishPlayerAction(1, true, "test");
    publishTargetAcquired(100, 200, 0.85, "test");
    publishSystemWarning("Test warning message", "test");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Print diagnostics
    auto diagnostics = EVENT_SYS()->getDiagnosticInfo();
    for (const auto& line : diagnostics) {
        logMessage(line);
    }
    
    // Cleanup test handler
    EVENT_SYS()->unsubscribe(EventType::Custom, "test_handler");
    
    logMessage("✅ Event System test completed");
}

void printEventSystemStatus() {
    if (!EVENT_SYS()) {
        logMessage("Event System: Not initialized");
        return;
    }
    
    auto diagnostics = EVENT_SYS()->getDiagnosticInfo();
    for (const auto& line : diagnostics) {
        logMessage(line);
    }
}

void EventSystem::setUseLockFreeQueue(bool use_lockfree) {
    m_use_lockfree_queue.store(use_lockfree);
    logMessage("Event system switched to " + std::string(use_lockfree ? "lock-free" : "legacy") + " queue");
}

bool EventSystem::isUsingLockFreeQueue() const {
    return m_use_lockfree_queue.load();
}