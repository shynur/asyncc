#pragma once
#include <atomic>
#include <coroutine>
#include <memory>
#include <concepts>
#include <utility>

namespace asyncxx { template <typename> class AsyncManualResetEvent; }

/**
 * @brief 异步的 可手动重置的 事件
 * @details 事件有 未完成 和 已完成 两种状态.
 *          协程会等待 (co_await) 未完成 的事件, 事件完成时会通知协程恢复执行;
 *          已完成 的事件不会阻塞协程.
 * @note  绝不会抛出异常.  没有堆分配.  无锁实现.
 * @tparam Value 与事件关联的元素的类型, 作为 co_await 的返回值.  可以是 void.
 */
template <typename Value>
class [[gnu::weak]] asyncxx::AsyncManualResetEvent {
    friend class Awaiter;
    mutable std::atomic<void *> queue;

    struct ValueBox{
        std::conditional_t<std::is_void_v<Value>, int, Value> value;
    };
    std::unique_ptr<ValueBox> value = nullptr;

  public:
    /**
     * @param  initial_state  事件的初始状态.  true/false 表示 已/未 完成.
     */
    AsyncManualResetEvent(const bool initial_state = false) noexcept
    : queue{initial_state ? this : nullptr} {}
    AsyncManualResetEvent(const AsyncManualResetEvent&) = delete;
    AsyncManualResetEvent& operator=(const AsyncManualResetEvent&) = delete;

    /**
     * @brief 获知事件的状态: true/false 表示 已/未 完成.
     */
    bool is_set() const noexcept {
        return this->queue.load(std::memory_order_acquire) == this;
    }
    /**
     * @brief 重置事件, 使其变为 未完成 的状态.
     * @note 仅当事件 已完成 时有效,
     *       因此在 未完成 时可以随意 reset 而不用担心丢失对 被挂起的协程 的跟踪.
     */
    void reset() noexcept {
        void *pself = this;
        this->queue.compare_exchange_strong(pself, nullptr, std::memory_order_acquire);
    }
    /**
     * @brief 将事件变为 已完成 状态, 通知所有等待的协程.
     * @note 事件已经处于 已完成 状态时, 不做任何事.
     */
    void set() noexcept {
        auto queue = this->queue.exchange(this, std::memory_order_acq_rel);
        if (queue == this)
            return;

        auto awaiter = (Awaiter *)queue;
        while (awaiter) {
            auto next = awaiter->next;
            awaiter->coro.resume();
            awaiter = next;
        }
    }
    /**
     * @brief 类似于 set(), 但可以传递值给 co_awaiting 的协程.
     */
    void set(std::convertible_to<Value> auto&& value) noexcept requires(!std::is_void_v<Value>) {
        this->value = std::make_unique<ValueBox>(std::forward<decltype(value)>(value));
        this->set();
    }

    class Awaiter {
        friend AsyncManualResetEvent;
        const AsyncManualResetEvent& event;
        std::coroutine_handle<> coro;
        Awaiter *next;

      public:
        Awaiter(const AsyncManualResetEvent& event) noexcept: event{event} {}

        bool await_ready() const noexcept { return this->event.is_set(); }
        bool await_suspend(std::coroutine_handle<> coro) noexcept {
            this->coro = coro;

            auto old_head = this->event.queue.load(std::memory_order_acquire);
            do {
                if (old_head == &this->event)
                    return false;
                this->next = (Awaiter *)old_head;
            } while (
                !this->event.queue.compare_exchange_weak(
                    old_head, this,
                    std::memory_order_release,
                    std::memory_order_acquire
                )
            );
            return true;
        }
        Value await_resume() noexcept {
            if constexpr (!std::is_void_v<Value>)
                return this->event.value->value;
        }
    };
    Awaiter operator co_await() const noexcept { return {*this}; }
};
