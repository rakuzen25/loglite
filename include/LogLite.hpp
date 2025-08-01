#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <format>
#include <fstream>
#include <mutex>
#include <queue>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>

namespace LogLite {
template <typename T>
class ThreadSafeQueue {
   public:
    void push(T value) {
        // Lock the mutex to ensure thread safety
        // A lock_guard automatically unlocks when it goes out of scope
        std::lock_guard<std::mutex> lock(m_mutex);
        // Push the value into the queue
        m_queue.push(std::move(value));
        // Notify one waiting thread that a new item is available
        m_cond.notify_one();
    }

    T wait_and_pop() {
        // Acquire a unique lock because we want to manually unlock it based on
        // the condition
        std::unique_lock<std::mutex> lock(m_mutex);
        // Wait until the queue is not empty
        m_cond.wait(lock, [this] { return !m_queue.empty(); });
        // Get the front item from the queue
        T value = std::move(m_queue.front());
        m_queue.pop();
        // Return the popped value
        return value;
    }

    bool try_pop(T& value) {
        // Lock the mutex to ensure thread safety
        std::lock_guard<std::mutex> lock(m_mutex);
        // Check if the queue is empty
        if (m_queue.empty()) {
            return false;
        }
        // Move the front item into the provided reference
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

   private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class Logger {
   public:
    // Disable copy constructor
    // Logger objects should not be copied (e.g. Logger logger2 = logger1;)
    Logger(const Logger&) = delete;

    // Disable assignment operator
    // Logger objects should not be assigned (e.g. logger1 = logger2;)
    Logger& operator=(const Logger&) = delete;

    static Logger& instance() {
        // Use a static local variable to ensure that the logger is created only
        // once and is destroyed when the program exits
        static Logger logger;
        return logger;
    }

    // 1. Args and args are "variadic template" that allows the function to
    // accept any number of arguments (similar to *args in Python and ...args in
    // JavaScript)
    // 2. std::format_string<Args...> is a C++20 feature that allows formatted
    // strings with type safety (similar to f-strings in Python and template
    // literals in JavaScript)
    // 3. && is a forwaring reference that perfectly forwards both lvalues
    // (declared variables in memory) and rvalues (transient values like
    // literals and function return values) without unnecessary copies
    template <typename... Args>
    void log(const std::format_string<Args...> fmt, Args&&... args) {
        // std::source_location is a C++20 feature that provides information
        // about the source code location (similar to stack trace in Python and
        // Error.stack in JavaScript)
        const std::source_location& loc = std::source_location::current();
        auto now = std::chrono::system_clock::now();
        // Format the user message
        std::string user_message =
            std::format(fmt, std::forward<Args>(args)...);
        // Add time and stack trace information
        std::string full_message =
            std::format("[{:%Y-%m-%d %H:%M:%S}] [{}:{}] {}", now,
                        loc.file_name(), loc.line(), user_message);
        // Push the formatted message to the thread-safe queue
        m_queue.push(std::move(full_message));
    }

   private:
    // Open the log file in append mode
    Logger() : m_log_file("log.txt", std::ios::app) {
        // Start the background writer thread
        // A jthread is used to automatically join the thread when it goes out
        // of scope
        m_writer_thread = std::jthread(&Logger::writer_loop, this);
    }
    ~Logger() {
        // Signal the writer thread to stop
        m_active = false;
        // Push a dummy message to wake up the writer thread if it's waiting
        m_queue.push("");
        // Here, jthread will automatically join, no need to call join()
    }
    void writer_loop() {
        while (m_active) {
            // Pop messages from the queue
            std::string message = m_queue.wait_and_pop();
            if (!message.empty()) {
                // Write the message to the log file
                m_log_file << message << std::endl;
            }
        }

        // On shutdown, drain remaining messages in the queue
        std::string message;
        while (m_queue.try_pop(message)) {
            if (!message.empty()) {
                m_log_file << message << std::endl;
            }
        }
    }

    std::atomic<bool> m_active{true};
    ThreadSafeQueue<std::string> m_queue;
    std::ofstream m_log_file;
    std::jthread m_writer_thread;
};
}  // namespace LogLite
