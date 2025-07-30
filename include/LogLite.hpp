#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace LogLite {
template <typename T>
class ThreadSafeQueue {
   public:
    void push(T value) {
        // Lock the mutex to ensure thread safety
        std::lock_guard<std::mutex> lock(m_mutex);
        // Push the value into the queue
        m_queue.push(std::move(value));
        // Notify one waiting thread that a new item is available
        m_cond.notify_one();
    }

    T wait_and_pop() {
    }

   private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};
}  // namespace LogLite
