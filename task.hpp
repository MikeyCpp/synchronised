#pragma once

#include "integer_sequence.hpp"

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <tuple>

class task
{
public:
    virtual ~task() = default;
    virtual void execute() = 0;
};

template<typename Base, typename T, typename ...Args>
class task_impl : public task
{
public:
    template<typename ...Igs>
    task_impl(std::promise<T> aPromise,
              T (Base::* aFunc)(Args...),
              Base* aBase,
              Igs&&... igs) :
        promise_(std::move(aPromise)),
        base_(aBase),
        func_(aFunc),
        args_(std::forward<Igs>(igs)...) {}

    ~task_impl() override = default;

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

private:
    template<int ...S>
    T execute_impl(integer_sequence<S...>)
    {
        return (base_->*func_)(std::forward<Args>(std::get<S>(args_))...);
    }

    std::promise<T> promise_;

    using FunctionType = T (Base::*)(Args...);
    Base* base_;
    FunctionType func_;
    std::tuple<Args...> args_;

    bool executed_{false};
};

template<typename T, typename Base, typename ...Args, typename ...Igs>
std::unique_ptr<task> make_task(std::promise<T> aPromise, T (Base::* aFunc)(Args...), Base* aBase, Igs&&... igs)
{
    return std::unique_ptr<task>(
                new task_impl<Base, T, Args...>(std::move(aPromise), aFunc, aBase, std::forward<Igs>(igs)...));
}
