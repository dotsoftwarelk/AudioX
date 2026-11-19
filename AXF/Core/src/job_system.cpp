#include "axf/core/job_system.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace axf::core
{
namespace
{
constexpr std::size_t kPriorityCount = 3;
std::size_t priorityIndex(JobPriority priority) noexcept
{
    return static_cast<std::size_t>(priority);
}
} // namespace

struct JobSystem::Impl final
{
    explicit Impl(std::uint32_t requested)
        : workerCount_(requested == 0 ? std::max(1U, std::thread::hardware_concurrency() > 0
                                                         ? std::thread::hardware_concurrency() - 1
                                                         : 1U)
                                      : requested)
    {
        workers_.reserve(workerCount_);
        for (std::uint32_t i = 0; i < workerCount_; ++i)
        {
            workers_.emplace_back([this] { run(); });
        }
    }

    ~Impl()
    {
        shutdown();
    }

    bool hasWork() const noexcept
    {
        return !queues_[0].empty() || !queues_[1].empty() || !queues_[2].empty();
    }

    void run()
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stopping_ || hasWork(); });
                if (stopping_ && !hasWork())
                {
                    break;
                }
                for (std::size_t index = kPriorityCount; index-- > 0;)
                {
                    if (!queues_[index].empty())
                    {
                        job = std::move(queues_[index].front());
                        queues_[index].pop_front();
                        break;
                    }
                }
                active_.fetch_add(1, std::memory_order_relaxed);
            }

            try
            {
                if (job)
                {
                    job();
                }
            }
            catch (...)
            {
                // A misbehaving job must not take down a worker thread.
            }
            active_.fetch_sub(1, std::memory_order_relaxed);
            cv_.notify_all();
        }
    }

    void submit(std::function<void()> job, JobPriority priority)
    {
        if (!job)
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_)
            {
                return;
            }
            queues_[priorityIndex(priority)].push_back(std::move(job));
        }
        cv_.notify_one();
    }

    void waitForAll()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock,
                 [this] { return !hasWork() && active_.load(std::memory_order_relaxed) == 0; });
    }

    void shutdown()
    {
        bool joinWorkers = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
            if (!joined_)
            {
                joined_ = true;
                joinWorkers = true;
            }
        }
        cv_.notify_all();
        if (joinWorkers)
        {
            for (std::thread& worker : workers_)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }
        }
    }

    std::size_t pendingCount() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queues_[0].size() + queues_[1].size() + queues_[2].size();
    }

    const std::uint32_t workerCount_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::array<std::deque<std::function<void()>>, kPriorityCount> queues_;
    std::atomic<std::uint32_t> active_{0};
    bool stopping_{false};
    bool joined_{false};
    std::vector<std::thread> workers_;
};

JobSystem::JobSystem(std::uint32_t workerCount) : impl_(std::make_unique<Impl>(workerCount))
{
}

JobSystem::~JobSystem() = default;

void JobSystem::submit(std::function<void()> job, JobPriority priority)
{
    impl_->submit(std::move(job), priority);
}

void JobSystem::waitForAll()
{
    impl_->waitForAll();
}

void JobSystem::shutdown()
{
    impl_->shutdown();
}

std::uint32_t JobSystem::workerCount() const noexcept
{
    return impl_->workerCount_;
}

std::size_t JobSystem::pendingCount() const noexcept
{
    return impl_->pendingCount();
}

bool JobSystem::isShutdown() const noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->stopping_;
}

} // namespace axf::core
