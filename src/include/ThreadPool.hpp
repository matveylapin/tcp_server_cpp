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
    std::queue<std::pair<int, std::function<void()>>> job_queue_;
    std::mutex jobs_queue_mutex_;
    std::mutex treads_vector_mutex_;
    std::condition_variable condition_threads_;

    void ThreadLoop();
    void setup(uint32_t threads_count);
public:

    ThreadPool(uint32_t threads_count = std::thread::hardware_concurrency());

    bool isBusy();

    void addJob(const std::function<void()>& job);
    void addJob(const std::function<void()>& job, int jobs_id);
    void run(uint32_t threads_count = std::thread::hardware_concurrency());
    void terminate();
    void join();
    void join(int jobs_id);
};