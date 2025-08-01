// network_optimizer.h - NETWORK OPTIMIZATION FRAMEWORK v1.0.0
// ----------------------------------------------------------------------------------------------
// description: Low-latency, scalable networking components for future network features.
//              Implements connection pooling, message batching, and adaptive protocols.
// ----------------------------------------------------------------------------------------------
// developer: ekastel
//
// version: 1.0.0 - Initial implementation
// date: 2025-07-21
// project: Tactical Aim Assist
// license: GNU General Public License v3.0
// ----------------------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Network message structure
struct NetworkMessage {
    uint32_t message_id;
    uint32_t sequence_number;
    uint64_t timestamp;
    std::string payload;
    uint8_t priority;
    bool reliable;
    
    NetworkMessage() : message_id(0), sequence_number(0), timestamp(0), priority(0), reliable(false) {}
    NetworkMessage(uint32_t id, const std::string& data, uint8_t prio = 0, bool rel = false)
        : message_id(id), sequence_number(0), timestamp(0), payload(data), priority(prio), reliable(rel) {}
};

// Connection pool for network optimization
class ConnectionPool {
public:
    struct Connection {
        int socket_fd;
        std::string address;
        uint16_t port;
        std::chrono::steady_clock::time_point last_used;
        bool is_active;
        uint64_t message_count;
        double latency_ms;
        
        Connection() : socket_fd(-1), port(0), is_active(false), message_count(0), latency_ms(0.0) {}
    };
    
    ConnectionPool(size_t max_connections = 10);
    ~ConnectionPool();
    
    // Connection management
    std::shared_ptr<Connection> acquireConnection(const std::string& address, uint16_t port);
    void releaseConnection(std::shared_ptr<Connection> conn);
    void closeConnection(std::shared_ptr<Connection> conn);
    
    // Pool statistics
    size_t getActiveConnections() const;
    size_t getAvailableConnections() const;
    double getAverageLatency() const;
    
private:
    std::vector<std::shared_ptr<Connection>> m_connections;
    mutable std::mutex m_pool_mutex;
    size_t m_max_connections;
    
    std::shared_ptr<Connection> createConnection(const std::string& address, uint16_t port);
    void cleanupInactiveConnections();
};

// Message batcher for network optimization
class MessageBatcher {
public:
    struct Batch {
        std::vector<NetworkMessage> messages;
        uint64_t batch_id;
        std::chrono::steady_clock::time_point created_time;
        size_t total_size;
        
        Batch() : batch_id(0), total_size(0) {}
    };
    
    MessageBatcher(size_t max_batch_size = 1024, std::chrono::milliseconds max_wait_time = std::chrono::milliseconds(10));
    ~MessageBatcher();
    
    // Message batching
    void addMessage(const NetworkMessage& message);
    std::shared_ptr<Batch> getNextBatch();
    void clearBatches();
    
    // Batch configuration
    void setMaxBatchSize(size_t size);
    void setMaxWaitTime(std::chrono::milliseconds time);
    
    // Statistics
    size_t getPendingMessages() const;
    size_t getBatchCount() const;
    double getAverageBatchSize() const;
    
private:
    std::deque<std::shared_ptr<Batch>> m_batches;
    mutable std::mutex m_batch_mutex;
    size_t m_max_batch_size;
    std::chrono::milliseconds m_max_wait_time;
    uint64_t m_next_batch_id;
    
    std::shared_ptr<Batch> createNewBatch();
    void addToCurrentBatch(const NetworkMessage& message);
};

// Adaptive protocol for network optimization
class AdaptiveProtocol {
public:
    enum class ProtocolType {
        UDP,
        TCP,
        ReliableUDP,
        Custom
    };
    
    struct ProtocolMetrics {
        double latency_ms;
        double throughput_mbps;
        double packet_loss_rate;
        double jitter_ms;
        uint64_t total_packets;
        uint64_t lost_packets;
        
        ProtocolMetrics() : latency_ms(0.0), throughput_mbps(0.0), packet_loss_rate(0.0), 
                          jitter_ms(0.0), total_packets(0), lost_packets(0) {}
    };
    
    AdaptiveProtocol();
    ~AdaptiveProtocol();
    
    // Protocol selection
    ProtocolType selectOptimalProtocol(const ProtocolMetrics& metrics);
    void updateMetrics(const ProtocolMetrics& metrics);
    
    // Configuration
    void setLatencyThreshold(double threshold_ms);
    void setThroughputThreshold(double threshold_mbps);
    void setPacketLossThreshold(double threshold);
    
    // Statistics
    ProtocolMetrics getCurrentMetrics() const;
    ProtocolType getCurrentProtocol() const;
    double getOptimizationScore() const;
    
private:
    ProtocolType m_current_protocol;
    ProtocolMetrics m_current_metrics;
    std::vector<ProtocolMetrics> m_metrics_history;
    
    double m_latency_threshold;
    double m_throughput_threshold;
    double m_packet_loss_threshold;
    
    void analyzeMetrics();
    double calculateProtocolScore(ProtocolType protocol, const ProtocolMetrics& metrics);
};

// Network optimizer main class
class NetworkOptimizer {
public:
    static NetworkOptimizer& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Connection management
    std::shared_ptr<ConnectionPool::Connection> getConnection(const std::string& address, uint16_t port);
    void releaseConnection(std::shared_ptr<ConnectionPool::Connection> conn);
    
    // Message handling
    void sendMessage(const NetworkMessage& message);
    void sendBatch(const std::vector<NetworkMessage>& messages);
    
    // Configuration
    void setConnectionPoolSize(size_t size);
    void setBatchSize(size_t size);
    void setProtocol(AdaptiveProtocol::ProtocolType protocol);
    
    // Statistics
    size_t getActiveConnections() const;
    size_t getPendingMessages() const;
    double getAverageLatency() const;
    double getThroughput() const;
    
private:
    NetworkOptimizer() = default;
    ~NetworkOptimizer() = default;
    NetworkOptimizer(const NetworkOptimizer&) = delete;
    NetworkOptimizer& operator=(const NetworkOptimizer&) = delete;
    
    std::unique_ptr<ConnectionPool> m_connection_pool;
    std::unique_ptr<MessageBatcher> m_message_batcher;
    std::unique_ptr<AdaptiveProtocol> m_adaptive_protocol;
    
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_optimizer_mutex;
};

// Global functions
bool initializeNetworkOptimizer();
void shutdownNetworkOptimizer();
NetworkOptimizer& getNetworkOptimizer(); 