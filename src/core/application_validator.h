// application_validator.h - APPLICATION VALIDATION SYSTEM v1.0.0
// ----------------------------------------------------------------------------------------------------
// description: Comprehensive validation system to prevent unexpected application shutdowns.
//              Provides exception handling, system health monitoring, and graceful error recovery.
// ----------------------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 1.0.0 - Initial implementation
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// ----------------------------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

// Forward declarations
class ApplicationValidator;
class HealthMonitor;
class ExceptionHandler;

// =============================================================================
// VALIDATION CONSTANTS AND TYPES
// =============================================================================

enum class ValidationLevel {
    Critical = 0,    // Application must shutdown
    Error = 1,       // Show error dialog, attempt recovery
    Warning = 2,     // Show warning, continue operation
    Info = 3         // Log information only
};

enum class RecoveryAction {
    None = 0,
    Restart = 1,
    Reinitialize = 2,
    Reset = 3,
    Shutdown = 4
};

struct ValidationResult {
    bool isValid;
    ValidationLevel level;
    std::string message;
    RecoveryAction recoveryAction;
    std::chrono::steady_clock::time_point timestamp;
    
    ValidationResult(bool valid = true, ValidationLevel lvl = ValidationLevel::Info, 
                    const std::string& msg = "", RecoveryAction action = RecoveryAction::None)
        : isValid(valid), level(lvl), message(msg), recoveryAction(action), 
          timestamp(std::chrono::steady_clock::now()) {}
};

struct SystemHealth {
    bool guiSystemHealthy;
    bool inputSystemHealthy;
    bool eventSystemHealthy;
    bool stateManagerHealthy;
    bool memoryUsageNormal;
    bool threadCountNormal;
    std::chrono::steady_clock::time_point lastCheck;
    
    SystemHealth() : guiSystemHealthy(true), inputSystemHealthy(true), 
                    eventSystemHealthy(true), stateManagerHealthy(true),
                    memoryUsageNormal(true), threadCountNormal(true),
                    lastCheck(std::chrono::steady_clock::now()) {}
};

// =============================================================================
// EXCEPTION HANDLER CLASS
// =============================================================================

class ExceptionHandler {
private:
    static std::atomic<bool> s_initialized;
    static std::function<void(const std::string&, ValidationLevel)> s_errorCallback;
    static std::mutex s_exceptionMutex;
    
    // Windows exception handler
    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
    
    // C++ exception handler
    static void terminateHandler();
    static void unexpectedHandler();
    
public:
    static bool initialize(std::function<void(const std::string&, ValidationLevel)> errorCallback = nullptr);
    static void shutdown();
    
    // Exception handling methods
    static void handleException(const std::exception& e, const std::string& context = "");
    static void handleSystemError(const std::string& error, ValidationLevel level = ValidationLevel::Error);
    static void handleCriticalError(const std::string& error);
    
    // Recovery methods
    static bool attemptRecovery(RecoveryAction action);
    static void showErrorDialog(const std::string& title, const std::string& message);
    static void logError(const std::string& error, ValidationLevel level);
};

// =============================================================================
// HEALTH MONITOR CLASS
// =============================================================================

class HealthMonitor {
private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_shouldStop;
    std::unique_ptr<std::thread> m_monitorThread;
    mutable std::mutex m_healthMutex;
    SystemHealth m_currentHealth;
    
    // Monitoring intervals
    static constexpr auto HEALTH_CHECK_INTERVAL = std::chrono::seconds(5);
    static constexpr auto MEMORY_CHECK_INTERVAL = std::chrono::seconds(30);
    static constexpr auto THREAD_CHECK_INTERVAL = std::chrono::seconds(10);
    
    // Thresholds
    static constexpr size_t MAX_MEMORY_USAGE_MB = 512;
    static constexpr size_t MAX_THREAD_COUNT = 20;
    static constexpr size_t MAX_HEALTH_FAILURES = 3;
    
    // Monitoring methods
    void monitoringLoop();
    ValidationResult checkGUISystem();
    ValidationResult checkInputSystem();
    ValidationResult checkEventSystem();
    ValidationResult checkStateManager();
    ValidationResult checkMemoryUsage();
    ValidationResult checkThreadCount();
    ValidationResult checkSystemResources();
    
    // Recovery methods
    bool attemptSystemRecovery(const ValidationResult& result);
    void restartUnhealthySystems();
    
public:
    HealthMonitor();
    ~HealthMonitor();
    
    bool initialize();
    void shutdown();
    
    // Health checking methods
    ValidationResult performHealthCheck();
    SystemHealth getCurrentHealth() const;
    bool isSystemHealthy() const;
    
    // Configuration methods
    void setHealthCheckInterval(std::chrono::milliseconds interval);
    void setMemoryThreshold(size_t maxMemoryMB);
    void setThreadThreshold(size_t maxThreads);
};

// =============================================================================
// APPLICATION VALIDATOR CLASS
// =============================================================================

class ApplicationValidator {
private:
    static std::unique_ptr<ApplicationValidator> s_instance;
    static std::mutex s_instanceMutex;
    
    std::unique_ptr<ExceptionHandler> m_exceptionHandler;
    std::unique_ptr<HealthMonitor> m_healthMonitor;
    
    std::atomic<bool> m_initialized;
    std::atomic<bool> m_shutdownRequested;
    std::mutex m_validatorMutex;
    
    // Validation history
    std::vector<ValidationResult> m_validationHistory;
    mutable std::mutex m_historyMutex;
    static constexpr size_t MAX_HISTORY_SIZE = 100;
    
    // Recovery state
    std::atomic<int> m_recoveryAttempts;
    std::chrono::steady_clock::time_point m_lastRecoveryAttempt;
    static constexpr int MAX_RECOVERY_ATTEMPTS = 3;
    static constexpr auto RECOVERY_COOLDOWN = std::chrono::minutes(5);
    
    // Internal validation methods
    ValidationResult validateApplicationState();
    ValidationResult validateSystemDependencies();
    ValidationResult validateThreadSafety();
    ValidationResult validateResourceUsage();
    
    // Recovery methods
    bool performRecovery(const ValidationResult& result);
    void handleValidationFailure(const ValidationResult& result);
    void updateValidationHistory(const ValidationResult& result);
    
public:
    ApplicationValidator();
    ~ApplicationValidator();
    
    static ApplicationValidator& getInstance();
    
    // Initialization and shutdown
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized.load(); }
    
    // Main validation methods
    ValidationResult validateApplication();
    bool performPeriodicValidation();
    
    // Error handling
    void handleError(const std::string& error, ValidationLevel level = ValidationLevel::Error);
    void handleCriticalError(const std::string& error);
    void handleSystemFailure(const std::string& failure);
    
    // Recovery control
    bool requestRecovery(RecoveryAction action);
    bool isRecoveryAvailable() const;
    int getRecoveryAttempts() const { return m_recoveryAttempts.load(); }
    
    // Health monitoring
    SystemHealth getSystemHealth() const;
    bool isSystemHealthy() const;
    
    // Validation history
    std::vector<ValidationResult> getValidationHistory() const;
    void clearValidationHistory();
    
    // Configuration
    void setValidationInterval(std::chrono::milliseconds interval);
    void setRecoveryEnabled(bool enabled);
    void setAutoRecoveryEnabled(bool enabled);
    
    // Shutdown control
    void requestShutdown();
    bool isShutdownRequested() const { return m_shutdownRequested.load(); }
};

// =============================================================================
// GLOBAL VALIDATION FUNCTIONS
// =============================================================================

// Initialize the validation system
bool initializeApplicationValidation();

// Shutdown the validation system
void shutdownApplicationValidation();

// Perform a validation check
ValidationResult validateApplicationHealth();

// Handle errors with the validation system
void handleApplicationError(const std::string& error, ValidationLevel level = ValidationLevel::Error);
void handleApplicationCriticalError(const std::string& error);

// Check if the application is healthy
bool isApplicationHealthy();

// Request application recovery
bool requestApplicationRecovery(RecoveryAction action);

// Get system health information
SystemHealth getApplicationHealth();

// =============================================================================
// VALIDATION MACROS (Simplified for -fno-exceptions)
// =============================================================================

#define VALIDATE_OR_RETURN(condition, errorMsg) \
    do { \
        if (!(condition)) { \
            handleApplicationError(errorMsg, ValidationLevel::Error); \
            return false; \
        } \
    } while(0)

#define VALIDATE_OR_THROW(condition, errorMsg) \
    do { \
        if (!(condition)) { \
            handleApplicationCriticalError(errorMsg); \
            return false; \
        } \
    } while(0)

#define VALIDATE_SYSTEM_HEALTH() \
    do { \
        if (!isApplicationHealthy()) { \
            handleApplicationError("System health check failed", ValidationLevel::Warning); \
        } \
    } while(0)

#define SAFE_EXECUTE(operation, errorContext) \
    do { \
        operation; \
    } while(0)

// End of application_validator.h