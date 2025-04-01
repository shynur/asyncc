#include "asyncxx/AsyncManualResetEvent.hpp"
#include <print>
#include <ranges>
#include <iostream>
#include <future>

/* >>>>>>>>>>>>>>>>>>>>>>>>> 用法示例 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
asyncxx::AsyncManualResetEvent get_number;  // 监听用户输入的事件
int user_input;  // 用户输入的数字

// 样板代码:
struct SomeTaskName {
    struct promise_type {
        SomeTaskName get_return_object() const noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void return_void() const noexcept {}
        void unhandled_exception() const noexcept {}
    };
};
SomeTaskName consumer() {
    std::println("{} 要取数字", __func__);
    co_await get_number; // 等待 get_number 事件完成
    std::println("{} 取到了数字 {}", __func__, user_input);
}

void producer() {
    std::println("{} 等待用户输入...", __func__);
    std::cout << "请输入一个整数: ";
    std::cin >> user_input;
    std::println("{} 已取得输入, 准备发布消息", __func__);
    get_number.set();  // 将 get_number 事件设为 已完成 的状态.
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
    const auto producing = std::async(producer);

    // 启动 消费者:
    for (auto _ : std::views::iota(0u, num_cor))
        consumer();
    // 前几次循环时, 生产者还没有收到数据, 此时消费者可以先进行接收数据前的准备工作,
    // 然后 co_await; 从某次循环开始, 用户输入了数据, 此时消费者内部串行执行, 无需等待.

    producing.wait();
    std::println("<<<<<<<<<<<<<<<<<<<<<<< 测试结束 <<<<<<<<<<<<<<<<<<<<<<");
}
