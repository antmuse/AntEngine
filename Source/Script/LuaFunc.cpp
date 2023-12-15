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
        Logger::log(static_cast<ELogLevel>(cnt < ELL_COUNT ? cnt : ELL_CRITICAL), "LuaEng> %s", lua_tostring(vm, 2));
        return 0;
    }
    Logger::logError("LuaEng> invalid log param, cnt=%u", cnt);
    return 0;
}

int LuaPanic(lua_State* vm) {
    const s8* errmsg = "none";
    size_t elen = 4;
    if (lua_type(vm, -1) == LUA_TSTRING) {
        errmsg = (s8*)lua_tolstring(vm, -1, &elen);
    }
    Logger::logCritical("LuaEng panic vm=%p, emsg=%.*s", vm, (s32)elen, errmsg);
    Logger::flush();
    return 0;
}


int LuaErrorFunc(lua_State* vm) {
    lua_Debug debug = {};
    s32 frame = 0;
    while (lua_getstack(vm, frame, &debug)) {
        if (lua_type(vm, -1) != LUA_TSTRING) {
            Logger::logCritical("LuaEngErr vm=%p, frame=%d, line=%d, NotStrErr", vm, frame, debug.currentline);
            continue;
        }
        Logger::logCritical("LuaEngErr vm=%p, frame=%d, line=%d, emsg=%s, eshort=%s", vm, frame, debug.currentline,
            lua_tostring(vm, -1), debug.short_src);
        ++frame;
    }
    return 0;
}


void LuaDumpStack(lua_State* vm) {
    s32 top = lua_gettop(vm);
    printf("VM = %p, cnt = %d, DumpStack:\n", vm, top);
    for (s32 i = 1; i <= top; i++) {
        printf("\t%d\t%s\t", i, luaL_typename(vm, i));
        switch (lua_type(vm, i)) {
        case LUA_TNUMBER:
            printf("\t%g\n", lua_tonumber(vm, i));
            break;
        case LUA_TSTRING:
            printf("\t%s\n", lua_tostring(vm, i));
            break;
        case LUA_TBOOLEAN:
            printf("\t%s\n", (lua_toboolean(vm, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf("\t%s\n", "nil");
            break;
        case LUA_TUSERDATA:
        {
            luaL_getmetafield(vm, i, "__name");
            const char* name = lua_tostring(vm, -1);
            printf("\t%s\n", name);
            lua_pop(vm, 1);
            break;
        }
        default:
            printf("\t%p\n", lua_topointer(vm, i));
            break;
        }
    }
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
    {"exit", LuaEngExit}, {"showInfo", LuaEngInfo}, {NULL, NULL} // sentinel
};


int LuaOpenEngLib(lua_State* vm) {
    luaL_newlib(vm, LuaEngLib);
    return 1;
}

int LuaInclude(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (1 != cnt || !lua_isstring(vm, 1)) {
        Logger::logError("LuaEng> LuaInclude fail=param");
        return 0;
    }

    const s8* fnm = lua_tostring(vm, 1); // relative path
    if (!fnm || '\0' == fnm[0]) {
        Logger::logError("LuaEng> LuaInclude fail=empty");
        return 0;
    }

    // load
    ScriptManager& eng = ScriptManager::getInstance();
    if (!eng.include(fnm)) {
        Logger::logError("LuaEng> LuaInclude fail=%s", fnm);
    }

    return 0;
}


// regist class as a global table
void LuaRegistClass(lua_State* vm, const luaL_Reg* func, const usz funcsz, const s8* className, const s8* namespac) {
    DASSERT(func && funcsz && className);

    luaL_newmetatable(vm, className);
    s32 metatable = lua_gettop(vm);

    lua_pushvalue(vm, -1);           // repush metatable pointer
    lua_setfield(vm, -2, "__index"); // pop 1

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
    lua_pop(vm, 1); // pop metatable
    DASSERT(0 == lua_gettop(vm));
}

} // namespace script
} // namespace app
