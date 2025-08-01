// simd_math.h - SIMD optimized mathematical operations v3.0.2
// --------------------------------------------------------------------------------------
// description: SIMD-accelerated math functions for performance optimization
// --------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 3.0.2 - SIMD mathematical operations
// date: 2025-07-16
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// --------------------------------------------------------------------------------------

#pragma once

#ifndef SIMD_MATH_H
#define SIMD_MATH_H

#include <cmath>
#include <algorithm>
#include <utility>
#include <thread>
#include <cctype>

#ifdef _WIN32
#include <immintrin.h>
#include <intrin.h>
#endif

// =============================================================================
// SIMD CAPABILITY DETECTION
// =============================================================================
namespace SIMDCapabilities {
    bool hasSSE2();
    bool hasSSE4_1();
    bool hasAVX();
    bool hasAVX2();
    bool hasFMA();
    
    void detectCapabilities();
    std::string getCapabilitiesString();
}

// =============================================================================
// SIMD MATHEMATICAL OPERATIONS
// =============================================================================
namespace SIMDMath {
    
    // =============================================================================
    // DISTANCE CALCULATIONS
    // =============================================================================
    
    // Fast distance calculation using SIMD when available
    inline double fastDistance(double x1, double y1, double x2, double y2) {
        #ifdef _WIN32
        #ifdef __AVX__
        if (sizeof(void*) == 8) { // 64-bit check
            __m128d vec1 = _mm_set_pd(y1, x1);
            __m128d vec2 = _mm_set_pd(y2, x2);
            __m128d diff = _mm_sub_pd(vec1, vec2);
            __m128d squared = _mm_mul_pd(diff, diff);
            
            double result[2];
            _mm_store_pd(result, squared);
            return std::sqrt(result[0] + result[1]);
        } else {
        #endif
        #endif
            // Fallback to standard calculation
            double dx = x1 - x2;
            double dy = y1 - y2;
            return std::sqrt(dx * dx + dy * dy);
        #ifdef _WIN32
        #ifdef __AVX__
        }
        #endif
        #endif
    }
    
    // Overload for distance from origin
    inline double fastDistance(double x, double y) {
        return fastDistance(x, y, 0.0, 0.0);
    }
    
    // Manhattan distance (L1 norm)
    inline double manhattanDistance(double x1, double y1, double x2, double y2) {
        return std::abs(x1 - x2) + std::abs(y1 - y2);
    }
    
    // Squared distance (avoiding sqrt for comparison operations)
    inline double fastDistanceSquared(double x1, double y1, double x2, double y2) {
        double dx = x1 - x2;
        double dy = y1 - y2;
        return dx * dx + dy * dy;
    }
    
    // =============================================================================
    // SMOOTHING AND INTERPOLATION
    // =============================================================================
    
    // Fast linear interpolation with SIMD
    inline std::pair<double, double> fastSmooth(double current_x, double current_y,
                                               double target_x, double target_y,
                                               double factor) {
        #ifdef _WIN32
        #ifdef __AVX__
        if (sizeof(void*) == 8) { // 64-bit check
            __m128d current = _mm_set_pd(current_y, current_x);
            __m128d target = _mm_set_pd(target_y, target_x);
            __m128d factor_vec = _mm_set1_pd(factor);
            __m128d one_minus_factor = _mm_set1_pd(1.0 - factor);
            
            __m128d weighted_current = _mm_mul_pd(current, one_minus_factor);
            __m128d weighted_target = _mm_mul_pd(target, factor_vec);
            __m128d result = _mm_add_pd(weighted_current, weighted_target);
            
            double output[2];
            _mm_store_pd(output, result);
            return {output[1], output[0]}; // x, y (reversed due to pd order)
        } else {
        #endif
        #endif
            // Fallback to standard calculation
            double smoothed_x = current_x * (1.0 - factor) + target_x * factor;
            double smoothed_y = current_y * (1.0 - factor) + target_y * factor;
            return {smoothed_x, smoothed_y};
        #ifdef _WIN32
        #ifdef __AVX__
        }
        #endif
        #endif
    }
    
    // Exponential smoothing
    inline std::pair<double, double> exponentialSmooth(double current_x, double current_y,
                                                      double target_x, double target_y,
                                                      double alpha) {
        // Clamp alpha to valid range
        alpha = std::clamp(alpha, 0.0, 1.0);
        return fastSmooth(current_x, current_y, target_x, target_y, alpha);
    }
    
    // Cubic smoothing (Hermite interpolation)
    inline double cubicSmooth(double t) {
        t = std::clamp(t, 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
    
    // Smoothstep interpolation
    inline std::pair<double, double> smoothstep(double current_x, double current_y,
                                               double target_x, double target_y,
                                               double t) {
        double smooth_t = cubicSmooth(t);
        return fastSmooth(current_x, current_y, target_x, target_y, smooth_t);
    }
    
    // =============================================================================
    // VECTOR OPERATIONS
    // =============================================================================
    
    // Vector normalization
    inline std::pair<double, double> normalize(double x, double y) {
        double length = fastDistance(x, y);
        if (length < 1e-10) { // Avoid division by zero
            return {0.0, 0.0};
        }
        return {x / length, y / length};
    }
    
    // Vector dot product
    inline double dotProduct(double x1, double y1, double x2, double y2) {
        return x1 * x2 + y1 * y2;
    }
    
    // Vector cross product (2D - returns scalar)
    inline double crossProduct(double x1, double y1, double x2, double y2) {
        return x1 * y2 - y1 * x2;
    }
    
    // Vector scaling
    inline std::pair<double, double> scale(double x, double y, double factor) {
        return {x * factor, y * factor};
    }
    
    // Vector addition
    inline std::pair<double, double> add(double x1, double y1, double x2, double y2) {
        return {x1 + x2, y1 + y2};
    }
    
    // Vector subtraction
    inline std::pair<double, double> subtract(double x1, double y1, double x2, double y2) {
        return {x1 - x2, y1 - y2};
    }
    
    // =============================================================================
    // PREDICTION AND TARGETING
    // =============================================================================
    
    // Vectorized prediction calculation
    inline std::pair<double, double> predictTarget(double x, double y,
                                                  double vel_x, double vel_y,
                                                  double prediction_time_ms) {
        double time_sec = prediction_time_ms / 1000.0;
        
        #ifdef _WIN32
        #ifdef __AVX__
        if (sizeof(void*) == 8) {
            __m128d position = _mm_set_pd(y, x);
            __m128d velocity = _mm_set_pd(vel_y, vel_x);
            __m128d time_vec = _mm_set1_pd(time_sec);
            
            __m128d displacement = _mm_mul_pd(velocity, time_vec);
            __m128d predicted = _mm_add_pd(position, displacement);
            
            double result[2];
            _mm_store_pd(result, predicted);
            return {result[1], result[0]}; // x, y
        } else {
        #endif
        #endif
            return {x + vel_x * time_sec, y + vel_y * time_sec};
        #ifdef _WIN32
        #ifdef __AVX__
        }
        #endif
        #endif
    }
    
    // Lead target calculation
    inline std::pair<double, double> calculateLead(double target_x, double target_y,
                                                  double target_vel_x, double target_vel_y,
                                                  double projectile_speed) {
        if (projectile_speed <= 0.0) {
            return {target_x, target_y};
        }
        
        double distance = fastDistance(target_x, target_y);
        double time_to_target = distance / projectile_speed;
        
        return predictTarget(target_x, target_y, target_vel_x, target_vel_y, time_to_target * 1000.0);
    }
    
    // =============================================================================
    // TRIGONOMETRIC FUNCTIONS (OPTIMIZED)
    // =============================================================================
    
    // Fast sine approximation using Taylor series
    inline double fastSin(double x) {
        // Normalize to [-π, π]
        const double PI = 3.14159265358979323846;
        while (x > PI) x -= 2.0 * PI;
        while (x < -PI) x += 2.0 * PI;
        
        // Taylor series approximation
        double x2 = x * x;
        return x * (1.0 - x2 / 6.0 * (1.0 - x2 / 20.0 * (1.0 - x2 / 42.0)));
    }
    
    // Fast cosine approximation
    inline double fastCos(double x) {
        const double PI_2 = 1.57079632679489661923;
        return fastSin(x + PI_2);
    }
    
    // Fast arctangent approximation
    inline double fastAtan2(double y, double x) {
        if (std::abs(x) < 1e-10 && std::abs(y) < 1e-10) {
            return 0.0;
        }
        return std::atan2(y, x); // Use standard for now, can optimize later
    }
    
    // =============================================================================
    // CLAMPING AND BOUNDS CHECKING
    // =============================================================================
    
    // Fast clamping
    template<typename T>
    inline T fastClamp(T value, T min_val, T max_val) {
        return std::max(min_val, std::min(value, max_val));
    }
    
    // 2D bounds checking
    inline bool isWithinBounds(double x, double y, double min_x, double min_y, 
                              double max_x, double max_y) {
        return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
    }
    
    // Circular bounds checking
    inline bool isWithinCircle(double x, double y, double center_x, double center_y, double radius) {
        return fastDistanceSquared(x, y, center_x, center_y) <= radius * radius;
    }
    
    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================
    
    // Linear map from one range to another
    inline double mapRange(double value, double in_min, double in_max, 
                          double out_min, double out_max) {
        if (std::abs(in_max - in_min) < 1e-10) {
            return out_min;
        }
        return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
    }
    
    // Smooth step function
    inline double smoothStep(double edge0, double edge1, double x) {
        double t = fastClamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
    
    // Exponential decay
    inline double exponentialDecay(double value, double decay_rate, double dt) {
        return value * std::exp(-decay_rate * dt);
    }
    
    // Spring damping calculation
    inline double springDamping(double current, double target, double velocity, 
                               double spring_constant, double damping, double dt) {
        double force = (target - current) * spring_constant - velocity * damping;
        return velocity + force * dt;
    }
}

// =============================================================================
// SIMD CAPABILITY IMPLEMENTATIONS
// =============================================================================
namespace SIMDCapabilities {
    inline bool hasSSE2() {
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return (cpuInfo[3] & (1 << 26)) != 0;
        #else
        return false; // Conservative fallback
        #endif
    }
    
    inline bool hasSSE4_1() {
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return (cpuInfo[2] & (1 << 19)) != 0;
        #else
        return false;
        #endif
    }
    
    inline bool hasAVX() {
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return (cpuInfo[2] & (1 << 28)) != 0;
        #else
        return false;
        #endif
    }
    
    inline bool hasAVX2() {
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 7);
        return (cpuInfo[1] & (1 << 5)) != 0;
        #else
        return false;
        #endif
    }
    
    inline bool hasFMA() {
        #ifdef _WIN32
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return (cpuInfo[2] & (1 << 12)) != 0;
        #else
        return false;
        #endif
    }
    
    inline void detectCapabilities() {
        // This would log detected capabilities
    }
    
    inline std::string getCapabilitiesString() {
        std::string caps = "SIMD Capabilities: ";
        if (hasSSE2()) caps += "SSE2 ";
        if (hasSSE4_1()) caps += "SSE4.1 ";
        if (hasAVX()) caps += "AVX ";
        if (hasAVX2()) caps += "AVX2 ";
        if (hasFMA()) caps += "FMA ";
        return caps;
    }
}

#endif // SIMD_MATH_H