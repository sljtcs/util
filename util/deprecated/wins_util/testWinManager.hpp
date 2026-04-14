#pragma once
#include "testWin.hpp"
#include <unordered_map>
#include <mutex>
#include <list>

struct TestWinInfo
{
    std::string key;
    int x =0, y=0, w=0, h=0;
    size_t id;
    TestWin* testWinPtr = nullptr;
};

class TestWinManager
{
public:
    TestWinManager(){}
    ~TestWinManager()
    {
        std::vector<int> ids;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for(const auto& [_, id] : _keyID_map)
            {
                ids.push_back(id);
            }
        }

        for(int id : ids)
            delWin(id);

        std::cout << "TestWinManager: release" << std::endl;
    }
public:
    void getWinSize(int id, int& w, int& h)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _IDIT_map.find(id);
        if(it == _IDIT_map.end()) return;

        auto& list_it = it->second;
        TestWinInfo* winInfo = *list_it;
        w = winInfo->w;
        w = winInfo->h;
    }
    void setArea(size_t x, size_t y, size_t w)
    {
        _x = x; _y = y; _w = w;
        reCalc();
    }
    void createWin(const std::string& key)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_keyID_map.find(key) != _keyID_map.end()) return;

            TestWinInfo* testWinInfoPtr = new TestWinInfo();
            testWinInfoPtr->key = key;
            testWinInfoPtr->id = _counter++;
            testWinInfoPtr->testWinPtr = new TestWin(key);
            _infoList.push_back(testWinInfoPtr);

            _IDIT_map[testWinInfoPtr->id] = std::prev(_infoList.end());
            _keyID_map[testWinInfoPtr->key] = testWinInfoPtr->id;
        }
        reCalc();
    }
    int getWinID(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _keyID_map.find(key);
        if(it ==_keyID_map.end()) return -1;
        return it->second;
    }
    HWND getRawHwnd(int id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _IDIT_map.find(id);
        if(it == _IDIT_map.end()) return nullptr;

        auto& list_it = it->second;
        TestWinInfo* winInfo = *list_it;
        return winInfo->testWinPtr->getHandle();
    }
    void delWin(int id)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _IDIT_map.find(id);
            if(it == _IDIT_map.end()) return;

            auto& list_it = it->second;
            TestWinInfo* delWinInfo = *list_it;
            // list中删除
            _infoList.erase(list_it);

            // IDITMap中删除 键值对
            _IDIT_map.erase(it);

            // KeyIDmap中删除 键值对
            auto keyID_it = _keyID_map.find(delWinInfo->key);
            if(keyID_it !=_keyID_map.end()){ _keyID_map.erase(keyID_it); }

            // 删除本身
            delete delWinInfo->testWinPtr;
            delete delWinInfo;
        }
        reCalc();
    }
    void doMsgWork()
    {
        // std::lock_guard<std::mutex> lock(_mutex);
        for(auto& info : _infoList)
        {
            info->testWinPtr->doMsgWork();
        }
    }
public:
    void reCalc()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_infoList.size() == 0) return;
        size_t line = std::ceil(std::sqrt(_infoList.size()));
        size_t startX = _x, startY = _y;
        size_t preW = _w/line;

        size_t idx = 0;
        for(auto& info : _infoList)
        {
            info->x = startX + idx%line *preW;
            info->y = startY + idx/line *preW;
            info->w = preW;
            info->h = preW;
            info->testWinPtr->setPos(info->x, info->y, info->w, info->h);
            ++idx;
        }
    }
public:
    size_t _x, _y, _w;
    size_t _counter = 0;
    std::mutex _mutex;
    std::list<TestWinInfo*> _infoList;
    std::unordered_map<int, std::list<TestWinInfo*>::iterator> _IDIT_map;
    std::unordered_map<std::string, int> _keyID_map;
};