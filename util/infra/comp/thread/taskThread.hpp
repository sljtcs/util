#pragma once
#include <memory>
#include <functional>
#include "util/infra/debug/log.hpp"

struct Task
{
public:
    explicit Task(std::function<void(std::shared_ptr<Task> task)> func_)
    : func(func_)
    {}
public:
    std::function<void()> func;
public:
    void cancel()
    {
        active = false;
    }
    bool isActive()
    {
        return active;
    }
    void run()
    {
        func(shared_from_this());
    }
private:
    bool active {false};
};






class TaskThread
{
public:

};

int main()
{
    std::shared_ptr<Task> cancelTask = std::make_shared<Task>(
        [](std::shared_ptr<Task> task){
            if(task->isActive())
                LOG("A");
            if(task->isActive())
                LOG("B");
            if(task->isActive())
                LOG("C");
        }
    );
}