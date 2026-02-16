#ifndef CONNECTIONPOOL_HPP
#define CONNECTIONPOOL_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>

class ConnectionPool {
   public:
    ConnectionPool(const std::string& connInfo, size_t size) {
        for (size_t i = 0; i < size; i++) {
            pool.push(std::make_unique<pqxx::connection>(connInfo));
        }
    }
    std::unique_ptr<pqxx::connection> acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !pool.empty(); });

        auto conn = std::move(pool.front());
        pool.pop();
        return conn;
    }
    void release(std::unique_ptr<pqxx::connection> conn) {
        std::lock_guard<std::mutex> lock(mtx);
        pool.push(std::move(conn));
        cv.notify_one();
    }

   private:
    std::queue<std::unique_ptr<pqxx::connection>> pool;
    std::mutex mtx;
    std::condition_variable cv;
};

#endif