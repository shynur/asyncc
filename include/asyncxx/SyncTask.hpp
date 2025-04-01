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
        void return_value(Ret&& val) {  // TODO: 针对任何 Ret 都可以这样写吗?
            this->ret = new RetBox{std::forward<Ret>(val)};
        }
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }

        struct RetBox {Ret value;} *ret = nullptr;
        ~promise_type() {
            if (this->ret)
                delete this->ret;
        }
    };

    std::coroutine_handle<promise_type> cor;
    SyncTask(const std::coroutine_handle<promise_type> handle): cor(handle) {}
    ~SyncTask() { this->cor.destroy(); }

    operator Ret() { return this->cor.promise().ret->value; }
};
