#include "asyncxx/Task.hpp"
#include <coroutine>
#include <iostream>

Task f() {
    std::cout << 'A' << '\n';
    co_await std::suspend_always{};
    std::cout << 'B' << '\n';
    co_return;
}

int main() {
}
