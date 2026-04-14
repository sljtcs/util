#pragma once

#include "test/rely.h"
#include <functional>
#include "testTask.h"

namespace test
{
    class StateMachine
    {
    public:
        void updateLaunch(){ isUpdating = true; }
        void updateOver(bool success){ isUpdating = false; isUpdateSuccess = success;}
    public:
        bool isUpdating {false};
        bool isUpdateSuccess {false};
    };
    StateMachine sm;
}

namespace test
{
    class SimulateAPI
    {
    public:
        static bool SyncConnect()
        {
            SLEEP_MiLLISECOND(3000);
            LOG("connect success");
            return true;
        };
        static void ASyncFetchFile(std::function<void(bool)> callback)
        {
            std::thread([callback](){
                size_t count {0};
                while(true)
                {
                    SLEEP_MiLLISECOND(1000);
                    ++count;
                    LOG("download part: ", count);
                    if(count>=10) break;
                }

                callback(true);
                LOG("call back over");
            }).detach();
        
        };
        static bool SyncLocalAuth()
        {
            SLEEP_MiLLISECOND(5000);
            LOG("Auth over");
            return true;
        };
    };
}

namespace test
{
    struct FetchFileAwaiter
    {
        bool result = false;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h)
        {
            test::SimulateAPI::ASyncFetchFile(
                [this, h](bool success)
                {
                    result = success;
                    h.resume();
                }
            );
        }

        bool await_resume() const noexcept
        {
            return result;
        }
    };

    Task updateSession_coro()
    {
        test::sm.updateLaunch();

        // Step 1: SyncConnect
        if(!test::SimulateAPI::SyncConnect())
        {
            test::sm.updateOver(false);
            co_return;
        }

        // Step 2: async fetch
        bool fetchOK = co_await FetchFileAwaiter{};
        if(!fetchOK)
        {
            test::sm.updateOver(false);
            co_return;
        }

        // Step 3: SyncLocalAuth
        if(!test::SimulateAPI::SyncLocalAuth())
        {
            test::sm.updateOver(false);
            co_return;
        }

        test::sm.updateOver(true);
    }
}

namespace test
{
    void test()
    {
        // if(ui_button)
        // updateSession();
        updateSession_coro();

        while(true)
        {
            SLEEP_MiLLISECOND(1000);

            if(!sm.isUpdating)
            {
                if(sm.isUpdateSuccess)
                    LOG("update success");
                else
                    LOG("update failed");
            }

            LOG("other UI");
            // other ui render
        }
    }
}