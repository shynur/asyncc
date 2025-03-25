#include "asyncxx/Task.hpp"
#include <print>
#include <ranges>

Task completes_synchronously() {
    Logger{__PRETTY_FUNCTION__};
    co_return;
}

Task loop_synchronously(const std::size_t count) {
    Logger{__PRETTY_FUNCTION__};
    for (auto _ : std::views::iota(0u, count))
        co_await completes_synchronously();
}

int main() {
    ManualExecutor e;
    e.sync_wait(loop_synchronously(0));
}
