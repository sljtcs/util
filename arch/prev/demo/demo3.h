#pragma once

#include "test/rely.h"
#include <functional>

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