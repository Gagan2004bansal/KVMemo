#pragma once
/**
 * @file metrics_snapshot.h
 * @brief Immutable snapshot of KVMemo runtime metrics.
 *
 * Metrics Snapshot represents a point-in-time view of system metrics.
 * It is produced by MetricsRegistry and used for :
 *  - Monitoring endpoints
 *  - Debugging tools
 *  - Performance reporting
 *  - Benchmark output
 *
 *  Thread Safety
 *  > Snapshot objects are immutable once created.
 *  > They are inherently thread-safe for concurrent reads.
 *
 *  Copyright © 2026 Gagan Bansal
 *  ALL RIGHT RESERVED
 */
#include <cstdint>
#include <string>

#include "latency_tracker.h"

namespace kvmemo::metrics
{
    /**
     * @brief Snapshot of command-level latency metrics.
     */
    struct CommandLatencySnapshot
    {
        LatencyStats get_latency;
        LatencyStats set_latency;
        LatencyStats delete_latency;
    };
    /**
     * @brief Snapshot of engine-level metrics.
     */
    struct EngineMetricsSnapshot
    {
        uint64_t total_keys{0};
        uint64_t total_requests{0};
        uint64_t total_evictions{0};
        uint64_t total_expirations{0};
    };
    /**
     * @brief Snapshot of network-level metrics.
     */
    struct NetworkMetricsSnapshot
    {
        uint64_t active_connections{0};
        uint64_t total_connections{0};
        uint64_t bytes_received{0};
        uint64_t bytes_sent{0};
    };

    /**
     * @brief Unified metrics snapshot for the entire KVMemo system.
     * This structure aggregates all metrics across subsystems.
     */
    class MetricsSnapshot final
    {
    public:
        MetricsSnapshot() = default;

        MetricsSnapshot(
            CommandLatencySnapshot command_latency,
            EngineMetricsSnapshot engine_metrics,
            NetworkMetricsSnapshot network_metrics) : command_latency_(command_latency),
                                                      engine_metrics_(engine_metrics),
                                                      network_metrics_(network_metrics) {}

        MetricsSnapshot(const MetricsSnapshot &) = default;
        MetricsSnapshot &operator=(const MetricsSnapshot &) = default;

        ~MetricsSnapshot() = default;

        /**
         * @brief Returns command latency metrics.
         */
        const CommandLatencySnapshot &commandLatency() const noexcept
        {
            return command_latency_;
        }

        /**
         * @brief Returns engine metrics.
         */
        const EngineMetricsSnapshot &EngineMetrics() const noexcept
        {
            return engine_metrics_;
        }

        /**
         * @brief Returns network metrics.
         */
        const NetworkMetricsSnapshot &NetworkMetrics() const noexcept
        {
            return network_metrics_;
        }

    private:
        CommandLatencySnapshot command_latency_;
        EngineMetricsSnapshot engine_metrics_;
        NetworkMetricsSnapshot network_metrics_;
    };
} // namespace kvmemo::metrics

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */