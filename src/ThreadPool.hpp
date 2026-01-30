#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <Task.hpp>
#include <iostream>
#include <queue>
#include <thread>
#include <vector>

#include "BlockingQueue.hpp"
#include "ClientConnection.hpp"

class ThreadPool {
   private:
    int workersCount;
    BlockingQueue<Task> clientsQueue;
    std::vector<std::thread> workers;
    std::mutex queueMutex;

    void workerLoop() {
        Task task;
        while (true) {
            if (!clientsQueue.pop(task)) {
                break;
            }
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "ThreadPoool::workerLoop(): Error, task execution "
                             "failed\n";
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
};

#endif