#pragma once
#include <coroutine>
#include <utility>

/**
 * @note 拥有 coroutine 所有权的 RAII 类.
 */
class Task {
  public:
    struct promise_type;
  private:
    std::coroutine_handle<promise_type> coro;
    explicit Task(std::coroutine_handle<promise_type> handle) noexcept: coro{handle} {}
  public:
    Task(Task&& task) noexcept: coro{std::exchange(task.coro, {})} {}
    ~Task() {
        if (this->coro)
            this->coro.destroy();
    }

    struct promise_type {
        std::coroutine_handle<> continuation;

        Task get_return_object() noexcept {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { throw; }
        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                    handle.promise().continuation.resume();
                }
                void await_resume() noexcept {}
            };
            return FinalAwaiter{};
        }
    };

    auto operator co_await() && noexcept {
        class Awaiter {
            std::coroutine_handle<promise_type> coro;
            friend Task;
            explicit Awaiter(std::coroutine_handle<promise_type> handle) noexcept: coro{handle} {}
          public:
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> continuation) noexcept {
                this->coro.promise().continuation = continuation;
                this->coro.resume();
            }
            void await_resume() noexcept {}
        };
        return Awaiter{this->coro};
    }
};

struct SyncWaitTask {
    struct promise_type {
        SyncWaitTask get_return_object() noexcept {
            return SyncWaitTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { throw; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }
    };

    std::coroutine_handle<promise_type> coro;

    explicit SyncWaitTask(std::coroutine_handle<promise_type> handle) noexcept: coro(handle) {}
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
        ScheduleOp(ManualExecutor& executor): executor(executor) {}
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> continuation) noexcept {
            this->continuation  = continuation;
            this->next          = this->executor.head;
            this->executor.head = this;
        }
        void await_resume() noexcept {}
    };
    ScheduleOp *head = nullptr;
    ScheduleOp schedule() noexcept {
        return ScheduleOp{*this};
    }
    void drain() {
        while (this->head != nullptr) {
            auto *item = this->head;
            this->head = item->next;
            item->continuation.resume();
        }
    }
    void sync_wait(Task&& task) {
        auto t = SyncWaitTask::start(std::move(task));
        while (!t.done())
            this->drain();
    }
};
