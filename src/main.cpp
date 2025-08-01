#include <iostream>
#include <thread>
#include <vector>

#include "LogLite.hpp"

void worker_function(int thread_id) {
    for (int i = 0; i < 100; ++i) {
        LogLite::Logger::instance().log("Thread {} logging message #{}",
                                        thread_id, i);
    }
}

int main() {
    std::cout << "Starting logger test with multiple threads..." << std::endl;

    std::vector<std::jthread> threads;
    const int num_threads = 10;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_function, i);
    }

    // The jthreads will automatically join when `threads` goes out of scope.

    std::cout << "Test finished." << std::endl;

    return 0;
}