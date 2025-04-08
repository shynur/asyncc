#pragma once
#include <coroutine>
#include <list>
#include <mutex>

namespace asyncxx { struct Scheduler; }

/**
 * @brief 协程调度器 & 被调度的任务类型.
 * @note 线程安全
 */
struct [[gnu::weak]] asyncxx::Scheduler {
    /**
     * @brief 被调度的任务类型.
     * @warning Task 实例要么被 enqueue 到调度器中, 要么你手动调用
     *          destroy() 方法.  否则会导致内存泄漏.
     */
    struct Task: std::coroutine_handle<> {
        Task(const auto handle): coroutine_handle<>{handle} {}
        struct promise_type {
            auto get_return_object() {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }
            auto initial_suspend() const -> std::suspend_always { return {}; }
            void return_void() const {}
            void unhandled_exception() const { throw; }
            auto final_suspend() const noexcept -> std::suspend_always { return {}; }
        };
    };

    /**
     * @brief 将任务 `Scheduler::Task` 加入到调度器中的任务队列中.
     * @param task 要加入的任务.
     */
    void enqueue(const auto task) {
        const auto _ = std::lock_guard{this->cors_lock};
        this->cors.push_back(task);
    }

    /**
     * @brief 表示一个 executor, 会持续运行直到任务队列为空.
     * @note 该方法未必要在单独的线程中运行.
     */
    void run() {
        while (true) {
            std::coroutine_handle<> active;
            {
                const auto _ = std::lock_guard{this->cors_lock};
                if (this->cors.empty())
                    break;
                active = this->cors.front();
                this->cors.pop_front();
            }

            active.resume();

            if (active.done())
                active.destroy();
        }
    }

    /**
     * @brief 在被调度的函数体中 `co_await scheduler`, 即可挂起当前协程并将
     *        执行权转移给 scheduler.
     */
    auto operator co_await() {
        struct Yielder: std::suspend_always {
            void await_suspend(const std::coroutine_handle<> handle) {
                this->scheduler->enqueue(handle);
            }
            Scheduler *scheduler;
        };
        return Yielder{.scheduler = this};
    }

    ~Scheduler() {
        for (auto cor : this->cors)
            cor.destroy();
    }
  private:
    std::list<std::coroutine_handle<>> cors;
    std::mutex cors_lock;
};
