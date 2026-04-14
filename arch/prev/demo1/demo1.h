#pragma once
#include "test/rely.h"

extern "C"
{
    #include <lua/lua.h>
    #include <lua/lauxlib.h>
    #include <lua/lualib.h>
}

namespace test
{
    static int l_cpp_log(lua_State* L)
    {
        // 参数数量
        int n = lua_gettop(L);

        // 取第 1 个参数（lua 下标从 1 开始）
        const char* msg = luaL_checkstring(L, 1);

        std::cout << "[cpp] cpp_log called, argc=" << n
                << ", msg=" << msg << "\n";

        // 返回值数量：0 表示不返回
        return 0;
    }
}


namespace test
{
    // std::string scriptPath = "./archive/script/test.lua";
    std::string scriptPath = "./archive/script/test1.lua";
}

namespace test
{
    struct Model
    {
        void setValueA(int value) { valueA = value; }
        int getValueB() const { return valueB; }

        int valueA{0};
        int valueB{10};
    };

    // Lua: model_getValueB(model_ptr)
    static int l_model_getValueB(lua_State* L)
    {
        auto* model = static_cast<Model*>(lua_touserdata(L, 1));
        if (!model) return luaL_error(L, "model_getValueB: model is null");

        lua_pushinteger(L, model->getValueB());
        return 1; // 返回 1 个值
    }

    // Lua: model_setValueA(model_ptr, value)
    static int l_model_setValueA(lua_State* L)
    {
        auto* model = static_cast<Model*>(lua_touserdata(L, 1));
        if (!model) return luaL_error(L, "model_setValueA: model is null");

        int value = (int)luaL_checkinteger(L, 2);
        model->setValueA(value);

        return 0;
    }

    // 执行规则
    inline bool runRule(Model& model, const char* scriptPath)
    {
        lua_State* L = luaL_newstate();
        if (!L) return false;

        luaL_openlibs(L);

        // 注册 C 函数给 Lua
        lua_register(L, "model_getValueB", l_model_getValueB);
        lua_register(L, "model_setValueA", l_model_setValueA);

        // 加载脚本
        if (luaL_dofile(L, scriptPath) != LUA_OK)
        {
            std::cerr << "[cpp] lua load error: " << lua_tostring(L, -1) << "\n";
            lua_close(L);
            return false;
        }

        // 调用 apply(model)
        lua_getglobal(L, "apply");
        if (!lua_isfunction(L, -1))
        {
            std::cerr << "[cpp] function apply not found\n";
            lua_close(L);
            return false;
        }

        lua_pushlightuserdata(L, &model);

        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            std::cerr << "[cpp] apply call error: " << lua_tostring(L, -1) << "\n";
            lua_close(L);
            return false;
        }

        lua_close(L);
        return true;
    }
}


namespace test
{
    int test()
    {
        test::Model model;
        model.valueB = 10;

        std::cout << "before: valueA=" << model.valueA << " valueB=" << model.valueB << "\n";

        test::runRule(model, scriptPath.c_str());

        std::cout << "after : valueA=" << model.valueA << " valueB=" << model.valueB << "\n";
        return 0;
    }
}