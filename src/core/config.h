// config.h - CONFIGURATION HEADER FILE v3.1.8
// description: Configuration management system header
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.8 - Complete header with ConfigValidationResult structure
// date: 2025-07-17
// project: Tactical Aim Assist

#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <mutex>

// =============================================================================
// CONFIGURATION VALIDATION RESULT STRUCTURE
// =============================================================================

struct ConfigValidationResult {
    bool is_valid = true;
    std::string timestamp = "";
    std::vector<std::string> issues;
    size_t error_count = 0;
    size_t warning_count = 0;
    
    // Default constructor
    ConfigValidationResult() = default;
    
    // Constructor with validation status
    ConfigValidationResult(bool valid) : is_valid(valid) {}
    
    // Copy constructor
    ConfigValidationResult(const ConfigValidationResult& other) = default;
    
    // Assignment operator
    ConfigValidationResult& operator=(const ConfigValidationResult& other) = default;
    
    // Utility methods
    bool hasErrors() const { return error_count > 0; }
    bool hasWarnings() const { return warning_count > 0; }
    bool hasIssues() const { return !issues.empty(); }
    size_t getTotalIssues() const { return error_count + warning_count; }
    
    // Clear all data
    void clear() {
        is_valid = true;
        timestamp.clear();
        issues.clear();
        error_count = 0;
        warning_count = 0;
    }
    
    // Add an error
    void addError(const std::string& error) {
        issues.push_back("[ERROR] " + error);
        error_count++;
        is_valid = false;
    }
    
    // Add a warning
    void addWarning(const std::string& warning) {
        issues.push_back("[WARNING] " + warning);
        warning_count++;
    }
    
    // Get summary string
    std::string getSummary() const {
        if (is_valid && issues.empty()) {
            return "Configuration is valid with no issues";
        }
        
        std::string summary = "Configuration ";
        summary += is_valid ? "is valid" : "has errors";
        
        if (error_count > 0) {
            summary += " (" + std::to_string(error_count) + " errors";
            if (warning_count > 0) {
                summary += ", " + std::to_string(warning_count) + " warnings";
            }
            summary += ")";
        } else if (warning_count > 0) {
            summary += " (" + std::to_string(warning_count) + " warnings)";
        }
        
        return summary;
    }
    
    // Get detailed report
    std::string getDetailedReport() const {
        std::string report = getSummary() + "\n";
        report += "Timestamp: " + timestamp + "\n";
        
        if (!issues.empty()) {
            report += "\nIssues found:\n";
            for (size_t i = 0; i < issues.size(); ++i) {
                report += std::to_string(i + 1) + ". " + issues[i] + "\n";
            }
        }
        
        return report;
    }
};

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

namespace ConfigConstants {
    // File paths
    constexpr const char* DEFAULT_CONFIG_FILE = "./config/main_config.json";
    constexpr const char* BACKUP_CONFIG_FILE = "./config/backup_config.json";
    constexpr const char* TEMPLATE_CONFIG_FILE = "./config/template_config.json";
    constexpr const char* USER_CONFIG_FILE = "./config/user_config.json";
    
    // Version information
    constexpr const char* CONFIG_VERSION = "3.1.8";
    constexpr int CONFIG_VERSION_MAJOR = 3;
    constexpr int CONFIG_VERSION_MINOR = 1;
    constexpr int CONFIG_VERSION_PATCH = 8;
    
    // Validation limits
    constexpr double MIN_SENSITIVITY = 0.1;
    constexpr double MAX_SENSITIVITY = 10.0;
    constexpr double MIN_SMOOTHING = 0.0;
    constexpr double MAX_SMOOTHING = 1.0;
    constexpr double MIN_FOV = 1.0;
    constexpr double MAX_FOV = 500.0;
    constexpr int MIN_FPS = 10;
    constexpr int MAX_FPS = 1000;
}

// =============================================================================
// EXTERNAL DECLARATIONS
// =============================================================================

extern ConfigValidationResult g_lastValidationResult;
extern std::string g_configFilePath;
extern std::mutex g_configurationMutex;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

// Core configuration functions
bool loadConfiguration(const std::string& filename);
bool saveConfiguration(const std::string& filename);
bool validateConfiguration();
void applyDefaultConfiguration();

// Advanced configuration functions
ConfigValidationResult validateConfigurationDetailed();
bool fixConfiguration();
bool exportConfiguration(const std::string& filename);
bool importConfiguration(const std::string& filename);

// Configuration utility functions
std::string getConfigurationSummary();
ConfigValidationResult getLastValidationResult();
bool resetConfigurationToFactory();
std::vector<std::string> getAvailableConfigurationFiles();
bool createConfigurationTemplate(const std::string& filename);

// Section-specific functions
bool loadConfigurationSection(const std::string& filename, const std::string& section);
bool saveConfigurationSection(const std::string& filename, const std::string& section);

// Configuration backup and restore
bool createConfigurationBackup(const std::string& backup_filename = "");
bool restoreConfigurationBackup(const std::string& backup_filename);
std::vector<std::string> getAvailableBackups();

// Configuration comparison and merging
bool compareConfigurations(const std::string& file1, const std::string& file2);
bool mergeConfigurations(const std::string& base_file, const std::string& overlay_file, const std::string& output_file);

// Configuration validation helpers
bool validateConfigurationFile(const std::string& filename);
bool isConfigurationFileValid(const std::string& filename);
std::string getConfigurationFileVersion(const std::string& filename);

#endif // CONFIG_H