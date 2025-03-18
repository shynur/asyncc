#include "asyncxx/AsyncManualResetEvent.hpp"

int main() {
    asyncc::TestAsyncManualResetEvent{}.test(
        233,
        100'0000  // 无栈协程支持百万级并发
    );
}
