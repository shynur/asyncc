/**
 * @brief 将 main 函数改写为协程.
 * @note C++ 规定 main 必须是普通函数, 所以在此我们额外写了一个函数 async_main.
 */

#include "asyncxx/SyncTask.hpp"
#include <iostream>
#include <coroutine>

asyncxx::SyncTask<int> async_main(const int argc, const char *const argv[]) {
    (void)argc, (void)argv;
    std::cout << "Hello, world!" << std::endl;
    co_await std::suspend_never{};  // 也可以 co_await, 别把执行权转移回 sync_main 的调用方就行.
    co_return 233;
}

int main(const int argc, const char *const argv[]) {
    return async_main(argc, argv);
}
