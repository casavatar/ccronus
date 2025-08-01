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
#include "../core/globals.h"

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

// FireMode, PIDParameters, RecoilPoint, and PlayerMovementState are defined in globals.h

// =============================================================================
// FIRE MODE UTILITY FUNCTIONS
// =============================================================================

std::string fireModeToString(FireMode mode);
FireMode stringToFireMode(const std::string& mode_str);
bool isFireModeValid(FireMode mode);

// =============================================================================
// ENHANCED WEAPON PROFILE STRUCTURE
// =============================================================================

// WeaponProfile is defined in globals.h - using that definition

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