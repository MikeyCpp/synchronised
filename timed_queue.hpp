#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

template<typename T, typename Clock = std::chrono::system_clock>
class timed_queue
{
public:
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;

    timed_queue() = default;
    virtual ~timed_queue() = default;

    virtual void push(time_point aTime, std::unique_ptr<T> aItem) =0;
    virtual std::unique_ptr<T> pop() =0;

    virtual void wait() const =0;
    virtual void wait_until(time_point aTime) const =0;
    virtual void wait_for(duration aDuration) const =0;
};

template<typename T, typename Clock = std::chrono::system_clock>
class timed_queue_impl : public timed_queue<T, Clock>
{
public:
    using typename timed_queue<T, Clock>::duration;
    using typename timed_queue<T, Clock>::time_point;

    timed_queue_impl() = default;
    ~timed_queue_impl() = default;

    void push(time_point aTime, std::unique_ptr<T> aItem) override
    {
        std::lock_guard<std::mutex> lk{m_};

        bool anotherTaskReadyBeforeThisOne = item_ready_before(aTime);

        queue_[aTime].push(std::move(aItem));

        if (!anotherTaskReadyBeforeThisOne)
            notify_new_item();
    }

    std::unique_ptr<T> pop() override
    {
        std::lock_guard<std::mutex> lk{m_};

        if (queue_.empty())
            return {};

        auto itemsIter = queue_.begin();

        const auto& itemReadyTime = itemsIter->first;

        if (itemReadyTime > time_now())
            return {};

        auto& queue = itemsIter->second;

        auto aItem = std::move(queue.front());
        queue.pop();

        if (queue.empty())
            queue_.erase(itemsIter);

        return aItem;
    }

    void wait() const override
    {
        wait_for(maximum_duration());
    }

    void wait_until(time_point aTime) const override
    {
        wait_for(aTime - time_now());
    }

    void wait_for(duration aDuration) const override
    {
        std::unique_lock<std::mutex> lk{m_};

        time_point originalTargetTime = time_now() + aDuration;

        while (true)
        {
            auto currentTime = time_now();

            if (originalTargetTime <= currentTime || item_ready_before(currentTime))
                break;

            auto targetDurationLeft = originalTargetTime - currentTime;
            auto durationTillTask = duration_till_next_item();

            auto nextTarketDuration = std::min(durationTillTask, targetDurationLeft);
            auto nextTargetTime = currentTime + nextTarketDuration;

            cv_.wait_for(lk, nextTarketDuration, [&]{
                return item_ready_before(nextTargetTime);
            } );
        }
    }

private:
    bool item_ready_before(time_point aTime) const
    {
        if (queue_.empty())
            return false;

        return queue_.begin()->first <= aTime;
    }

    duration duration_till_next_item() const
    {
        if (queue_.empty())
            return maximum_duration();

        auto time = queue_.begin()->first;
        return time - time_now();
    }

    void notify_new_item()
    {
        cv_.notify_all();
    }

    time_point time_now() const
    {
        return Clock::now();
    }

    duration maximum_duration() const
    {
        return Clock::duration::max();
    }

    std::map<time_point, std::queue<std::unique_ptr<T>>> queue_;
    mutable std::mutex m_;
    mutable std::condition_variable cv_;
};
