#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
private:

    volatile bool terminate_ = true;

    std::vector<std::thread> thread_pool_;
    std::queue<std::pair<int, std::function<void()>>> job_queue_;
    std::mutex jobs_queue_mutex_;
    std::condition_variable condition_threads_;

    void ThreadLoop();
    void setup(uint32_t threads_count);
public:

    ThreadPool(uint32_t threads_count = std::thread::hardware_concurrency());

    bool isBusy();

    void addJob(const std::function<void()>& job);
    void addJob(const std::function<void()>& job, int job_id);
    bool isJobInQueue(int job_id);

    void run(uint32_t threads_count = std::thread::hardware_concurrency());
    void terminate();
    void join();
    void join(int job_id);
};