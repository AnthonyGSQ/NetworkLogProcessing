#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
template <typename T>
class BlockingQueue {
   private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopped{false};

   public:
    void push(T request) {
        // every time we touch the queue, we need to make sure that only one
        // thread have acccess to the queue, so we use a mutex
        {
            std::unique_lock<std::mutex> lock(mtx);
            queue.push(std::move(request));
        }
        cv.notify_one();
    }
    // every time we touch the queue, we need to make sure that only one
    // thread have acccess to the queue, so we use a mutex
    bool pop(T& request) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty() || stopped; });
        if (stopped && queue.empty()) {
            return false;  // stop condition
        }
        request = std::move(queue.front());
        queue.pop();
        return true;
    }
    // every time we touch the queue, we need to make sure that only one
    // thread have acccess to the queue, so we use a mutex
    void stop() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stopped = true;
        }
        cv.notify_all();
    }
};