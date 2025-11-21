#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <cassert>

#include "internal_log.h"
#include "context/thread_pool.h"

using namespace logger;

void test_simple_task() {
    ThreadPool pool(2);
    auto fut = pool.SubmitWithFuture([]() { return 42; });
    assert(fut.get() == 42);

    LOG_INFO("test_simple_task passed\n");
}

void test_multiple_tasks() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;
    for (int i = 0; i < 10; ++i) {
        results.push_back(pool.SubmitWithFuture([i]() { return i * i; }));
    }
    for (int i = 0; i < 10; ++i) {
        assert(results[i].get() == i * i);
    }

    LOG_INFO("test_multiple_tasks passed\n");
}

void test_parallel_increment() {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> tasks;
    for (int i = 0; i < 100; ++i) {
        tasks.push_back(pool.SubmitWithFuture([&counter]() { counter++; }));
    }
    for (auto& f : tasks) f.get();
    assert(counter == 100);

    LOG_INFO("test_parallel_increment passed\n");
}

void test_submit_void_task() {
    ThreadPool pool(2);
    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        pool.Submit([&counter]() { counter++; });
    }
    // 等待所有任务执行完毕
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(counter == 10);
    LOG_INFO("test_submit_void_task passed\n");
}

int main() {
    test_simple_task();
    test_multiple_tasks();
    test_parallel_increment();
    test_submit_void_task();
    LOG_INFO("All ThreadPool tests passed!\n");
    return 0;
}
