#include "asyncxx/Task.hpp"
#include <print>
#include <ranges>

Task completes_synchronously() {
    co_return;
}

Task loop_synchronously(const std::size_t count) {
    for (auto _ : std::views::iota(0u, count))
        co_await completes_synchronously();
}

int main() {
    ManualExecutor e;
    e.sync_wait(loop_synchronously(111122110));
}
