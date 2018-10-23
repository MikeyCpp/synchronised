#pragma once

#include "integer_sequence.hpp"

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <tuple>

struct deferred_task_expired : public std::logic_error
{
    deferred_task_expired(const std::string& e)
        : logic_error(e) {}
};

class deferred_task
{
public:
    virtual ~deferred_task() = default;
    virtual void execute() = 0;
};

template<typename Base, typename T, typename ...Args>
class  deferred_task_impl : public deferred_task
{
public:
    template<typename ...Igs>
    deferred_task_impl(
            T (Base::* aFunc)(Args...),
            std::weak_ptr<Base> aBase,
            Igs&&... igs) :
        base_(aBase),
        func_(aFunc),
        args_(std::forward<Igs>(igs)...) {}

    ~deferred_task_impl() override = default;

    void execute() override
    {
        if(executed_)
            return;
        try
        {
            T result = execute_impl(make_integer_sequence(args_));

            try
            {
                promise_.set_value(std::forward<T>(result));
            } catch(...) {}
        }
        catch(...)
        {
            try
            {
                promise_.set_exception(std::current_exception());
            } catch(...) {};
        }

        executed_ = true;
    }

    std::future<T> get_future()
    {
        return promise_.get_future();
    }

private:
    template<int ...S>
    T execute_impl(integer_sequence<S...>)
    {
        if(base_.expired())
            throw deferred_task_expired("The deferred task has expired");

        return (base_.lock().get()->*func_)(std::forward<Args>(std::get<S>(args_))...);
    }

    std::promise<T> promise_;

    using FunctionType = T (Base::*)(Args...);
    std::weak_ptr<Base> base_;
    FunctionType func_;
    std::tuple<Args...> args_;

    bool executed_{false};
};

template<typename T, typename Base, typename ...Args, typename ...Igs>
std::unique_ptr<deferred_task_impl<Base, T, Args...>> make_deferred_task(T (Base::* aFunc)(Args...), std::shared_ptr<Base> aBase, Igs&&... igs)
{
    return std::unique_ptr<deferred_task_impl<Base,T,Args...>>(
                new  deferred_task_impl<Base, T, Args...>(aFunc, aBase, std::forward<Igs>(igs)...));
}
