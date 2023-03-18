#include "Script/LuaFunc.h"
#include "Logger.h"
#include "Engine.h"
#include "Script/ScriptManager.h"
#include "Script/Script.h"

namespace app {
namespace script {

/**
 * @brief log a msg
 * @param 1: [opt] log level, @see AppLogLevelNames
 * @param 2: msg
 * @return 0 none
 */
int LuaLog(lua_State* vm) {
    u32 cnt = lua_gettop(vm);

    // Log("msg");
    if (1 == cnt && lua_isstring(vm, 1)) {
        Logger::logInfo("LuaEng> %s", lua_tostring(vm, 1));
        return 0;
    }

    // Log(Info, "msg");
    if (2 == cnt && lua_isinteger(vm, 1) && lua_isstring(vm, 2)) {
        cnt = static_cast<u32>(lua_tointeger(vm, 1));
        Logger::log(static_cast<ELogLevel>(cnt < ELL_COUNT ? cnt : ELL_CRITICAL),
            "LuaEng> %s", lua_tostring(vm, 2));
        return 0;
    }
    Logger::logError("LuaEng> invalid log param, cnt=%u", cnt);
    return 0;
}


/**
 * @brief  构造函数: 颜色
 * @param 1 红
 * @param 2 绿
 * @param 3 蓝
 * @param 4 透明度
 * @return 颜色
 */
int LuaColor(lua_State* vm) {
    u32 col = 0xFF & lua_tointeger(vm, 1);
    col |= (0xFF & lua_tointeger(vm, 2)) << 8;
    col |= (0xFF & lua_tointeger(vm, 2)) << 16;
    col |= (0xFF & lua_tointeger(vm, 2)) << 24;

    void* ret = lua_newuserdata(vm, sizeof(col));
    // memcpy(ret, &col, sizeof(col));
    *reinterpret_cast<u32*>(ret) = col;
    return 1;
}


int LuaRandom(lua_State* vm) {
    printf("LuaRandom = call test\n");
    lua_pushinteger(vm, rand());
    return 1;
}

int LuaEngExit(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    printf("LuaEng> stack size=%d\n", cnt);
    return 0;
}

int LuaEngInfo(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    Logger::logInfo("LuaEng> LUA.Version = %s", LUA_RELEASE);
    Logger::logInfo("LuaEng> LUA.MemSize = %llu", ScriptManager::getInstance().getMemory());
    Logger::logInfo("LuaEng> LUA.StackSize = %d", cnt);
    return 0;
}


LUALIB_API luaL_Reg LuaEngLib[] = {
    {"exit", LuaEngExit},
    {"showInfo", LuaEngInfo},
    {NULL, NULL}    //sentinel
};


int LuaOpenEngLib(lua_State* vm) {
    luaL_newlib(vm, LuaEngLib);
    return 1;
}

int LuaInclude(lua_State* vm) {
    ScriptManager& eng = ScriptManager::getInstance();
    const s8* fnm;  //relative path

    s32 cnt = lua_gettop(vm);
    if (1 != cnt || !lua_isstring(vm, 1)) {
        Logger::logError("LuaEng> LuaInclude fail=param");
        goto GT_LUA_FAIL;
    }

    fnm = lua_tostring(vm, 1);
    if (!fnm || '\0' == fnm[0]) {
        Logger::logError("LuaEng> LuaInclude fail=empty");
        goto GT_LUA_FAIL;
    }

    //load
    if (!eng.loadScript(fnm)) {
        Logger::logError("LuaEng> LuaInclude fail=include");
        goto GT_LUA_FAIL;
    }

    // success
    lua_pushnumber(vm, 1);
    return 1;

    GT_LUA_FAIL:
    lua_pushnil(vm);
    return 0;
}


void LuaCreateArray(lua_State* vm, s8** arr, int cnt) {
    lua_newtable(vm);
    for (ssz j = 0; j < cnt; ++j) {
        lua_pushlstring(vm, arr[j], strlen(arr[j]));
        lua_rawseti(vm, -2, j + 1);
    }
}


// regist class as a global table
void LuaRegistClass(lua_State* vm, const luaL_Reg* func, const usz funcsz,
    const s8* className, const s8* namespac) {
    DASSERT(func && funcsz && className);

    luaL_newmetatable(vm, className);
    s32 metatable = lua_gettop(vm);
    lua_pushvalue(vm, -1);            //repush metatable pointer
    lua_setfield(vm, -2, "__index");  //pop 1

    /*
    lua_pushstring(vm, "__gc");
    lua_pushcfunction(vm, &Wrapper<T>::gc_obj);
    lua_settable(vm, metatable);

    lua_pushstring(vm, "__tostring");
    lua_pushcfunction(vm, &Wrapper<T>::to_string);
    lua_settable(vm, metatable);

    lua_pushstring(vm, "__eq");
    lua_pushcfunction(vm, &Wrapper<T>::equals);
    lua_settable(vm, metatable);

    lua_pushstring(vm, "__index");
    lua_pushcfunction(vm, &Wrapper<T>::property_getter);
    lua_settable(vm, metatable);

    lua_pushstring(vm, "__newindex");
    lua_pushcfunction(vm, &Wrapper<T>::property_setter);
    lua_settable(vm, metatable);
    */
    for (usz i = 0; i < funcsz; ++i) {
        if (nullptr == func[i].name && nullptr == func[i].func) {
            continue;
        }
        if (App4Char2S32("new") != App4Char2S32(func[i].name)) {
            lua_pushstring(vm, func[i].name);
            lua_pushcfunction(vm, func[i].func);
            lua_settable(vm, metatable);
        } else {
            if (namespac && 0 != namespac[0]) {
                lua_getglobal(vm, namespac);
                if (lua_isnil(vm, -1)) { // Create namespace if not exsit
                    lua_newtable(vm);
                    lua_pushvalue(vm, -1); // Duplicate table pointer since setglobal pop the value
                    lua_setglobal(vm, namespac);
                }
                lua_pushcfunction(vm, func[i].func);
                lua_setfield(vm, -2, className);
                lua_pop(vm, 1);
            } else {
                lua_pushcfunction(vm, func[i].func);
                lua_setglobal(vm, className);
            }
        }
    }
    lua_pop(vm, 1);  //pop metatable
    DASSERT(0 == lua_gettop(vm));
}

}//namespace script
}//namespace app
