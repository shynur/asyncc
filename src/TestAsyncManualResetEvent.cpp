//#define ASYNCXX_TEST_LOG "日志开关"
#include "asyncxx/AsyncManualResetEvent.hpp"

int main() {
    asyncxx::TestAsyncManualResetEvent{}.test(
        233,
        1'000'0000  // 无栈协程支持千万级并发
    );
}
