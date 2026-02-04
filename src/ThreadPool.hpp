#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "BlockingQueue.hpp"
#include "TaskInterface.hpp"

class ThreadPool {
   private:
    int workersCount;
    // queue with task objects, this objects can be (for example),
    // clientConnections
    BlockingQueue<std::unique_ptr<Task>> clientsQueue;
    std::vector<std::thread> workers;

    void workerLoop() {
        std::unique_ptr<Task> task;
        while (true) {
            // pop() retorna false cuando se llama stop()
            if (!clientsQueue.pop(task)) {
                break;
            }
            try {
                // Llama al método virtual execute() de la tarea
                // Polimorfismo: cada tipo de Task implementa execute() a su
                // manera
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

    // enqueueTask es template y acepta cualquier clase derivada de Task
    // Crea un unique_ptr y lo encola
    template <typename TaskType>
    void enqueueTask(TaskType&& task) {
        // std::make_unique crea un objeto en el heap y retorna un unique_ptr
        // Esto permite pasar objetos move-only (como tcp::socket)
        auto taskPtr = std::make_unique<std::decay_t<TaskType>>(
            std::forward<TaskType>(task));
        clientsQueue.push(std::move(taskPtr));
    }
};

#endif