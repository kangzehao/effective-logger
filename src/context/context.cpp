#include "context.h"
namespace logger {
namespace ctx {

class ExecutorManager {
public:
    ExecutorManager() : executor_(std::make_unique<Executor>()) {}

    ~ExecutorManager() {
        if (executor_) {
            executor_.reset();
        }
    }
    Executor* GetExecutor() const {
        return executor_.get();
    }

    TaskRunnerTag CreateNewTaskRunner() const {
        return executor_->AddTaskRunner();
    }

private:
    std::unique_ptr<Executor> executor_;
};

Context::Context() : executor_manager_(std::make_unique<ExecutorManager>()) {}

Context::~Context() {
    if (executor_manager_) {
        executor_manager_.reset();
    }
}

Executor* Context::GetExecutor() const {
    return executor_manager_->GetExecutor();
}

TaskRunnerTag Context::CreateNewTaskRunner() const {
    return executor_manager_->CreateNewTaskRunner();
}
}  // namespace ctx
}  // namespace logger
