/**
 * 演示调度器的使用方式:  将 M 个 task 分配到 N 的线程上执行, 其中 M >> N.
 *
 * 另一个例子: asyncxx::Scheduler 还可以轻松地实现层序遍历.
 *     asyncxx::Scheduler s;
 *     auto bfs(Node *node) -> asyncxx::Scheduler::Task {
 *         std::cout << node->value;
 *         s.enqueue(bfs(node->left));
 *         s.enqueue(bfs(node->right));
 *         co_return;
 *     }
 *     s.enqueue(bfs(root));
 *     s.run();
 */
#include "asyncxx/Scheduler.hpp"
#include <print>
#include <thread>
#include <atomic>
#include <cstdlib>

asyncxx::Scheduler s1, s2;
auto num_tasks = std::atomic_uint{4};  // 测试 4 个 task, 见后文.

auto task_factory(const int i) -> asyncxx::Scheduler::Task {
    std::println("{} -+       @ thread-{}", i, std::this_thread::get_id());

    co_await (std::rand() % 2 ? s1 : s2);
    // 这里发生了几件事:
    // 1. 调度器暂停执行该协程, 因此协程被挂起.
    // 2. 挂起时, 协程将自己后续的代码放到 `std::rand()%2 ? s1 : s2` 的任务队列中.
    // 3. 调度器从自己的任务队列中挑出一个继续执行 (如有).

    std::println("{} ---+     @ thread-{}", i, std::this_thread::get_id());

    co_await (std::rand() % 2 ? s1 : s2);

    std::println("{} -----+   @ thread-{}", i, std::this_thread::get_id());

    co_await (std::rand() % 2 ? s1 : s2);

    std::println("{} -------+ @ thread-{}", i, std::this_thread::get_id());

    num_tasks--;
}

int main() {
    // 创建几个 task:
    auto t1 = task_factory(1);
    auto t2 = task_factory(2);
    auto t3 = task_factory(3);
    auto t4 = task_factory(4);

    // 将 task 加入到 scheduler 中:
    s1.enqueue(t1);
    s2.enqueue(t2);
    s2.enqueue(t3);

    // 在两个线程上分别运行 scheduler::run().
    std::thread{[] { while (true) s1.run(); }}.detach();
    s1.enqueue(t4);  // s1 在 run 的过程中可以继续 enqueue, 没关系, 这是线程安全的.
    std::thread{[] { while (true) s2.run(); }}.detach();

    while (num_tasks != 0)
        std::this_thread::yield();
}
