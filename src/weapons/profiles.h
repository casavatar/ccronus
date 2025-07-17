// profiles.h - WEAPON PROFILES HEADER FILE v3.1.9
// description: Weapon profiles management system - CORRECTED VERSION
// developer: ingekastel
// license: GNU General Public License v3.0
// version: 3.1.9 - Fixed all compilation errors and overload issues
// date: 2025-07-17
// project: Tactical Aim Assist

#pragma once
#ifndef PROFILES_H
#define PROFILES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include <chrono>
#include <cmath>

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

enum class PlayerMovementState {
    Stationary,
    Walking,
    Sprinting,
    Strafing,
    Sliding,
    Crouching,
    Jumping,
    Falling
};

// =============================================================================
// PID CONTROLLER PARAMETERS STRUCTURE
// =============================================================================

struct PIDParameters {
    double kp = 0.7;                    // Proportional gain
    double ki = 0.3;                    // Integral gain
    double kd = 0.2;                    // Derivative gain
    double max_output = 50.0;           // Maximum output limit
    double integral_limit = 100.0;      // Integral windup limit
    double derivative_limit = 25.0;     // Derivative kick limit
    
    // Default constructor
    PIDParameters() = default;
    
    // Parameterized constructor
    PIDParameters(double p, double i, double d, double max_out = 50.0, 
                 double int_limit = 100.0, double deriv_limit = 25.0)
        : kp(p), ki(i), kd(d), max_output(max_out), 
          integral_limit(int_limit), derivative_limit(deriv_limit) {}
    
    // Validation
    bool isValid() const {
        return kp >= 0.0 && ki >= 0.0 && kd >= 0.0 && 
               max_output > 0.0 && integral_limit > 0.0 && derivative_limit > 0.0;
    }
    
    // Reset to defaults
    void reset() {
        kp = 0.7;
        ki = 0.3;
        kd = 0.2;
        max_output = 50.0;
        integral_limit = 100.0;
        derivative_limit = 25.0;
    }
    
    // Scale all parameters by a factor
    void scale(double factor) {
        kp *= factor;
        ki *= factor;
        kd *= factor;
        max_output *= factor;
        integral_limit *= factor;
        derivative_limit *= factor;
    }
};

// =============================================================================
// RECOIL PATTERN POINT STRUCTURE
// =============================================================================

struct RecoilPoint {
    double x = 0.0;                     // Horizontal offset
    double y = 0.0;                     // Vertical offset
    
    RecoilPoint() = default;
    RecoilPoint(double x_offset, double y_offset) : x(x_offset), y(y_offset) {}
    
    // Vector operations
    RecoilPoint operator+(const RecoilPoint& other) const {
        return RecoilPoint(x + other.x, y + other.y);
    }
    
    RecoilPoint operator-(const RecoilPoint& other) const {
        return RecoilPoint(x - other.x, y - other.y);
    }
    
    RecoilPoint operator*(double scalar) const {
        return RecoilPoint(x * scalar, y * scalar);
    }
    
    // Magnitude
    double magnitude() const {
        return std::sqrt(x * x + y * y);
    }
    
    // Normalize
    RecoilPoint normalize() const {
        double mag = magnitude();
        if (mag > 0.0) {
            return RecoilPoint(x / mag, y / mag);
        }
        return RecoilPoint(0.0, 0.0);
    }
};

// =============================================================================
// FIRE MODE ENUMERATION
// =============================================================================

enum class FireMode {
    Single,                             // Single shot
    Controlled,                         // Controlled burst
    Automatic,                          // Full automatic
    Rapid,                              // Rapid fire
    Tactical,                           // Tactical mode
    Custom                              // Custom mode
};

// Fire mode utility functions
std::string fireModeToString(FireMode mode);
FireMode stringToFireMode(const std::string& mode_str);
bool isFireModeValid(FireMode mode);

// =============================================================================
// ENHANCED WEAPON PROFILE STRUCTURE
// =============================================================================

struct WeaponProfile {
    // Basic identification
    std::string name = "Default";
    FireMode fire_mode = FireMode::Single;
    
    // Core parameters
    double sensitivity = 1.0;
    double smoothing = 0.75;
    double prediction = 0.5;
    bool recoil_compensation = true;
    
    // Timing parameters
    int fire_delay_base = 50;           // Base fire delay in milliseconds
    int fire_delay_variance = 5;       // Variance in fire delay
    
    // Enhanced control parameters
    double smoothing_factor = 0.75;     // Smoothing factor for movements
    double prediction_aggressiveness = 0.5; // How aggressive prediction is
    
    // PID control parameters for different movement states
    std::unordered_map<std::string, PIDParameters> pid_states;
    
    // Recoil pattern
    std::vector<RecoilPoint> recoil_pattern;
    
    // Performance tracking
    uint64_t usage_count = 0;
    std::chrono::steady_clock::time_point last_used = std::chrono::steady_clock::now();
    double effectiveness_rating = 0.0;
    double accuracy_rating = 0.0;
    
    // Advanced settings
    struct AdvancedSettings {
        double aim_speed = 1.0;
        double trigger_delay = 50.0;
        double headshot_multiplier = 1.5;
        double body_multiplier = 1.0;
        double limb_multiplier = 0.8;
        double max_range = 1000.0;
        double optimal_range = 300.0;
        double min_range = 50.0;
        double aim_fov = 100.0;
        double trigger_fov = 50.0;
        double scan_fov = 200.0;
        bool enable_prediction = true;
        bool enable_recoil_comp = true;
        bool enable_auto_aim = false;
        bool enable_trigger_bot = false;
        bool enable_silent_aim = false;
        bool enable_adaptive_sensitivity = true;
        double reaction_time = 100.0;
        double acquisition_time = 50.0;
        double lock_time = 200.0;
    } advanced;
    
    // Default constructor
    WeaponProfile() {
        initializeDefaultPIDStates();
    }
    
    // Parameterized constructor
    WeaponProfile(const std::string& profile_name, FireMode mode, 
                  double sens = 1.0, double smooth = 0.75)
        : name(profile_name), fire_mode(mode), sensitivity(sens), smoothing(smooth) {
        initializeDefaultPIDStates();
    }
    
    // Copy constructor
    WeaponProfile(const WeaponProfile& other) = default;
    
    // Assignment operator
    WeaponProfile& operator=(const WeaponProfile& other) = default;
    
    // Move constructor
    WeaponProfile(WeaponProfile&& other) noexcept = default;
    
    // Move assignment
    WeaponProfile& operator=(WeaponProfile&& other) noexcept = default;
    
    // Destructor
    ~WeaponProfile() = default;
    
    // Validation
    bool isValid() const;
    
    // PID state management
    void initializeDefaultPIDStates();
    PIDParameters getPIDParameters(const std::string& movement_state) const;
    void setPIDParameters(const std::string& movement_state, const PIDParameters& params);
    bool hasPIDState(const std::string& movement_state) const;
    std::vector<std::string> getAvailablePIDStates() const;
    
    // Recoil pattern management
    void addRecoilPoint(double x, double y);
    void addRecoilPoint(const RecoilPoint& point);
    void clearRecoilPattern();
    RecoilPoint getRecoilOffset(int shot_number) const;
    size_t getRecoilPatternSize() const;
    void scaleRecoilPattern(double factor);
    void normalizeRecoilPattern();
    
    // Usage tracking
    void incrementUsage();
    void updateEffectiveness(double rating);
    void updateAccuracy(double accuracy);
    double getUsageScore() const;
    std::chrono::duration<double> getTimeSinceLastUse() const;
    
    // Utility methods
    std::string fireModeString() const;
    void resetToDefaults();
    void resetStatistics();
    
    // Serialization helpers
    std::string toString() const;
    void fromString(const std::string& data);
    
    // Comparison operators
    bool operator==(const WeaponProfile& other) const;
    bool operator!=(const WeaponProfile& other) const;
    
    // Performance optimization
    void optimizeForGame(const std::string& game_name);
    void optimizeForSensitivity(double target_sensitivity);
    void optimizeForAccuracy();
    void optimizeForSpeed();
};

// =============================================================================
// WEAPON PROFILE COLLECTION CLASS
// =============================================================================

class WeaponProfileCollection {
private:
    std::vector<std::unique_ptr<WeaponProfile>> profiles_;
    int active_profile_index_ = 0;
    std::string collection_name_ = "Default Collection";
    std::string file_path_ = "";
    bool auto_save_ = true;
    bool modified_ = false;
    
public:
    // Constructors
    WeaponProfileCollection() = default;
    explicit WeaponProfileCollection(const std::string& name);
    WeaponProfileCollection(const WeaponProfileCollection& other);
    WeaponProfileCollection& operator=(const WeaponProfileCollection& other);
    WeaponProfileCollection(WeaponProfileCollection&& other) noexcept;
    WeaponProfileCollection& operator=(WeaponProfileCollection&& other) noexcept;
    
    // Destructor
    ~WeaponProfileCollection();
    
    // Profile management
    bool addProfile(std::unique_ptr<WeaponProfile> profile);
    bool addProfile(const WeaponProfile& profile);
    bool removeProfile(int index);
    bool removeProfile(const std::string& name);
    WeaponProfile* getProfile(int index);
    const WeaponProfile* getProfile(int index) const;
    WeaponProfile* getProfile(const std::string& name);
    const WeaponProfile* getProfile(const std::string& name) const;
    WeaponProfile* getActiveProfile();
    const WeaponProfile* getActiveProfile() const;
    
    // Active profile management
    bool setActiveProfile(int index);
    bool setActiveProfile(const std::string& name);
    int getActiveProfileIndex() const;
    std::string getActiveProfileName() const;
    bool hasActiveProfile() const;
    
    // Collection information
    size_t size() const;
    bool empty() const;
    std::vector<std::string> getProfileNames() const;
    std::vector<int> getProfileIndices() const;
    bool hasProfile(const std::string& name) const;
    int findProfileIndex(const std::string& name) const;
    
    // File operations
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    bool loadFromJSON(const std::string& json_content);
    std::string saveToJSON() const;
    
    // Auto-save functionality
    void setAutoSave(bool enabled);
    bool isAutoSaveEnabled() const;
    void setFilePath(const std::string& path);
    std::string getFilePath() const;
    bool isModified() const;
    void markModified();
    void markSaved();
    
    // Collection management
    void clear();
    void sort(bool by_name = true);
    void reverse();
    WeaponProfileCollection clone() const;
    bool merge(const WeaponProfileCollection& other);
    
    // Statistics and analysis
    WeaponProfile* getMostUsedProfile();
    WeaponProfile* getMostEffectiveProfile();
    WeaponProfile* getMostAccurateProfile();
    std::vector<WeaponProfile*> getProfilesByFireMode(FireMode mode);
    std::vector<WeaponProfile*> getRecentlyUsedProfiles(int count = 5);
    
    // Utility
    std::string getCollectionName() const;
    void setCollectionName(const std::string& name);
    std::string toString() const;
    
    // Iterator support
    auto begin() { return profiles_.begin(); }
    auto end() { return profiles_.end(); }
    auto begin() const { return profiles_.begin(); }
    auto end() const { return profiles_.end(); }
    auto cbegin() const { return profiles_.cbegin(); }
    auto cend() const { return profiles_.cend(); }
};

// =============================================================================
// GLOBAL WEAPON PROFILE MANAGER
// =============================================================================

class WeaponProfileManager {
private:
    static WeaponProfileManager* instance_;
    WeaponProfileCollection profiles_;
    std::string profiles_directory_ = "./config/profiles/";
    bool initialized_ = false;
    
    // Private constructor for singleton
    WeaponProfileManager() = default;
    
public:
    // Singleton access
    static WeaponProfileManager& getInstance();
    static void destroyInstance();
    
    // Initialization
    bool initialize(const std::string& profiles_dir = "./config/profiles/");
    bool isInitialized() const;
    void shutdown();
    
    // Profile access
    WeaponProfileCollection& getProfiles();
    const WeaponProfileCollection& getProfiles() const;
    WeaponProfile* getActiveProfile();
    const WeaponProfile* getActiveProfile() const;
    
    // Quick access methods
    bool setActiveProfile(const std::string& name);
    bool setActiveProfile(int index);
    bool addProfile(const WeaponProfile& profile);
    bool removeProfile(const std::string& name);
    bool loadProfiles(const std::string& filename = "");
    bool saveProfiles(const std::string& filename = "") const;
    
    // Profile creation helpers
    WeaponProfile createDefaultProfile(const std::string& name);
    WeaponProfile createARProfile(const std::string& name);
    WeaponProfile createSMGProfile(const std::string& name);
    WeaponProfile createSniperProfile(const std::string& name);
    WeaponProfile createPistolProfile(const std::string& name);
    
    // Directory management
    std::string getProfilesDirectory() const;
    void setProfilesDirectory(const std::string& dir);
    std::vector<std::string> getAvailableProfileFiles() const;
    
    // Auto-detection and optimization
    bool detectAndLoadOptimalProfile(const std::string& game_name);
    void optimizeActiveProfile();
    void resetActiveProfileStatistics();
    
    // Backup and restore
    bool createBackup(const std::string& backup_name = "");
    bool restoreBackup(const std::string& backup_name);
    std::vector<std::string> getAvailableBackups() const;
    
    // Validation and repair
    bool validateProfiles();
    bool repairProfiles();
    void resetToDefaults();
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

// Profile creation utilities
WeaponProfile createProfileFromJSON(const std::string& json_data);
std::string profileToJSON(const WeaponProfile& profile);
bool validateProfileJSON(const std::string& json_data);

// PID utilities
PIDParameters interpolatePIDParameters(const PIDParameters& p1, const PIDParameters& p2, double t);
PIDParameters optimizePIDForTarget(const PIDParameters& base, double target_error);

// Recoil pattern utilities
std::vector<RecoilPoint> generateRecoilPattern(FireMode mode, int pattern_length);
std::vector<RecoilPoint> smoothRecoilPattern(const std::vector<RecoilPoint>& pattern, double smooth_factor);
RecoilPoint calculateRecoilCompensation(const std::vector<RecoilPoint>& pattern, int shot_number);

// Movement state utilities
std::string movementStateToString(PlayerMovementState state);
PlayerMovementState stringToMovementState(const std::string& state_str);

// Profile analysis utilities
double calculateProfileSimilarity(const WeaponProfile& p1, const WeaponProfile& p2);
WeaponProfile interpolateProfiles(const WeaponProfile& p1, const WeaponProfile& p2, double t);
std::vector<std::string> analyzeProfilePerformance(const WeaponProfile& profile);

#endif // PROFILES_H