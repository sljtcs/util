#pragma once
#include "test/rely.h"
#include <string>
#include <memory>

extern "C"
{
    #include <lua/lua.h>
    #include <lua/lauxlib.h>
    #include <lua/lualib.h>
}

namespace test
{
    // std::string scriptPath = "./archive/script/test.lua";
}



namespace test
{
    struct Vec3
    {
        float x{0.0f}, y{0.0f}, z{0.0f};
    };
    struct Object
    {
        uint32_t id {0};
        float x,y,z;
    };

    class Handle
    {
    public:
        explicit Handle(std::string type)
        : m_type(type)
        {}
    public:
        void render()
        {
            LOG("render: ", m_type);
            for(auto& ptr : m_store)
                LOG("id: ", ptr->id,
                    ", x: ", ptr->x,
                    ", y: ", ptr->y,
                    ", z: ", ptr->z
                );
        }
    public:
        uint32_t create()
        {
            auto ptr = std::make_shared<Object>();
            ptr->id = id++;
            m_store.push_back(ptr);
            return ptr->id;
        }
        void clear()
        {
            m_store.clear();
            m_store.shrink_to_fit();
        }
        void setPos(uint32_t id, Vec3 pos)
        {
            for(auto& ptr : m_store)
                if(ptr->id == id)
                {
                    ptr->x = pos.x;
                    ptr->y = pos.y;
                    ptr->z = pos.z;
                }
        }
    private:
        std::string m_type;
        uint32_t id{0};
        std::vector<std::shared_ptr<Object>> m_store;
    };
}



namespace test
{
    static bool get_vec3_field(lua_State* L, int index, const char* key, Vec3& out)
    {
        // 期望: table[key] 是一个 table，里面有 x/y/z
        lua_getfield(L, index, key);            // push table[key]
        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }

        lua_getfield(L, -1, "x"); out.x = (float)luaL_optnumber(L, -1, 0.0); lua_pop(L, 1);
        lua_getfield(L, -1, "y"); out.y = (float)luaL_optnumber(L, -1, 0.0); lua_pop(L, 1);
        lua_getfield(L, -1, "z"); out.z = (float)luaL_optnumber(L, -1, 0.0); lua_pop(L, 1);

        lua_pop(L, 1);                          // pop table[key]
        return true;
    }

    static bool apply_list(lua_State* L, const char* globalName, Handle& handle)
    {
        lua_getglobal(L, globalName);           // push _G[globalName]
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return false; // 没有该表就当作空
        }

        // Lua 数组从 1 开始
        size_t n = lua_rawlen(L, -1);
        for (size_t i = 1; i <= n; ++i)
        {
            lua_rawgeti(L, -1, (lua_Integer)i); // push list[i]
            if (!lua_istable(L, -1)) { lua_pop(L, 1); continue; }

            Vec3 pos{};
            // 期望元素长这样：{ pos = {x=..,y=..,z=..} }
            if (!get_vec3_field(L, -1, "pos", pos)) {
                // 也可以支持简写：{x=,y=,z=}（可选）
                lua_getfield(L, -1, "x");
                if (lua_isnumber(L, -1)) { out:; }
                lua_pop(L, 1);
            }

            uint32_t id = handle.create();
            handle.setPos(id, pos);

            lua_pop(L, 1); // pop list[i]
        }

        lua_pop(L, 1); // pop _G[globalName]
        return true;
    }
}

namespace test
{
    Handle CameraHandle("camera");
    Handle ModelHandle("model");
    struct PreviewHandle
    {
    public:
        void load(const std::string luaPath)
        {
            // 每次 load 前先清空
            unload();

            lua_State* L = luaL_newstate();
            luaL_openlibs(L);

            if (luaL_dofile(L, luaPath.c_str()) != LUA_OK)
            {
                const char* err = lua_tostring(L, -1);
                LOG("lua error: ", (err ? err : "unknown"));
                lua_pop(L, 1);
                lua_close(L);
                return;
            }

            // 从全局表创建对象
            apply_list(L, "Cameras", CameraHandle);
            apply_list(L, "Models",  ModelHandle);

            lua_close(L);
        }
        void unload()
        {
            CameraHandle.clear();
            ModelHandle.clear();
        }
        void render()
        {
            CameraHandle.render();
            ModelHandle.render();
        }
    };
}


namespace test
{
    void test()
    {
        std::string scriptPath = "./archive/script/test2.lua";
        PreviewHandle ph;
        ph.load(scriptPath);
        ph.render();
    }
}


/*
这东西能够用来干什么

1.做关卡
    当系统很完备的时候
    写个图形化编辑器 对象创建接口 属性创建接口 时序等等
    然后导出成lua 就是关卡
    用来做演示场景似乎有点浪费了

2.做开机启动脚本
    当前后端非常分离的时候
    命令行启动指定脚本路径
    脚本1 脚本2 脚本3

    脚本调用C接口
    A接口初始化
    延时
    B接口XXX参数设置
    延时
    C接口XXX参数设置
    延时
    D接口调用
    C面板打开
    D面板打开


    "C:\path\to\MyApp.exe" --script "C:\path\to\user_settings.lua"

    如果觉得cpp转lua_C接口很麻烦 可以用 sol这个库
    sol也是个lua库
    #include <sol/sol.hpp>
    #include <iostream>

    int main(int argc, char* argv[]) {
        // 创建 Lua 状态机
        sol::state lua;
        lua.open_libraries(sol::lib::base);

        // 解析命令行参数，判断是否有指定的 Lua 脚本
        if (argc > 1) {
            std::string lua_script = argv[1];
            try {
                // 执行指定的 Lua 脚本
                lua.script_file(lua_script);
                std::cout << "Executed Lua script: " << lua_script << std::endl;
            } catch (const sol::error& e) {
                std::cerr << "Error running Lua script: " << e.what() << std::endl;
            }
        } else {
            std::cout << "No Lua script specified." << std::endl;
        }

        return 0;
    }
*/

// 假如我的软件有
// {
// backend:
//     bool opencamera();
//     void checklocal();
//     void startmocap();
//     void startUDP();

// fontend:
//     void openWinCam();
//     void openWin3D();
// }

// sol2是lua的封装 不需要手动操作lua虚拟机

// sol::state lua;
// lua.open_libraries(sol::lib::base);
// lua.set_function("opencamera", &opencamera);
// lua.set_function("checklocal", &checklocal);
// lua.set_function("startmocap", &startmocap);
// lua.set_function("startUDP", &startUDP);
// lua.set_function("openWinCam", &openWinCam);
// lua.set_function("openWin3D", &openWin3D);

// 直接绑定就可以了 不需要再写C函数

// 假设我lua脚本地址是 ./test.lua
// lua脚本里面的逻辑已经写好
// 也有lua库依赖
// 那么这里想要完整地执行这个脚本
// 是怎么操作
// void executeScript(const std::string& path)
// {
//     sol::state lua;
    
//     // 打开 Lua 基础库
//     lua.open_libraries(sol::lib::base);

//     // 注册 C++ 函数到 Lua
//     lua.set_function("opencamera", &opencamera);
//     lua.set_function("checklocal", &checklocal);
//     lua.set_function("startmocap", &startmocap);
//     lua.set_function("startUDP", &startUDP);
//     lua.set_function("openWinCam", &openWinCam);
//     lua.set_function("openWin3D", &openWin3D);

//     // 加载 Lua 脚本
//     try {
//         lua.script_file(path);  // 执行指定路径的 Lua 脚本
//     } catch (const sol::error& e) {
//         std::cerr << "Error executing Lua script: " << e.what() << std::endl;
//     }
// }

// int main() {
//     std::string scriptPath = "./test.lua"; // Lua 脚本路径
//     executeScript(scriptPath);
//     return 0;
// }


// ⭐⭐⭐⭐⭐
// C++ 提供能力和约束
// Lua 提供决策和数据
// 把接口交给 Lua，让Lua自己去填写数据
// 而不要让cpp试图从lua的数据表中读取数据