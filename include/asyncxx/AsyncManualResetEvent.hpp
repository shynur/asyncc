#pragma once
#include <atomic>
#include <coroutine>

namespace asyncxx {
    class AsyncManualResetEvent;
    struct TestAsyncManualResetEvent;
}

/**
 * @brief 异步的 可手动重置的 事件
 * @details 事件有 未完成 和 已完成 两种状态.
 *          协程会等待 (co_await) 未完成 的事件, 事件完成时会通知协程恢复执行;
 *          已完成 的事件不会阻塞协程.
 * @note  绝不会抛出异常.  没有堆分配.  无锁实现.
 */
class [[gnu::weak]] asyncxx::AsyncManualResetEvent {
    friend struct Awaiter;
    mutable std::atomic<void *> queue;

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

    class Awaiter {
        friend class AsyncManualResetEvent;
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
        void await_resume() noexcept {}
    };
    Awaiter operator co_await() const noexcept { return {*this}; }
};

/********************************* 用法示例 **********************************/

#include <chrono>
#include <print>
#include <ranges>
#include <thread>

struct [[gnu::weak]] asyncxx::TestAsyncManualResetEvent {
    // 样板代码:
    struct Task {
        struct promise_type {
            Task get_return_object() const { return {}; }
            std::suspend_never initial_suspend() const { return {}; }
            std::suspend_never final_suspend() const noexcept { return {}; }
            void return_void() const {}
            void unhandled_exception() const {}
        };
    };

    /*************************** 测试主体 *******************************/
    AsyncManualResetEvent get_number;
    int user_input;

    /**
     * @brief 测试函数, 直接调用即可完成测试.
     * @param user_input 用户假设一个最终的 I/O 结果.
     * @param num_consumers  测试的协程的数量.
     */
    void test(const int user_input, const unsigned num_consumers) noexcept {
#ifdef ASYNCXX_TEST_LOG
        std::println(">>>>>>>>>>> 开始测试: AsyncManualResetEvent >>>>>>>>>>>");
#endif

        for (auto _ : std::views::iota(0u, num_consumers))
            this->consumer();        // 先启动 消费者, 进行接收数据前的准备工作.
        this->producer(user_input);  // 再启动 生产者, 生产者会 notify 消费者.

#ifdef ASYNCXX_TEST_LOG
        std::println("<<<<<<<<<<<<<<<<<<<<<<< 测试结束 <<<<<<<<<<<<<<<<<<<<<<");
#endif
    }

    void producer(const int user_input) noexcept {
#ifdef ASYNCXX_TEST_LOG
        std::println("等待用户输入...");
#endif

        std::this_thread::sleep_for(std::chrono::seconds{1}); // 模拟等待 I/O 的耗时
        this->user_input = user_input;

#ifdef ASYNCXX_TEST_LOG
        std::println("已取得输入, 准备发布消息");
#endif

        this->get_number.set();  // 将 get_number 事件设为 已完成 的状态.
    }

    Task consumer() const noexcept {
#ifdef ASYNCXX_TEST_LOG
        std::println("我要取数字");
#endif

        co_await this->get_number; // 等待 get_number 事件完成

#ifdef ASYNCXX_TEST_LOG
        std::println("取到了数字 {}", this->user_input);
#endif
    }
};
