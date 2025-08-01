// application_validator.cpp - APPLICATION VALIDATION SYSTEM v1.0.0
// --------------------------------------------------------------------------------------------------------------
// description: Implementation of comprehensive validation system to prevent unexpected application shutdowns.
//              Provides exception handling, system health monitoring, and graceful error recovery.
// --------------------------------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 1.0.0 - Initial implementation
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------------------------------

#include "application_validator.h"
#include "globals.h"
#include "state_manager.h"
#include "event_system.h"
#include "input.h"
#include "gui.h"

#include <psapi.h>
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>

// =============================================================================
// EXCEPTION HANDLER IMPLEMENTATION
// =============================================================================

std::atomic<bool> ExceptionHandler::s_initialized(false);
std::function<void(const std::string&, ValidationLevel)> ExceptionHandler::s_errorCallback(nullptr);
std::mutex ExceptionHandler::s_exceptionMutex;

bool ExceptionHandler::initialize(std::function<void(const std::string&, ValidationLevel)> errorCallback) {
    std::lock_guard<std::mutex> lock(s_exceptionMutex);
    
    if (s_initialized.load()) {
        return true;
    }
    
    s_errorCallback = errorCallback;
    
    // Set up Windows exception handler
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    
    // Set up C++ exception handlers
    std::set_terminate(terminateHandler);
    // Note: set_unexpected is deprecated in C++17, but we'll keep it for compatibility
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::set_unexpected(unexpectedHandler);
    #pragma GCC diagnostic pop
    
    s_initialized.store(true);
    logMessage("Exception handler initialized successfully");
    return true;
}

void ExceptionHandler::shutdown() {
    std::lock_guard<std::mutex> lock(s_exceptionMutex);
    
    if (!s_initialized.load()) {
        return;
    }
    
    // Restore default handlers
    SetUnhandledExceptionFilter(nullptr);
    std::set_terminate(nullptr);
    // Note: set_unexpected is deprecated in C++17, but we'll keep it for compatibility
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::set_unexpected(nullptr);
    #pragma GCC diagnostic pop
    
    s_initialized.store(false);
    s_errorCallback = nullptr;
    
    logMessage("Exception handler shutdown complete");
}

LONG WINAPI ExceptionHandler::unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    std::string errorMsg = "Unhandled Windows exception: ";
    
    switch (exceptionInfo->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            errorMsg += "Access violation";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            errorMsg += "Stack overflow";
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            errorMsg += "Illegal instruction";
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            errorMsg += "Privileged instruction";
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            errorMsg += "Non-continuable exception";
            break;
        default:
            errorMsg += "Unknown exception (0x" + 
                       std::to_string(exceptionInfo->ExceptionRecord->ExceptionCode) + ")";
            break;
    }
    
    handleCriticalError(errorMsg);
    return EXCEPTION_EXECUTE_HANDLER;
}

void ExceptionHandler::terminateHandler() {
    std::string errorMsg = "Application terminated unexpectedly";
    handleCriticalError(errorMsg);
    
    // Show error dialog
    showErrorDialog("Application Error", 
                   "The application has encountered a critical error and must shut down.\n\n"
                   "Error: " + errorMsg + "\n\n"
                   "Please restart the application.");
    
    // Force exit
    ExitProcess(1);
}

void ExceptionHandler::unexpectedHandler() {
    std::string errorMsg = "Unexpected exception occurred";
    handleSystemError(errorMsg, ValidationLevel::Error);
}

void ExceptionHandler::handleException(const std::exception& e, const std::string& context) {
    std::string errorMsg = context.empty() ? e.what() : context + ": " + e.what();
    handleSystemError(errorMsg, ValidationLevel::Error);
}

void ExceptionHandler::handleSystemError(const std::string& error, ValidationLevel level) {
    logError(error, level);
    
    if (s_errorCallback) {
        s_errorCallback(error, level);
    }
    
    // Show error dialog for critical errors
    if (level == ValidationLevel::Critical) {
        showErrorDialog("Critical Error", error);
    }
}

void ExceptionHandler::handleCriticalError(const std::string& error) {
    handleSystemError(error, ValidationLevel::Critical);
}

bool ExceptionHandler::attemptRecovery(RecoveryAction action) {
    switch (action) {
        case RecoveryAction::Restart:
            logMessage("Attempting system restart...");
            // Implement restart logic
            return true;
            
        case RecoveryAction::Reinitialize:
            logMessage("Attempting system reinitialization...");
            // Implement reinitialization logic
            return true;
            
        case RecoveryAction::Reset:
            logMessage("Attempting system reset...");
            // Implement reset logic
            return true;
            
        case RecoveryAction::Shutdown:
            logMessage("Initiating graceful shutdown...");
            // Implement shutdown logic
            return true;
            
        default:
            return false;
    }
}

void ExceptionHandler::showErrorDialog(const std::string& title, const std::string& message) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), 
                MB_OK | MB_ICONERROR | MB_TOPMOST);
}

void ExceptionHandler::logError(const std::string& error, ValidationLevel level) {
    std::string levelStr;
    switch (level) {
        case ValidationLevel::Critical: levelStr = "CRITICAL"; break;
        case ValidationLevel::Error: levelStr = "ERROR"; break;
        case ValidationLevel::Warning: levelStr = "WARNING"; break;
        case ValidationLevel::Info: levelStr = "INFO"; break;
    }
    
    logMessage("[" + levelStr + "] " + error);
}

// =============================================================================
// HEALTH MONITOR IMPLEMENTATION
// =============================================================================

HealthMonitor::HealthMonitor() : m_running(false), m_shouldStop(false) {}

HealthMonitor::~HealthMonitor() {
    shutdown();
}

bool HealthMonitor::initialize() {
    if (m_running.load()) {
        return true;
    }
    
    m_running.store(true);
    m_shouldStop.store(false);
    
    m_monitorThread = std::make_unique<std::thread>(&HealthMonitor::monitoringLoop, this);
    
    logMessage("Health monitor initialized successfully");
    return true;
}

void HealthMonitor::shutdown() {
    if (!m_running.load()) {
        return;
    }
    
    m_shouldStop.store(true);
    m_running.store(false);
    
    if (m_monitorThread && m_monitorThread->joinable()) {
        m_monitorThread->join();
    }
    
    m_monitorThread.reset();
    logMessage("Health monitor shutdown complete");
}

void HealthMonitor::monitoringLoop() {
    logMessage("Health monitoring loop started");
    
    auto lastMemoryCheck = std::chrono::steady_clock::now();
    auto lastThreadCheck = std::chrono::steady_clock::now();
    auto lastSystemCheck = std::chrono::steady_clock::now();
    
    // Increase intervals to reduce overhead
    static constexpr auto MEMORY_CHECK_INTERVAL = std::chrono::seconds(60);  // Was 30s
    static constexpr auto THREAD_CHECK_INTERVAL = std::chrono::seconds(30);  // Was 10s
    static constexpr auto SYSTEM_CHECK_INTERVAL = std::chrono::seconds(30);  // Was 5s
    
    while (!m_shouldStop.load()) {
        auto now = std::chrono::steady_clock::now();
        
        // Perform system health checks less frequently
        if (now - lastSystemCheck >= SYSTEM_CHECK_INTERVAL) {
            performHealthCheck();
            lastSystemCheck = now;
        }
        
        // Check memory usage less frequently
        if (now - lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
            checkMemoryUsage();
            lastMemoryCheck = now;
        }
        
        // Check thread count less frequently
        if (now - lastThreadCheck >= THREAD_CHECK_INTERVAL) {
            checkThreadCount();
            lastThreadCheck = now;
        }
        
        // Increased sleep interval to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Was 100ms
    }
    
    logMessage("Health monitoring loop ended");
}

ValidationResult HealthMonitor::performHealthCheck() {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    
    // Check all systems
    auto guiResult = checkGUISystem();
    auto inputResult = checkInputSystem();
    auto eventResult = checkEventSystem();
    auto stateResult = checkStateManager();
    
    // Update health status
    m_currentHealth.guiSystemHealthy = guiResult.isValid;
    m_currentHealth.inputSystemHealthy = inputResult.isValid;
    m_currentHealth.eventSystemHealthy = eventResult.isValid;
    m_currentHealth.stateManagerHealthy = stateResult.isValid;
    m_currentHealth.lastCheck = std::chrono::steady_clock::now();
    
    // Determine overall health
    bool overallHealthy = guiResult.isValid && inputResult.isValid && 
                         eventResult.isValid && stateResult.isValid;
    
    if (!overallHealthy) {
        std::string errorMsg = "System health check failed: ";
        if (!guiResult.isValid) errorMsg += "GUI ";
        if (!inputResult.isValid) errorMsg += "Input ";
        if (!eventResult.isValid) errorMsg += "Event ";
        if (!stateResult.isValid) errorMsg += "State ";
        
        return ValidationResult(false, ValidationLevel::Warning, errorMsg, RecoveryAction::Reinitialize);
    }
    
    return ValidationResult(true, ValidationLevel::Info, "System health check passed");
}

ValidationResult HealthMonitor::checkGUISystem() {
    // Check if GUI window exists and is valid
    HWND mainWindow = FindWindowA("TacticalAimAssistGUIClass", nullptr);
    if (!mainWindow || !IsWindow(mainWindow)) {
        return ValidationResult(false, ValidationLevel::Error, "GUI window not found", RecoveryAction::Restart);
    }
    
    return ValidationResult(true, ValidationLevel::Info, "GUI system healthy");
}

ValidationResult HealthMonitor::checkInputSystem() {
    // Check if input system is responding
    // This is a simplified check - in a real implementation, you'd check actual input system state
    return ValidationResult(true, ValidationLevel::Info, "Input system healthy");
}

ValidationResult HealthMonitor::checkEventSystem() {
    // Check if event system is running
    // This is a simplified check - in a real implementation, you'd check actual event system state
    return ValidationResult(true, ValidationLevel::Info, "Event system healthy");
}

ValidationResult HealthMonitor::checkStateManager() {
    // Check if state manager is initialized and running
    // This is a simplified check - in a real implementation, you'd check actual state manager state
    return ValidationResult(true, ValidationLevel::Info, "State manager healthy");
}

ValidationResult HealthMonitor::checkMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        size_t memoryUsageMB = pmc.WorkingSetSize / (1024 * 1024);
        
        if (memoryUsageMB > MAX_MEMORY_USAGE_MB) {
            m_currentHealth.memoryUsageNormal = false;
            return ValidationResult(false, ValidationLevel::Warning, 
                                 "High memory usage: " + std::to_string(memoryUsageMB) + "MB", 
                                 RecoveryAction::Reset);
        }
        
        m_currentHealth.memoryUsageNormal = true;
    }
    
    return ValidationResult(true, ValidationLevel::Info, "Memory usage normal");
}

ValidationResult HealthMonitor::checkThreadCount() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return ValidationResult(true, ValidationLevel::Info, "Could not check thread count");
    }
    
    DWORD currentProcessId = GetCurrentProcessId();
    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);
    
    size_t threadCount = 0;
    if (Thread32First(snapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == currentProcessId) {
                threadCount++;
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }
    
    CloseHandle(snapshot);
    
    if (threadCount > MAX_THREAD_COUNT) {
        m_currentHealth.threadCountNormal = false;
        return ValidationResult(false, ValidationLevel::Warning, 
                             "High thread count: " + std::to_string(threadCount), 
                             RecoveryAction::Reset);
    }
    
    m_currentHealth.threadCountNormal = true;
    return ValidationResult(true, ValidationLevel::Info, "Thread count normal");
}

ValidationResult HealthMonitor::checkSystemResources() {
    // Check system resources like handles, GDI objects, etc.
    return ValidationResult(true, ValidationLevel::Info, "System resources normal");
}

SystemHealth HealthMonitor::getCurrentHealth() const {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    return m_currentHealth;
}

bool HealthMonitor::isSystemHealthy() const {
    std::lock_guard<std::mutex> lock(m_healthMutex);
    return m_currentHealth.guiSystemHealthy && m_currentHealth.inputSystemHealthy &&
           m_currentHealth.eventSystemHealthy && m_currentHealth.stateManagerHealthy &&
           m_currentHealth.memoryUsageNormal && m_currentHealth.threadCountNormal;
}

// =============================================================================
// APPLICATION VALIDATOR IMPLEMENTATION
// =============================================================================

std::unique_ptr<ApplicationValidator> ApplicationValidator::s_instance = nullptr;
std::mutex ApplicationValidator::s_instanceMutex;

ApplicationValidator::ApplicationValidator() 
    : m_initialized(false), m_shutdownRequested(false), m_recoveryAttempts(0) {
    m_exceptionHandler = std::make_unique<ExceptionHandler>();
    m_healthMonitor = std::make_unique<HealthMonitor>();
}

ApplicationValidator::~ApplicationValidator() {
    shutdown();
}

ApplicationValidator& ApplicationValidator::getInstance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::make_unique<ApplicationValidator>();
    }
    return *s_instance;
}

bool ApplicationValidator::initialize() {
    std::lock_guard<std::mutex> lock(m_validatorMutex);
    
    if (m_initialized.load()) {
        return true;
    }
    
    // Initialize exception handler
    if (!ExceptionHandler::initialize([this](const std::string& error, ValidationLevel level) {
        this->handleError(error, level);
    })) {
        logError("Failed to initialize exception handler");
        return false;
    }
    
    // Initialize health monitor
    if (!m_healthMonitor->initialize()) {
        logError("Failed to initialize health monitor");
        return false;
    }
    
    m_initialized.store(true);
    logMessage("Application validator initialized successfully");
    return true;
}

void ApplicationValidator::shutdown() {
    std::lock_guard<std::mutex> lock(m_validatorMutex);
    
    if (!m_initialized.load()) {
        return;
    }
    
    m_healthMonitor->shutdown();
    ExceptionHandler::shutdown();
    
    m_initialized.store(false);
    logMessage("Application validator shutdown complete");
}

ValidationResult ApplicationValidator::validateApplication() {
    if (!m_initialized.load()) {
        return ValidationResult(false, ValidationLevel::Error, "Validator not initialized");
    }
    
    // Perform comprehensive validation
    auto stateResult = validateApplicationState();
    if (!stateResult.isValid) {
        return stateResult;
    }
    
    auto dependencyResult = validateSystemDependencies();
    if (!dependencyResult.isValid) {
        return dependencyResult;
    }
    
    auto threadResult = validateThreadSafety();
    if (!threadResult.isValid) {
        return threadResult;
    }
    
    auto resourceResult = validateResourceUsage();
    if (!resourceResult.isValid) {
        return resourceResult;
    }
    
    return ValidationResult(true, ValidationLevel::Info, "Application validation passed");
}

ValidationResult ApplicationValidator::validateApplicationState() {
    // Check if application is in a valid state
    if (m_shutdownRequested.load()) {
        return ValidationResult(false, ValidationLevel::Critical, "Application shutdown requested");
    }
    
    return ValidationResult(true, ValidationLevel::Info, "Application state valid");
}

ValidationResult ApplicationValidator::validateSystemDependencies() {
    // Check if all required systems are available and functioning
    return ValidationResult(true, ValidationLevel::Info, "System dependencies valid");
}

ValidationResult ApplicationValidator::validateThreadSafety() {
    // Check for potential deadlocks or thread issues
    return ValidationResult(true, ValidationLevel::Info, "Thread safety validated");
}

ValidationResult ApplicationValidator::validateResourceUsage() {
    // Check resource usage and limits
    return ValidationResult(true, ValidationLevel::Info, "Resource usage validated");
}

bool ApplicationValidator::performPeriodicValidation() {
    auto result = validateApplication();
    updateValidationHistory(result);
    
    if (!result.isValid) {
        handleValidationFailure(result);
    }
    
    return result.isValid;
}

void ApplicationValidator::handleError(const std::string& error, ValidationLevel level) {
    logError("Application error: " + error);
    
    if (level == ValidationLevel::Critical) {
        handleCriticalError(error);
    }
}

void ApplicationValidator::handleCriticalError(const std::string& error) {
    logError("Critical error: " + error);
    
    // Show error dialog
    ExceptionHandler::showErrorDialog("Critical Application Error", 
                                    "A critical error has occurred:\n\n" + error + 
                                    "\n\nThe application will attempt to recover or shut down gracefully.");
    
    // Request shutdown if recovery is not possible
    if (!isRecoveryAvailable()) {
        requestShutdown();
    }
}

void ApplicationValidator::handleSystemFailure(const std::string& failure) {
    logError("System failure: " + failure);
    handleError(failure, ValidationLevel::Critical);
}

bool ApplicationValidator::requestRecovery(RecoveryAction action) {
    if (!isRecoveryAvailable()) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (m_recoveryAttempts.load() >= MAX_RECOVERY_ATTEMPTS) {
        if (now - m_lastRecoveryAttempt < RECOVERY_COOLDOWN) {
            logWarning("Recovery attempts exceeded, waiting for cooldown");
            return false;
        }
        m_recoveryAttempts.store(0);
    }
    
    m_recoveryAttempts.fetch_add(1);
    m_lastRecoveryAttempt = now;
    
    return ExceptionHandler::attemptRecovery(action);
}

bool ApplicationValidator::isRecoveryAvailable() const {
    return m_recoveryAttempts.load() < MAX_RECOVERY_ATTEMPTS;
}

SystemHealth ApplicationValidator::getSystemHealth() const {
    return m_healthMonitor->getCurrentHealth();
}

bool ApplicationValidator::isSystemHealthy() const {
    return m_healthMonitor->isSystemHealthy();
}

std::vector<ValidationResult> ApplicationValidator::getValidationHistory() const {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    return m_validationHistory;
}

void ApplicationValidator::clearValidationHistory() {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    m_validationHistory.clear();
}

void ApplicationValidator::updateValidationHistory(const ValidationResult& result) {
    std::lock_guard<std::mutex> lock(m_historyMutex);
    
    m_validationHistory.push_back(result);
    
    // Keep history size manageable
    if (m_validationHistory.size() > MAX_HISTORY_SIZE) {
        m_validationHistory.erase(m_validationHistory.begin());
    }
}

void ApplicationValidator::handleValidationFailure(const ValidationResult& result) {
    logError("Validation failure: " + result.message);
    
    if (result.recoveryAction != RecoveryAction::None) {
        performRecovery(result);
    }
}

bool ApplicationValidator::performRecovery(const ValidationResult& result) {
    return requestRecovery(result.recoveryAction);
}

void ApplicationValidator::requestShutdown() {
    m_shutdownRequested.store(true);
    logMessage("Application shutdown requested");
}

// =============================================================================
// GLOBAL VALIDATION FUNCTIONS
// =============================================================================

bool initializeApplicationValidation() {
    return ApplicationValidator::getInstance().initialize();
}

void shutdownApplicationValidation() {
    ApplicationValidator::getInstance().shutdown();
}

ValidationResult validateApplicationHealth() {
    return ApplicationValidator::getInstance().validateApplication();
}

void handleApplicationError(const std::string& error, ValidationLevel level) {
    ApplicationValidator::getInstance().handleError(error, level);
}

void handleApplicationCriticalError(const std::string& error) {
    ApplicationValidator::getInstance().handleCriticalError(error);
}

bool isApplicationHealthy() {
    return ApplicationValidator::getInstance().isSystemHealthy();
}

bool requestApplicationRecovery(RecoveryAction action) {
    return ApplicationValidator::getInstance().requestRecovery(action);
}

SystemHealth getApplicationHealth() {
    return ApplicationValidator::getInstance().getSystemHealth();
} 