#pragma once

#include "test/rely.h"
#include <functional>
#include "testFuture.h"

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
            }).detach();
        
        };
        static bool SyncLocalAuth()
        {
            SLEEP_MiLLISECOND(3000);
            LOG("Auth over");
            return true;
        };
    };
}

namespace test
{
    ThenFuture<bool> async_sync_connect()
    {
        std::promise<bool> p;
        auto fut = p.get_future();

        std::thread([p = std::move(p)]() mutable {
            bool ok = SimulateAPI::SyncConnect();
            p.set_value(ok);
        }).detach();

        return ThenFuture<bool>(std::move(fut));
    }

    ThenFuture<bool> async_fetch_file()
    {
        std::promise<bool> p;
        auto fut = p.get_future();
        SimulateAPI::ASyncFetchFile([p = std::move(p)](bool ok) mutable {
            p.set_value(ok);
        });

        return ThenFuture<bool>(std::move(fut));
    }

    ThenFuture<bool> async_sync_auth()
    {
        std::promise<bool> p;
        auto fut = p.get_future();

        std::thread([p = std::move(p)]() mutable {
            bool ok = SimulateAPI::SyncLocalAuth();
            p.set_value(ok);
        }).detach();

        return ThenFuture<bool>(std::move(fut));
    }

}

namespace test
{
    void updateSession()
    {
        StateMachine* ptr = &sm;
        std::thread([ptr](){
            ptr->updateLaunch();
            if(SimulateAPI::SyncConnect())
            {
                SimulateAPI::ASyncFetchFile([ptr](bool success){
                    if(success)
                    {
                        if(SimulateAPI::SyncLocalAuth())
                            ptr->updateOver(true);
                        else
                            ptr->updateOver(false);
                    }
                    else ptr->updateOver(false);
                });
            }
            else ptr->updateOver(false);
        }).detach();
    }

    void updateSession_then()
    {
        StateMachine* ptr = &sm;
        ptr->updateLaunch();

        async_sync_connect()
            .then([ptr](bool ok) {
                if (!ok) {
                    ptr->updateOver(false);
                    return;
                }

                async_fetch_file()
                    .then([ptr](bool ok2){
                        if (!ok2) {
                            ptr->updateOver(false);
                            return;
                        }

                        async_sync_auth()
                            .then([ptr](bool ok3) {
                                ptr->updateOver(ok3);
                            });
                    });
            });
    }
}

namespace test
{
    void test()
    {
        // if(ui_button)
            updateSession();

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