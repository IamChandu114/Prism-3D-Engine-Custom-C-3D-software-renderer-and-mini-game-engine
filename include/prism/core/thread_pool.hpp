#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace prism::core {

// ----------------------------------------------------------------------------
// ThreadPool – zero-heap-allocation parallel_for after construction.
//
// Key design:
// 1. Fixed worker threads created once at startup.
// 2. Per-batch descriptor lives on the stack of parallel_for.
// 3. Workers claim index chunks via atomic fetch_add.
// 4. Latch synchronization ensures all workers completely exit the batch
//    before parallel_for returns, eliminating dangling stack references.
// ----------------------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 0) : stop_(false) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0)
                num_threads = 4;
        }

        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this]() { worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_work_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable())
                w.join();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    [[nodiscard]] size_t worker_count() const noexcept { return workers_.size(); }

    // Parallel-for over [0, count). Zero heap allocations per call.
    template <typename Func> void parallel_for(int count, Func&& func, int chunk_size = 1) {
        if (count <= 0)
            return;

        if (workers_.empty()) {
            for (int i = 0; i < count; ++i)
                func(i);
            return;
        }

        using FuncRef = std::remove_reference_t<Func>;
        auto invoke_fn = [](void* ptr, int idx) { (*reinterpret_cast<FuncRef*>(ptr))(idx); };

        Batch batch;
        batch.func_ptr = reinterpret_cast<void*>(&func);
        batch.invoke = invoke_fn;
        batch.next_idx.store(0, std::memory_order_relaxed);
        batch.total = count;
        batch.chunk_size = std::max(1, chunk_size);
        batch.remaining_workers.store(static_cast<int>(workers_.size()), std::memory_order_relaxed);

        {
            std::unique_lock<std::mutex> lock(mutex_);
            current_batch_ = &batch;
            generation_++;
        }
        cv_work_.notify_all();

        // Calling thread participates in work
        process_batch(&batch);

        // Wait until all workers have completely finished with this batch
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_done_.wait(lock, [&batch]() {
                return batch.remaining_workers.load(std::memory_order_acquire) == 0;
            });
            current_batch_ = nullptr;
        }
    }

private:
    struct Batch {
        void* func_ptr{nullptr};
        void (*invoke)(void*, int){nullptr};
        std::atomic<int> next_idx{0};
        int total{0};
        int chunk_size{1};
        std::atomic<int> remaining_workers{0};
    };

    static void process_batch(Batch* batch) {
        const int chunk = batch->chunk_size;
        const int total = batch->total;
        while (true) {
            int start = batch->next_idx.fetch_add(chunk, std::memory_order_relaxed);
            if (start >= total)
                break;
            int end = std::min(start + chunk, total);
            for (int i = start; i < end; ++i) {
                batch->invoke(batch->func_ptr, i);
            }
        }
    }

    void worker_loop() {
        uint64_t seen_gen = 0;
        while (true) {
            Batch* batch = nullptr;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_work_.wait(lock,
                              [this, &seen_gen]() { return stop_ || generation_ != seen_gen; });
                if (stop_)
                    return;
                seen_gen = generation_;
                batch = current_batch_;
            }

            if (batch) {
                process_batch(batch);
                if (batch->remaining_workers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_done_.notify_one();
                }
            }
        }
    }

    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable cv_work_;
    std::condition_variable cv_done_;
    bool stop_{false};
    uint64_t generation_{0};
    Batch* current_batch_{nullptr};
};

} // namespace prism::core
