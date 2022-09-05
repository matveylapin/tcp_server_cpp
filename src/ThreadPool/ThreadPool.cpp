#include "ThreadPool.hpp"

ThreadPool::ThreadPool(uint32_t threads_count) { setup(threads_count); }

void ThreadPool::ThreadLoop()
{
    while (true)
    {
        std::function<void()> job;
        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (terminate_)
            return lock.unlock();

        condition_threads_.wait(lock, [this]
                                { return !job_queue_.empty(); });

        job = job_queue_.front();
        job_queue_.pop();

        lock.unlock();

        job();
    }
}

void ThreadPool::setup(uint32_t threads_count)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);

    terminate_ = false;
    thread_pool_.resize(threads_count);

    for (uint32_t i = 0; i < threads_count; i++)
        thread_pool_.at(i) = std::thread([this]
                                         { ThreadLoop(); });
}

bool ThreadPool::isBusy()
{
    bool poolBusy;
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        poolBusy = !job_queue_.empty();
    }

    return poolBusy;
}

void ThreadPool::addJob(const std::function<void()> &job)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    job_queue_.push(job);
    condition_threads_.notify_one();
}

void ThreadPool::run(uint32_t threads_count)
{
    if (!terminate_)
        return;

    if (isBusy())
        terminate();

    setup(threads_count);
}

void ThreadPool::terminate()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        terminate_ = true;
    }
    condition_threads_.notify_all();

    join();

    std::queue<std::function<void()>> empty;
    std::swap(job_queue_, empty);
    thread_pool_.clear();
}

void ThreadPool::join()
{
    for (auto &i : thread_pool_) i.join();
}
