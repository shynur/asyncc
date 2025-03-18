#include <atomic>
#include <coroutine>
using namespace std::literals;

namespace asyncc {
    class AsyncManualResetEvent;
    struct TestAsyncManualResetEvent;
}

/**
 * @brief 异步的 可手动重置的 事件
 * @details 事件有 未完成 和 已完成 两种状态.  协程会等待 未完成 的事件, 事件完成时会通知协程恢复执行; 已完成 的事件不会阻塞协程.
 * @note 绝不会抛出异常.  没有堆分配.  无锁实现.
 */
class [[gnu::weak]] asyncc::AsyncManualResetEvent {
    mutable std::atomic<void *> queue;
    public:
    /**
     * @param settled  事件的初始状态.  true/false 表示 已/未 完成.
     */
    AsyncManualResetEvent(const bool settled = false) noexcept: queue{settled ? this : nullptr} {}
    AsyncManualResetEvent(const AsyncManualResetEvent&) = delete;
    AsyncManualResetEvent& operator=(const AsyncManualResetEvent&) = delete;

    /**
     * @brief 获知事件的状态: true/false 表示 已/未 完成.
     */
    bool is_set() const noexcept { return this->queue.load(std::memory_order_acquire) == this; }
    /**
     * @brief 重置事件, 使其变为 未完成 的状态.
     * @note 仅当事件 已完成 时有效, 因此在 未完成 时可以随意 reset 而不用担心丢失对 被挂起的协程 的跟踪.
     */
    void reset() noexcept {
        void *pself = this;
        this->queue.compare_exchange_strong(pself, nullptr,
                                            std::memory_order_acquire);
    }
    /**
     * @brief 将事件变为 已完成 状态, 通知所有等待的协程.
     * @note 事件已经处于 已完成 状态时, 不做任何事.
     */
    void set() noexcept {
        void *first = this->queue.exchange(this, std::memory_order_acq_rel);
        if (first == this)
            return;

        for (auto *first = (Awaiter *)first, *next; awaiter; first = next) {
            next = first->next;
            first->awaiting.resume();
        }
    }

    class Awaiter {
        const AsyncManualResetEvent& event;
        std::coroutine_handle<> awaiting;
        Awaiter *next;
        public:
        Awaiter(const AsyncManualResetEvent& event) noexcept: event{event} {}

        bool await_ready() const noexcept { return this->event.is_set(); }
        bool await_suspend(std::coroutine_handle<> awaiting) noexcept {
            this->awaiting = awaiting;

            void *old_head = this->event.queue.load(std::memory_order_acquire);
            do {
                if (old_head == &this->event)
                    return false;
                this->next = (Awaiter *)old_head;
            } while (!this->event.queue.compare_exchange_weak(old_head, this,
                                                              std::memory_order_release,
                                                              std::memory_order_acquire));
            return true;
        }
        void await_resume() noexcept {}
    }
        friend struct Awaiter;
    Awaiter operator co_await() const noexcept { return {*this}; }
};

/********************************* 用法示例 **********************************/

#include <thread>
#include <print>
#include <ranges>

struct [[gnu::weak]] asyncc::TestAsyncManualResetEvent {
    AsyncManualResetEvent get_number;
    int number, user_input;

    /**
     * @param user_input 用户假设一个最终的计算结果.
     * @param num_consumers  测试的协程的数量.
     */
    void test(int user_input, unsigned num_consumers) {
        this->user_input = user_input;

        std::println(">>>>>>>>>>> 开始测试: AsyncManualResetEvent >>>>>>>>>>>");
        for (auto _ : std::views::iota(0) | std::views::take(num_consumers))
            this->consumer();
        this->producer();
        std::println("<<<<<<<<<<<<<<<<<<<<<<< 测试结束 <<<<<<<<<<<<<<<<<<<<<<");
    }

    void producer() {
        std::println("开始计算, 完成后会把结果放到 number 里");
        std::this_thread::sleep_for(1s);
        this->number = this->user_input;

        std::println("发布消息");
        this->event.set();
    }

    Task consumer() const {
        std::println("我要取数字");
        co_await this->get_number;  // 等待取数字的事件
        std::println("取到了数字 {}", this->number);
    }

    struct Task {
        struct promise_type {
            Task get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };
    };
};

/*
 * int main() {
 *     asyncc::TestAsyncManualResetEvent{}.test(
 *         233,
 *         9999  // 无栈协程支持百万级并发
 *     );
 * }
 */
