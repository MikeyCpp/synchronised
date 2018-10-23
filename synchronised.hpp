#pragma once

#include "task.hpp"
#include "timed_queue.hpp"

#include <chrono>
#include <future>
#include <memory>

template<typename T, typename Clock = std::chrono::system_clock>
class synchronised
{
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;
public:  
    synchronised(std::unique_ptr<T> ptr,
                 std::shared_ptr<timed_queue<deferred_task, Clock>> queue)
        : queue_(queue),
          ptr_(std::move(ptr)) {}

    ~synchronised() = default;

    template<typename U, typename ...Args, typename ...Igs>
    std::future<U> enqueue(U (T::* aFunc)(Args...), Igs&&... igs)
    {
        return enqueue_for(Clock::now(), aFunc, std::forward<Igs>(igs)...);
    }

    template<typename U, typename ...Args, typename ...Igs>
    std::future<U> enqueue_after(duration aDuration, U (T::* aFunc)(Args...), Igs&&... igs)
    {
        return enqueue_for(Clock::now() + aDuration, aFunc, std::forward<Igs>(igs)...);
    }

    template<typename U, typename ...Args, typename ...Igs>
    std::future<U> enqueue_for(time_point aTime, U (T::* aFunc)(Args...), Igs&&... igs)
    {
        auto aTask = make_deferred_task(aFunc, ptr_, std::forward<Igs>(igs)...);
        auto aFuture = aTask->get_future();

        queue_->push(aTime, std::move(aTask));

        return aFuture;
    }

private:
    std::shared_ptr<timed_queue<deferred_task, Clock>> queue_;
    std::shared_ptr<T> ptr_;
};
