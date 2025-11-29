#include <iostream>
#include <cassert>
#include <atomic>
#include <thread>

#include "internal_log.h"
#include "context/context.h"

using namespace logger::ctx;

void test_context_basic() {
    Context* ctx = Context::GetIntance();
    Executor* executor = ctx->GetExecutor();
    TaskRunnerTag tag = ctx->CreateNewTaskRunner();
    std::atomic<int> counter{0};
    executor->PostTask(tag, [&counter]() { counter++; });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(counter == 1);
    LOG_INFO("test_context_basic passed ");
}

void test_context_post_task_and_get_result() {
    Context* ctx = Context::GetIntance();
    Executor* executor = ctx->GetExecutor();
    TaskRunnerTag tag = ctx->CreateNewTaskRunner();
    auto fut = executor->PostTaskAndGetResult(tag, []() { return 123; });
    assert(fut.get() == 123);
    LOG_INFO("test_context_post_task_and_get_result passed ");
}

void test_context_delayed_task() {
    Context* ctx = Context::GetIntance();
    Executor* executor = ctx->GetExecutor();
    TaskRunnerTag tag = ctx->CreateNewTaskRunner();
    std::atomic<int> counter{0};
    executor->PostDelayedTask(tag, [&counter]() { counter++; }, std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    assert(counter == 1);
    LOG_INFO("test_context_delayed_task passed ");
}

void test_context_repeated_task() {
    Context* ctx = Context::GetIntance();
    Executor* executor = ctx->GetExecutor();
    TaskRunnerTag tag = ctx->CreateNewTaskRunner();
    std::atomic<int> counter{0};
    executor->PostRepeatedTask(tag, [&counter]() { counter++; }, std::chrono::milliseconds(30), 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    assert(counter == 5);
    LOG_INFO("test_context_repeated_task passed");
}

void test_context_cancel_repeated_task() {
    Context* ctx = Context::GetIntance();
    Executor* executor = ctx->GetExecutor();
    TaskRunnerTag tag = ctx->CreateNewTaskRunner();
    std::atomic<int> counter{0};
    auto id = executor->PostRepeatedTask(tag, [&counter]() { counter++; }, std::chrono::milliseconds(50), 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    executor->CancelRepeatedTask(id);
    int val = counter.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert(counter == val);  // 取消后计数不再增加
    LOG_INFO("test_context_cancel_repeated_task passed ");
}

int main() {
    test_context_basic();
    test_context_post_task_and_get_result();
    test_context_delayed_task();
    test_context_repeated_task();
    test_context_cancel_repeated_task();
    LOG_INFO("All context tests passed! ");
    return 0;
}
