#include "thread_pool.h"

namespace logger {

ThreadPool::ThreadPool(size_t pool_size) : is_running_(true), pool_size_(pool_size) {
    workers_vector_.reserve(pool_size);
    CreatePoolWorkers();
}

void ThreadPool::CreatePoolWorkers() {
    for (int i = 0; i < pool_size_; i++) {
        workers_vector_.emplace_back(std::move(std::thread([&]() {
            while (true) {
                std::function<void()> now_task;
                {
                    std::unique_lock<std::mutex> lock(mutex_task_que_);

                    cv_.wait(lock, [this]() { return !is_running_ || !task_que_.empty(); });

                    if (!is_running_) {
                        break;
                    }

                    now_task = std::move(task_que_.front());
                    task_que_.pop();
                }

                now_task();
            }
        })));
    }
}

void ThreadPool::DeletePoolWorkers() {
    if (is_running_) {
        return;
    }

    for (auto& worker : workers_vector_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

ThreadPool::~ThreadPool() {
    is_running_ = false;
    cv_.notify_all();
    DeletePoolWorkers();
}
}  // namespace logger
