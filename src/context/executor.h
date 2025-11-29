#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <unordered_set>

#include "thread_pool.h"

namespace logger {
namespace ctx {

using Task = std::function<void(void)>;
using RepeatedTaskID = uint64_t;
using TaskRunnerTag = uint64_t;

class Executor {
private:
    class ExecutorTimer {  // 定时器模块
    public:
        struct ScheduledTask {  // 计划任务单元
            std::chrono::time_point<std::chrono::high_resolution_clock> scheduled_time_point;
            Task task;
            RepeatedTaskID repeated_id;
            bool operator<(const ScheduledTask& other) const {
                return scheduled_time_point > other.scheduled_time_point;  // 时间晚的优先级小于时间早的, 最小堆
            }
        };

    public:
        ExecutorTimer();
        ~ExecutorTimer();

        // 禁用拷贝/移动
        ExecutorTimer(const ExecutorTimer& other) = delete;
        ExecutorTimer& operator=(const ExecutorTimer& other) = delete;

        ExecutorTimer(ExecutorTimer&& other) = delete;
        ExecutorTimer& operator=(ExecutorTimer&& other) = delete;

        bool Start();
        void Stop();

        void PostDelayedTask(Task task, const std::chrono::microseconds& delayed_time);

        RepeatedTaskID PostRepeatedTask(Task task,
                                        const std::chrono::microseconds& interval__time,
                                        uint64_t repeat_num);

        void CancelRepeatedTask(RepeatedTaskID task_id);

    private:
        void Run();

        void PostRepeatedTask(Task task,
                              const std::chrono::microseconds& interval__time,
                              RepeatedTaskID repeated_task_id,
                              uint64_t repeat_num);

        RepeatedTaskID GetNextRepeatedTaskID() {
            return repeated_task_id_++;
            // 左++ 右++ 都可以 目的是让ID唯一
        }

    private:
        std::priority_queue<ScheduledTask> scheduled_task_queue_;
        std::mutex scheduled_task_queue_mutex_;

        std::condition_variable cv_;
        std::atomic<bool> is_running_;

        std::unique_ptr<ThreadPool> thread_pool_ptr_;

        std::atomic<RepeatedTaskID> repeated_task_id_;

        // 任务ID存储表
        std::unordered_set<RepeatedTaskID> repeated_task_id_set_;
        std::mutex repeated_task_id_set_mutex_;
    };

    class TaskRunnerManager {
        using TaskRunner = ThreadPool;
        using TaskRunnerPtr = std::unique_ptr<TaskRunner>;
        friend class Executor;

    public:
        TaskRunnerManager();
        ~TaskRunnerManager() = default;

        // 禁用拷贝移动
        TaskRunnerManager(const TaskRunnerManager& other) = delete;
        TaskRunnerManager& operator=(const TaskRunnerManager& other) = delete;
        TaskRunnerManager(TaskRunnerManager&& other) = delete;
        TaskRunnerManager& operator=(TaskRunnerManager&& other) = delete;

        TaskRunnerTag AddTaskRunner();

    private:
        TaskRunner* GetTaskRunner(const TaskRunnerTag& tag);

        TaskRunnerTag GetNextTaskRunnerTag() {
            return task_tag_++;
        }

    private:
        std::mutex task_runner_map_mutex_;
        std::atomic<TaskRunnerTag> task_tag_;
        std::unordered_map<uint64_t, TaskRunnerPtr> task_runner_map_;
    };

public:
    Executor();
    ~Executor();

    // 禁用拷贝 移动
    Executor(const Executor& other) = delete;
    Executor& operator=(const Executor& other) = delete;
    Executor(Executor&& other) = delete;
    Executor& operator=(Executor&& other) = delete;

    TaskRunnerTag AddTaskRunner();

    // 提交普通任务
    void PostTask(const TaskRunnerTag& runner_tag, Task task);

    // 提交待返回值任务
    template <typename F, typename... Args>
    auto PostTaskAndGetResult(const TaskRunnerTag& runner_tag, F&& f, Args&&... argc)
            -> std::future<std::invoke_result_t<F, Args...>> {
        auto runner = task_runner_manager_->GetTaskRunner(runner_tag);
        if (!runner) {
            throw std::runtime_error("TaskRunner not found for tag: " + runner_tag);
        }

        return runner->SubmitWithFuture(std::forward<F>(f), std::forward<Args>(argc)...);
    }

    // 提交延时任务
    template <typename R, typename P>
    void PostDelayedTask(const TaskRunnerTag& runner_tag, Task task, const std::chrono::duration<R, P>& delayed_time) {
        // 定时器不执行任务， 可以把提交任务的动作放在定时器，任务的运行放在runner中
        Task delayed_post_task = [this, runner_tag, task]() { PostTask(runner_tag, std::move(task)); };

        executor_timer_->Start();
        executor_timer_->PostDelayedTask(std::move(delayed_post_task),
                                         std::chrono::duration_cast<std::chrono::microseconds>(delayed_time));
    }

    // 提交定时循环任务
    template <typename R, typename P>
    RepeatedTaskID PostRepeatedTask(const TaskRunnerTag& runner_tag,
                                    Task task,
                                    const std::chrono::duration<R, P>& interval__time,
                                    uint64_t repeat_num) {
        Task Repeated_post_task = [this, runner_tag, task]() { PostTask(runner_tag, std::move(task)); };
        // LOG_DEBUG("runner_tag: {}, interval__time: {}", runner_tag, interval__time);
        executor_timer_->Start();
        return executor_timer_->PostRepeatedTask(std::move(Repeated_post_task),
                                                 std::chrono::duration_cast<std::chrono::microseconds>(interval__time),
                                                 repeat_num);
    }

    // 关闭定时运行任务
    void CancelRepeatedTask(RepeatedTaskID task_id) {
        executor_timer_->CancelRepeatedTask(task_id);
    }

private:
    std::unique_ptr<TaskRunnerManager> task_runner_manager_;
    std::unique_ptr<ExecutorTimer> executor_timer_;
};
}  // namespace ctx
}  // namespace logger