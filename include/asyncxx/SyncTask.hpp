/**
 * Author: 谢骐 <shynur@outlook.com>.
 */

#pragma once
#include <coroutine>
#include <type_traits>
#include <utility>

namespace asyncxx {
    template <typename Ret> requires (!std::is_void_v<Ret>)
    struct SyncTask;
}

/**
 * @brief 将普通函数直接改写为协程.
 * @tparam Ret 返回值类型, 不可以是 void.
 * @example
 * ```
 * int f() { return 42; }
 * ```
 * 可以改写为:
 * ```
 * asyncxx::SyncTask<int> g() { co_return 42; }
 * ```
 */
template <typename Ret> requires (!std::is_void_v<Ret>)
struct asyncxx::SyncTask {
    struct promise_type {
        auto get_return_object() -> asyncxx::SyncTask<Ret> {
            return {
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        auto initial_suspend() -> std::suspend_never { return {}; }
        void return_value(
            std::convertible_to<Ret> auto&& val  // 模仿 <https://itnext.io/daily-bit-e-of-c-coroutines-step-by-step-e726b976d239>
        ) {
            this->ret = new std::decay_t<decltype(*this->ret)>{
                std::forward<decltype(val)>(val)
            };
        }
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }

        struct {Ret value;} *ret = nullptr;
        ~promise_type() { delete this->ret; }
    };

    std::coroutine_handle<promise_type> cor;
    SyncTask(const std::coroutine_handle<promise_type> handle): cor(handle) {}
    ~SyncTask() { this->cor.destroy(); }

    operator Ret() { return this->cor.promise().ret->value; }
};
