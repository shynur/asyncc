#pragma once
#include <coroutine>
#include <type_traits>
#include <utility>
#include <cassert>

struct Task: std::coroutine_handle<> {
    struct promise_type {
        Task get_return_object() {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        auto initial_suspend() -> std::suspend_always { return {}; }
        void return_void() {}
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept {
            /* 对称转移到 continuation.  */
            struct FinalAwaiter: std::suspend_always {
#ifndef NDEBUG
                const promise_type *const promise;
                FinalAwaiter(const promise_type *const promise): promise{promise} {}
#endif
                auto await_suspend(const std::coroutine_handle<promise_type> handle)
                -> std::coroutine_handle<> {
                    assert(&handle.promise() == this->promise);
                    return handle.promise().continuation;
                }
            };
            return FinalAwaiter{
#ifndef NDEBUG
                this
#endif
            };
        }
      private:
        friend Task;
        std::coroutine_handle<> continuation;
    };
    explicit Task(const std::coroutine_handle<promise_type> handle)
    : coroutine_handle<>{handle} {}
  public:
    Task(Task&& other) noexcept
    : coroutine_handle<>{std::exchange<coroutine_handle<>>(other, {})} {}
    ~Task() {
        if (*this)  // RAII
            this->destroy();
    }

    auto operator co_await() && {
        struct Awaiter: std::suspend_always {
            Awaiter(const std::coroutine_handle<> task) noexcept
            : task{std::coroutine_handle<promise_type>::from_address(task.address())} {}
            auto await_suspend(const std::coroutine_handle<> continuation) noexcept
            -> std::coroutine_handle<> {
                this->task.promise().continuation = continuation;
                return this->task;
            }
          private:
            const std::coroutine_handle<promise_type> task;
        };
        return Awaiter{*this};
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
