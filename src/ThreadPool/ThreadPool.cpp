#include "ThreadPool.hpp"

ThreadPool::ThreadPool(uint32_t threads_count) { setup(threads_count); }

void ThreadPool::ThreadLoop()
{
    while (true)
    {
        int job_id = -1;
        std::function<void()> job;
        std::unique_lock<std::mutex> lock(jobs_queue_mutex_);

        if (terminate_)
            return lock.unlock();

        condition_threads_.wait(lock, [this]
                                { return !job_queue_.empty(); });

        job_id = job_queue_.front().first;
        job = job_queue_.front().second;
        job_queue_.pop();

        lock.unlock();

        treads_vector_mutex_.lock();

        treads_vector_mutex_.unlock();

        job();
    }
}

void ThreadPool::setup(uint32_t threads_count)
{
    jobs_queue_mutex_.lock();

    terminate_ = false;
    // thread_pool_.resize(threads_count);

    for (uint32_t i = 0; i < threads_count; i++)
    {
        treads_vector_mutex_.lock();

        thread_pool_.push_back(std::thread([this]
                                           { ThreadLoop(); }));
        treads_vector_mutex_.unlock();
    }
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

void ThreadPool::addJob(const std::function<void()> &job, int jobs_id)
{
    // std::unique_lock<std::mutex> lock(jobs_queue_mutex_);
    jobs_queue_mutex_.lock();
    std::pair<int, std::function<void()>> newJob(jobs_id, job);

    job_queue_.emplace(newJob);
    condition_threads_.notify_one();

    jobs_queue_mutex_.unlock();
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

void ThreadPool::join(int jobs_id)
{
}
