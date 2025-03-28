#pragma once
#include <coroutine>
#include <type_traits>
#include <utility>

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() -> std::suspend_always { return {}; }
        void return_void() {}
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept {
            /* 对称转移到 continuation.  */
            struct FinalAwaiter: std::suspend_always {
                auto await_suspend(const std::coroutine_handle<promise_type> handle)
                -> std::coroutine_handle<> {
                    return handle.promise().continuation;
                }
            };
            return FinalAwaiter{};
        }
      private:
        friend Task;
        std::coroutine_handle<> continuation;
    };
  private:
    std::coroutine_handle<promise_type> cor;
    explicit Task(
        const std::coroutine_handle<promise_type> handle
    ) noexcept: cor{handle} {}
  public:
    Task(Task&& task) noexcept: cor{std::exchange(task.cor, {})} {}
    ~Task() {
        // RAII
        if (this->cor)
            this->cor.destroy();
    }

    auto operator co_await() && noexcept {
        struct Awaiter: std::suspend_always {
            Awaiter(
                const std::coroutine_handle<promise_type> handle
            ) noexcept: cor{handle} {}
            auto await_suspend(const std::coroutine_handle<> continuation) noexcept
            -> std::coroutine_handle<> {
                this->cor.promise().continuation = continuation;
                return this->cor;
            }
          private:
            const std::coroutine_handle<promise_type> cor;
        };
        return Awaiter{this->cor};
    }
};

struct T {
    struct promise_type {
        T get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        void return_void() {}
        void unhandled_exception() { throw; }
        std::suspend_never final_suspend() noexcept { return {}; }
    };
};
