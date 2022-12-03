#include "Script/ScriptFunc.h"
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
int LuaLog(lua_State* iState) {
    u32 cnt = lua_gettop(iState);

    // Log("msg");
    if (1 == cnt && lua_isstring(iState, 1)) {
        Logger::logInfo("LuaEng> %s", lua_tostring(iState, 1));
        return 0;
    }

    // Log(Info, "msg");
    if (2 == cnt && lua_isinteger(iState, 1) && lua_isstring(iState, 2)) {
        cnt = static_cast<u32>(lua_tointeger(iState, 1));
        Logger::log(static_cast<ELogLevel>(cnt < ELL_COUNT ? cnt : ELL_CRITICAL),
            "LuaEng> %s", lua_tostring(iState, 2));
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
int LuaColor(lua_State* iState) {
    u32 col = 0xFF & lua_tointeger(iState, 1);
    col |= (0xFF & lua_tointeger(iState, 2)) << 8;
    col |= (0xFF & lua_tointeger(iState, 2)) << 16;
    col |= (0xFF & lua_tointeger(iState, 2)) << 24;

    void* ret = lua_newuserdata(iState, sizeof(col));
    // memcpy(ret, &col, sizeof(col));
    *reinterpret_cast<u32*>(ret) = col;
    return 1;
}


int LuaRandom(lua_State* iState) {
    printf("LuaRandom = call test\n");
    lua_pushinteger(iState, rand());
    return 1;
}

int LuaEngExit(lua_State* iState) {
    printf("LuaEngExit = call test\n");
    return 0;
}

int LuaEngInfo(lua_State* iState) {
    Logger::logInfo("LuaEng> LUA = %s", LUA_RELEASE);
    Logger::logInfo("LuaEng> LUA.size = %llu", ScriptManager::getInstance().getMemory());
    return 0;
}


LUALIB_API luaL_Reg LuaEngLib[] = {
    {"Exit", LuaEngExit},
    {"Info", LuaEngInfo},
    {NULL, NULL}    //sentinel
};


int LuaOpenEngLib(lua_State* val) {
    luaL_newlib(val, LuaEngLib);
    return 1;
}

int LuaInclude(lua_State* iState) {
    ScriptManager& eng = ScriptManager::getInstance();
    const s8* fnm;  //relative path

    s32 cnt = lua_gettop(iState);
    if (1 != cnt || !lua_isstring(iState, 1)) {
        Logger::logError("LuaEng> LuaInclude fail=param");
        goto GT_LUA_FAIL;
    }

    fnm = lua_tostring(iState, 1);
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
    lua_pushnumber(iState, 1);
    return 1;

    GT_LUA_FAIL:
    lua_pushnil(iState);
    return 0;
}

void LuaCreateArray(lua_State* val, s8** arr, int cnt) {
    lua_newtable(val);
    for (ssz j = 0; j < cnt; ++j) {
        lua_pushlstring(val, arr[j], strlen(arr[j]));
        lua_rawseti(val, -2, j + 1);
    }
}

}//namespace script
}//namespace app
