#include "asyncxx/Task.hpp"
#include <iostream>

Task completes_synchronously() {
    std::cout << 'A' << '\n';
    co_await std::suspend_always{};
    std::cout << 'B' << '\n';
    co_return;
}

Task loop_synchronously() {
    std::cout << 1 << '\n';
    auto t = completes_synchronously();
    std::cout << 2 << '\n';
    auto u = completes_synchronously();
    std::cout << 3 << '\n';
    co_await std::move(u);
    std::cout << 4 << '\n';
    co_await std::move(t);
    std::cout << 5 << '\n';
}

int main() {
    std::cout << 6 << '\n';
    auto t = loop_synchronously();
    std::cout << 7 << '\n';
    [&] -> T {
        std::cout << 8 << '\n';
        co_await std::move(t);
        std::cout << 9 << '\n';
    }();
    std::cout << 10 << '\n';
}
