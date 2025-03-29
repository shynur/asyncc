#include "asyncxx/Task.hpp"
#include "asyncxx/Trivial.hpp"
#include <coroutine>
#include <iostream>

Task f() {
    std::cout << 'A' << '\n';
    co_await std::suspend_always{};
    std::cout << 'B' << '\n';
    co_return;
}

int main() {
    auto cor = [] -> TrivialTask {
        std::cout << 1 << '\n';
        auto t = f();
        std::cout << 2 << '\n';
        auto u = f();
        std::cout << 3 << '\n';
        co_await std::move(u);
        std::cout << 4 << '\n';
        co_await std::move(t);
        std::cout << 5 << '\n';
    }();
    while (!cor.done())
        cor.resume();
}
