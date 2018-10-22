#include "synchronised.hpp"
#include "timed_queue.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <memory>

class TestClass
{
public:

    int foo(int k)
    {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        std::cout << "foo called " << k << std::endl;
        static int res = 7;
        return res++;
    }
};

int main()
{
    std::unique_ptr<TestClass> myTestClass{new TestClass()};
    auto taskQueue = std::make_shared<timed_queue_impl<task>>();

    synchronised<TestClass> syncTestClass(std::move(myTestClass),
                                          taskQueue);

    auto f = std::async(std::launch::async,
                        [&]{
        std::cout << "starting wait" << std::endl;
        taskQueue->wait_for(std::chrono::milliseconds{2000});
        std::cout << "wait over" << std::endl;
        while(auto task = taskQueue->pop())
        {
            task->execute();
        }
        std::cout << "all tasks ran" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds{1});

    std::cout << "Adding Task" << std::endl;

    auto f1 = syncTestClass.enqueue(&TestClass::foo, 99);
    auto f2 = syncTestClass.enqueue(&TestClass::foo, 100);
    {
        auto f3 = syncTestClass.enqueue(&TestClass::foo, 101);
    }

    //syncTestClass.wait_for(std::chrono::milliseconds(12));

    std::cout << f1.get() << std::endl;
    std::cout << f2.get() << std::endl;
    //std::cout << f3.get() << std::endl;


    //std::cout << "Hello World!" << std::endl;
    return 0;
}
