#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "BlockingQueue.hpp"
#include "TaskInterface.hpp"

// Fixed-size thread pool for concurrent task execution.
// Spawns worker threads that dequeue tasks from a blocking queue
// and execute them. Supports graceful shutdown via queue.stop().
class ThreadPool {
   private:
    int workersCount;
    // Blocking queue holding tasks to be executed
    BlockingQueue<std::unique_ptr<Task>> clientsQueue;
    // Worker threads managed by pool lifetime
    std::vector<std::thread> workers;

    // Worker thread entry point. Continuously dequeues and executes tasks
    // until queue.stop() is called (pop returns false).
    void workerLoop() {
        std::unique_ptr<Task> task;
        while (true) {
            // pop() returns false when queue is stopped
            if (!clientsQueue.pop(task)) {
                break;
            }
            try {
                // Execute polymorphic task (e.g., clientConnection)
                task->execute();
            } catch (const std::exception& e) {
                std::cerr << "ThreadPool::workerLoop(): Error, task execution "
                             "failed: "
                          << e.what() << "\n";
            }
        }
    }

   public:
    explicit ThreadPool(int workersCount) : workersCount(workersCount) {
        for (int i = 0; i < workersCount; i++) {
            workers.emplace_back([this]() { this->workerLoop(); });
        }
    }

    ~ThreadPool() {
        clientsQueue.stop();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Template method to enqueue tasks of any type derived from Task.
    // Perfect forwarding enables efficient handling of move-only types.
    template <typename TaskType>
    void enqueueTask(TaskType&& task) {
        auto taskPtr = std::make_unique<std::decay_t<TaskType>>(
            std::forward<TaskType>(task));
        clientsQueue.push(std::move(taskPtr));
    }
};

#endif