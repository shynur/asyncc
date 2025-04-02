/**
 * 生成器/co_yield 的示例.
 */
#include "asyncxx/Generator.hpp"
#include <string>
#include <print>

int main() {
    auto range = [] -> asyncxx::Generator<std::string> {
        auto s = std::string{"\t:"};
        while (true) {
            co_yield s;
            s += '!';
            if (s.size() > 10)
                co_return;
        }
    }();

    for (auto& s : range) {
        std::println("\n{}\n", s);
        s += "wq";
    }
}
