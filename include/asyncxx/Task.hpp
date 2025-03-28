#pragma once
#include <atomic>
#include <coroutine>
#include <exception>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <utility>

/**
 * @note 拥有 coroutine 所有权的 RAII 类.
 */
class Task {
  public:
    struct promise_type;
  private:
    std::coroutine_handle<promise_type> coro;
    explicit Task(
        const std::coroutine_handle<promise_type> handle
    ) noexcept: coro{handle} {}
  public:
    Task(Task&& task) noexcept: coro{std::exchange(task.coro, {})} {}
    ~Task() {
        if (this->coro)
            this->coro.destroy();
    }

    struct promise_type {
        std::coroutine_handle<> continuation;
        std::atomic_flag        continuation_ready = false;

        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() noexcept { return false; }
                auto await_suspend(const std::coroutine_handle<promise_type> handle) noexcept
                    -> std::coroutine_handle<> {
                    return handle.promise().continuation;
                }
                void await_resume() noexcept {}
            };
            return FinalAwaiter{};
        }
    };

    auto operator co_await() && noexcept {
        class Awaiter {
            const std::coroutine_handle<promise_type> coro;
          public:
            explicit Awaiter(
                const std::coroutine_handle<promise_type> handle
            ) noexcept: coro{handle} {}
            bool await_ready() noexcept { return false; }
            auto await_suspend(const std::coroutine_handle<> continuation) noexcept
                -> std::coroutine_handle<> {
                this->coro.promise().continuation = continuation;
                return this->coro;
            }
            void await_resume() noexcept {}
        };
        return Awaiter{this->coro};
    }
};

struct SyncWaitTask {
    struct promise_type {
        SyncWaitTask get_return_object() noexcept {
            return SyncWaitTask{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }
    };
    std::coroutine_handle<promise_type> coro;
    explicit SyncWaitTask(
        std::coroutine_handle<promise_type> handle
    ) noexcept: coro(handle) {}
    SyncWaitTask(SyncWaitTask&& task) noexcept: coro{std::exchange(task.coro, {})} {}
    ~SyncWaitTask() {
        if (this->coro)
            this->coro.destroy();
    }

    static auto start(Task&& task) -> SyncWaitTask {
        co_await std::move(task);
    }
    bool done() { return this->coro.done(); }
};

struct ManualExecutor {
    struct ScheduleOp {
        ManualExecutor&         executor;
        ScheduleOp             *next = nullptr;
        std::coroutine_handle<> continuation;
        ScheduleOp(ManualExecutor& executor): executor{executor} {}
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            this->continuation  = continuation;
            this->next          = this->executor.head;
            this->executor.head = this;
        }
        void await_resume() noexcept {}
    };
    ScheduleOp *head = nullptr;
    auto        schedule() noexcept -> ScheduleOp {
        return {*this};
    }
    void drain() {
        while (this->head) {
            auto *item = this->head;
            this->head = item->next;
            item->continuation();
        }
    }
    void sync_wait(Task&& task) {
        auto sync_task = SyncWaitTask::start(std::move(task));
        while (!sync_task.done())
            this->drain();
    }
};
