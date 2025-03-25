#pragma once
#include <coroutine>
#include <iostream>
#include <utility>
#include <type_traits>
#include <ranges>

struct Logger {
    const std::string func;
    inline static unsigned depth = 0;
    Logger(const std::string func): func{func} {
        ++std::decay_t<decltype(*this)>::depth;
        for (auto _ : std::views::iota(0u, std::decay_t<decltype(*this)>::depth))
            std::cerr << "->";
        std::cerr << ' ' << func << '\n';
    }
    ~Logger() {
        for (auto _ : std::views::iota(0u, std::decay_t<decltype(*this)>::depth))
            std::cerr << "<-";
        std::cerr << ' ' << func << '\n';
        --std::decay_t<decltype(*this)>::depth;
    }
};

/**
 * @note 拥有 coroutine 所有权的 RAII 类.
 */
class Task {
  public:
    struct promise_type;
  private:
    std::coroutine_handle<promise_type> coro;
    explicit Task(std::coroutine_handle<promise_type> handle) noexcept: coro{handle} {
        auto logger = Logger{__PRETTY_FUNCTION__};
    }
  public:
    Task(Task&& task) noexcept: coro{std::exchange(task.coro, {})} {
        auto logger = Logger{__PRETTY_FUNCTION__};
    }
    ~Task() {
        auto logger = Logger{__PRETTY_FUNCTION__};
        if (this->coro)
            this->coro.destroy();
    }

    struct promise_type {
        std::coroutine_handle<> continuation;

        Task get_return_object() noexcept {
            auto logger = Logger{__PRETTY_FUNCTION__};
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() noexcept -> std::suspend_always {
            auto logger = Logger{__PRETTY_FUNCTION__};
            return {};
        }
        void return_void() noexcept {
            auto logger = Logger{__PRETTY_FUNCTION__};
        }
        void unhandled_exception() noexcept { throw; }
        auto final_suspend() noexcept {
            auto logger = Logger{__PRETTY_FUNCTION__};
            struct FinalAwaiter {
                bool await_ready() noexcept {
                    auto logger = Logger{__PRETTY_FUNCTION__};
                    return false;
                }
                void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                    auto logger = Logger{__PRETTY_FUNCTION__};
                    handle.promise().continuation.resume();
                }
                void await_resume() noexcept {
                    auto logger = Logger{__PRETTY_FUNCTION__};
                }
            };
            return FinalAwaiter{};
        }
    };

    auto operator co_await() && noexcept {
        auto logger = Logger{__PRETTY_FUNCTION__};
        class Awaiter {
            std::coroutine_handle<promise_type> coro;
            friend Task;
            explicit Awaiter(std::coroutine_handle<promise_type> handle) noexcept: coro{handle} {
                auto logger = Logger{__PRETTY_FUNCTION__};
            }
          public:
            bool await_ready() noexcept {
                auto logger = Logger{__PRETTY_FUNCTION__};
                return false;
            }
            void await_suspend(std::coroutine_handle<> continuation) noexcept {
                auto logger = Logger{__PRETTY_FUNCTION__};
                this->coro.promise().continuation = continuation;
                this->coro.resume();
            }
            void await_resume() noexcept {
                auto logger = Logger{__PRETTY_FUNCTION__};
            }
        };
        return Awaiter{this->coro};
    }
};

struct SyncWaitTask {
    struct promise_type {
        SyncWaitTask get_return_object() noexcept {
            auto logger = Logger{__PRETTY_FUNCTION__};
            return SyncWaitTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() noexcept -> std::suspend_never {
            auto logger = Logger{__PRETTY_FUNCTION__};
            return {};
        }
        void return_void() noexcept {
            auto logger = Logger{__PRETTY_FUNCTION__};
        }
        void unhandled_exception() noexcept { throw; }
        auto final_suspend() noexcept -> std::suspend_always {
            auto logger = Logger{__PRETTY_FUNCTION__};
            return {};
        }
    };

    std::coroutine_handle<promise_type> coro;

    explicit SyncWaitTask(std::coroutine_handle<promise_type> handle) noexcept: coro(handle) {
        auto logger = Logger{__PRETTY_FUNCTION__};
    }
    SyncWaitTask(SyncWaitTask&& task) noexcept: coro{std::exchange(task.coro, {})} {
        auto logger = Logger{__PRETTY_FUNCTION__};
    }
    ~SyncWaitTask() {
        auto logger = Logger{__PRETTY_FUNCTION__};
        if (this->coro)
            this->coro.destroy();
    }

    static auto start(Task&& task) -> SyncWaitTask {
        auto logger = Logger{__PRETTY_FUNCTION__};
        co_await std::move(task);
    }
    bool done() {
        auto logger = Logger{__PRETTY_FUNCTION__};
        return this->coro.done();
    }
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
        auto logger = Logger{__PRETTY_FUNCTION__};
        while (this->head != nullptr) {
            auto *item = this->head;
            this->head = item->next;
            item->continuation.resume();
        }
    }
    void sync_wait(Task&& task) {
        auto logger = Logger{__PRETTY_FUNCTION__};
        auto t = SyncWaitTask::start(std::move(task));
        while (!t.done()) {
            this->drain();
        }
    }
};
