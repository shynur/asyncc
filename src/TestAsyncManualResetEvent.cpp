/**
 * 这个示例用到的线程数由 C++ 库定义.  它可能是双线程的.
 * 那么一开始, 生产者跑在一个线程上, 消费者们跑在另一个线程上.
 *
 * 当消费者遇到 co_await 时会查看生产者有没有拿到用户输入:
 * - 如果没有, 那么这个消费者会被挂起, 并将自身的执行权转交给生产者.
 * - 如果有, 那么这个消费者就会查看用户输入, 并继续执行.
 *
 * 在生产者拿到用户输入后, 会帮某些消费者执行剩下的代码, 因为这些消费者
 * 把执行权交给了它.
 *
 * 综上, 一部分消费者始终在原有线程上执行,
 * 另一部分消费者挂起后在别的线程上恢复执行.
 *
 * 所以整个过程无需加锁, 并且无论从生产者还是消费者的角度看,
 * 自身的函数体都是串行执行的.
 */

#include "asyncxx/AsyncManualResetEvent.hpp"
#include <print>
#include <ranges>
#include <iostream>
#include <future>
#include <thread>
#include <chrono>

/* >>>>>>>>>>>>>>>>>>>>>>>>> 用法示例 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
struct Producer {
    asyncxx::AsyncManualResetEvent<int> get_number;  // 监听用户输入的事件
    void produce() {
        std::println(
            "\n  {}@thread-{} 等待用户输入...",
            __func__, std::this_thread::get_id()
        );

        int user_input;
        std::cout << "请输入一个整数!!!!!!! (随便什么时候输入) \n";
        std::cin >> user_input;

        std::println(
            "  {}@thread-{} 已取得输入, 准备发布消息",
            __func__, std::this_thread::get_id()
        );
        this->get_number.set(user_input);  // 将 get_number 事件设为 已完成 的状态.
    }
} producer;

struct SomeTaskName {
    // 样板代码:
    struct promise_type {
        SomeTaskName get_return_object() const noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void return_void() const noexcept {}
        void unhandled_exception() const noexcept {}
    };
};
SomeTaskName consume(const unsigned id) {
    std::println(
        "\n{}-{}@thread-{} 要取数字",
        __func__, id, std::this_thread::get_id()
    );

    auto number = co_await producer.get_number; // 等待 get_number 事件完成
    //     悬挂点 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    std::println(
        "{}-{}@thread-{} 取到了数字 {}",
        __func__, id, std::this_thread::get_id(),
        number
    );
}
/* <<<<<<<<<<<<<<<<<<<<<<<<<<< 用法示例 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

/**
 * @brief 直接执行即可测试, 根据提示输入两个数字.
 */
int main() {
    std::cout << "要测试的协程的数量: ";
    unsigned num_cor;
    std::cin >> num_cor;

    std::println(">>>>>>>>>>> 开始测试: AsyncManualResetEvent >>>>>>>>>>>");

    // 异步启动生产者 (无所谓它在什么时候运行):
    const auto producing = std::async([] {producer.produce();});

    // 启动 消费者:
    for (auto i : std::views::iota(0u, num_cor)) {
        consume(i+1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    // 前几次循环时, 生产者还没有收到数据, 此时消费者可以先进行接收数据前的准备工作,
    // 然后 co_await; 从某次循环开始, 用户输入了数据, 此时消费者内部串行执行, 无需等待.

    producing.wait();
    std::println("<<<<<<<<<<<<<<<<<<<<<<< 测试结束 <<<<<<<<<<<<<<<<<<<<<<");
}
