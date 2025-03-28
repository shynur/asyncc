#include "asyncxx/Task.hpp"
#include <iostream>

Task f() {
    std::cout << 'A' << '\n';
    co_return;
}

int main() {
    [] -> T {
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
}
