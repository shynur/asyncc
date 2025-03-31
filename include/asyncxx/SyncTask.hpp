#pragma once
#include <coroutine>
#include <type_traits>

namespace asyncxx {
    namespace __detail {
        template <typename Ret>
        struct SyncTask;
    }
    template <typename Ret>
    struct SyncTask;
}

template <typename Ret>
struct asyncxx::__detail::SyncTask {
    struct promise_type {
        auto get_return_object() -> SyncTask {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() -> std::suspend_never { return {}; }
        void unhandled_exception() { throw; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }

        struct RetBox {
            Ret value;
            RetBox(Ret val): value{val} {}
        } *ret = nullptr;
        ~promise_type() {
            if (this->ret)
                delete this->ret;
        }
    };

    std::coroutine_handle<promise_type> cor;
    SyncTask(const std::coroutine_handle<promise_type> handle): cor(handle) {}
    ~SyncTask() { this->cor.destroy(); }

    operator Ret() {
        return this->cor.promise().ret->value;
    }
};

template <typename Ret>
struct asyncxx::SyncTask: asyncxx::__detail::SyncTask<Ret> {
    void return_value(Ret val) {
        this->ret = new RetBox{val};
    }
};

template <>
struct asyncxx::SyncTask<void>: asyncxx::__detail::SyncTask<void> {
    void return_void() {}
};
