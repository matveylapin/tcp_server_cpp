#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
private:

    bool terminate_ = true;

    std::vector<std::thread> thread_pool_;
    std::queue<std::function<void()>> job_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_threads_;

    void ThreadLoop();
    void setup(uint32_t threads_count);
public:

    ThreadPool(uint32_t threads_count = std::thread::hardware_concurrency());

    bool isBusy();

    void addJob(const std::function<void()>& job);
    void run(uint32_t threads_count = std::thread::hardware_concurrency());
    void terminate();
    void join();
};