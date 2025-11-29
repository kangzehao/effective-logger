#include "thread_pool.h"

namespace logger {

ThreadPool::ThreadPool(size_t pool_size) : pool_size_(pool_size) {
    workers_vector_.reserve(pool_size);
    is_running_.store(false);
}

bool ThreadPool::Start() {
    if (is_running_.load()) {
        return false;
    }
    is_running_.store(true);
    CreatePoolWorkers();
    return true;
}

void ThreadPool::Stop() {
    if (!is_running_.load()) {
        return;
    }
    is_running_.store(false);
    cv_.notify_all();
    DeletePoolWorkers();
}

void ThreadPool::CreatePoolWorkers() {
    for (size_t i = 0; i < pool_size_; i++) {
        workers_vector_.emplace_back(std::move(std::thread([this]() {
            while (true) {
                std::function<void()> now_task;
                {
                    std::unique_lock<std::mutex> lock(mutex_task_que_);

                    cv_.wait(lock, [this]() { return !is_running_.load() || !task_que_.empty(); });

                    if (!is_running_.load()) {
                        break;
                    }

                    now_task = std::move(task_que_.front());
                    task_que_.pop();
                }

                if (now_task) {
                    now_task();
                }
            }
        })));
    }
}

void ThreadPool::DeletePoolWorkers() {
    if (is_running_.load()) {
        return;
    }

    for (auto& worker : workers_vector_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_vector_.clear();
}

ThreadPool::~ThreadPool() {
    Stop();
}
}  // namespace logger
