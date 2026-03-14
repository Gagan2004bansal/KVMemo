#pragma once
/**
 * @file thread_pool.h
 * @brief Generic thread pool for executing server tasks.
 *
 * Responsibilities :
 * - Maintain a pool of worker threads.
 * - Execute submitted tasks asynchronously.
 * - Reuse threads to avoid thread creation overhead.
 *
 * Thread Safety :
 * > Fully thread-safe.
 * > Multiple producers can submit tasks.
 *
 *  Copyright © 2026 KVMemo
 *  Author: Gagan Bansal
 *  ALL RIGHTS RESERVED.
 */

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>

namespace kvmemo::server
{
    /**
     * @brief Fixed size worker thread pool.
     */
    class ThreadPool final
    {
    public:
        /**
         * @brief Creates thread pool with given worker count.
         */
        explicit ThreadPool(std::size_t thread_count)
            : stop_(false)
        {
            workers_.reserve(thread_count);

            for (std::size_t i = 0; i < thread_count; ++i)
            {
                workers_.emplace_back([this]
                                      { WorkerLoop(); });
            }
        }

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;

        ThreadPool(ThreadPool &&) noexcept = delete;
        ThreadPool &operator=(ThreadPool &&) noexcept = delete;

        /**
         * @brief Gracefully shuts down pool.
         */
        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                stop_ = true;
            }

            condition_.notify_all();

            for (auto &thread : workers_)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }

        /**
         * @brief Submits a task for execution.
         */
        void Submit(std::function<void()> task)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                tasks_.push(std::move(task));
            }

            condition_.notify_one();
        }

    private:
        /**
         * @brief Worker execution loop.
         */
        void WorkerLoop()
        {
            std::function<void()> task;
            while (true)
            {
                std::unique_lock<std::mutex> lock(mutex_);

                condition_.wait(lock, [this]
                                { return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty())
                {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_;
    };
} // namespace kvmemo::server

/**
 * This source code may not be copied, modified, or
 * distributed without explicit permission from the author.
 */