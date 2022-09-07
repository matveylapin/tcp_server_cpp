#include "ThreadPool.hpp"

ThreadPool::ThreadPool(uint32_t threads_count) { setup(threads_count); }

void ThreadPool::ThreadLoop()
{
    while (true)
    {
        int job_id;
        std::function<void()> job;
        
        if (terminate_) return;

        jobs_queue_mutex_.lock();

        if (job_queue_.empty())
        {
            jobs_queue_mutex_.unlock();
            continue;
        }

        job_id = job_queue_.front().first;
        job = job_queue_.front().second;
        job_queue_.pop();

        jobs_queue_mutex_.unlock();

        job();
    }
}

void ThreadPool::setup(uint32_t threads_count)
{
    jobs_queue_mutex_.lock();
    terminate_ = false;

    for (uint32_t i = 0; i < threads_count; i++)
        thread_pool_.push_back(std::thread([this]
                                           { ThreadLoop(); }));
    
    jobs_queue_mutex_.unlock();
}

bool ThreadPool::isBusy()
{
    bool poolBusy;
    {
        jobs_queue_mutex_.lock();
        poolBusy = !job_queue_.empty();
        jobs_queue_mutex_.unlock();
    }

    return poolBusy;
}

void ThreadPool::addJob(const std::function<void()> &job)
{
    addJob(job, 0);
}

void ThreadPool::addJob(const std::function<void()> &job, int job_id)
{
    jobs_queue_mutex_.lock();
    std::pair<int, std::function<void()>> newJob(job_id, job);

    job_queue_.emplace(newJob);
    condition_threads_.notify_one();

    jobs_queue_mutex_.unlock();
}

bool ThreadPool::isJobInQueue(int job_id)
{
    jobs_queue_mutex_.lock();
    std::queue<std::pair<int, std::function<void()>>> queue_copy = job_queue_;
    
    while (!queue_copy.empty())
    {
        auto tmpJob = queue_copy.front();
        queue_copy.pop();
        
        if (tmpJob.first == job_id)
        {
            jobs_queue_mutex_.unlock();
            return true;
        }
    }

    jobs_queue_mutex_.unlock();

    return false;
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

    jobs_queue_mutex_.lock();
    terminate_ = true;
    jobs_queue_mutex_.unlock();

    condition_threads_.notify_all();

    join();

    std::queue<std::pair<int, std::function<void()>>> empty;
    std::swap(job_queue_, empty);
    thread_pool_.clear();
}

void ThreadPool::join()
{
    for (auto &i : thread_pool_)
        i.join();
}

