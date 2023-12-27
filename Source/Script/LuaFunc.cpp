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
s32 LuaLog(lua_State* vm) {
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

s32 LuaPanic(lua_State* vm) {
    const s8* errmsg = "none";
    size_t elen = 4;
    if (lua_type(vm, -1) == LUA_TSTRING) {
        errmsg = (s8*)lua_tolstring(vm, -1, &elen);
    }
    Logger::logCritical("LuaEng panic vm=%p, emsg=%.*s", vm, (s32)elen, errmsg);
    Logger::flush();
    return 0;
}


s32 LuaErrorFunc(lua_State* vm) {
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


s32 LuaDumpStack(lua_State* vm) {
    s32 top = lua_gettop(vm);
    Logger::logCritical("VM = %p, cnt = %d, DumpStack:", vm, top);
    for (s32 i = 1; i <= top; i++) {
        switch (lua_type(vm, i)) {
        case LUA_TNUMBER:
            Logger::logCritical("\t%d\t%s\t%g", i, luaL_typename(vm, i), lua_tonumber(vm, i));
            break;
        case LUA_TSTRING:
            Logger::logCritical("\t%d\t%s\t%s", i, luaL_typename(vm, i), lua_tostring(vm, i));
            break;
        case LUA_TBOOLEAN:
            Logger::logCritical("\t%d\t%s\t%s", i, luaL_typename(vm, i), (lua_toboolean(vm, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            Logger::logCritical("\t%d\t%s\t%s", i, luaL_typename(vm, i), "nil");
            break;
        case LUA_TUSERDATA:
        {
            luaL_getmetafield(vm, i, "__name");
            const char* name = lua_tostring(vm, -1);
            Logger::logCritical("\t%d\t%s\t%s", i, luaL_typename(vm, i), name);
            lua_pop(vm, 1);
            break;
        }
        default:
            Logger::logCritical("\t%d\t%s\t%p", i, luaL_typename(vm, i), lua_topointer(vm, i));
            break;
        }
    }
    return 0;
}


s32 LuaRandom(lua_State* vm) {
    lua_pushinteger(vm, rand());
    return 1;
}

s32 LuaEngInfo(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    Logger::logInfo("LuaEng> LUA.Version = %s", LUA_RELEASE);
    Logger::logInfo("LuaEng> LUA.MemSize = %llu", ScriptManager::getInstance().getMemory());
    Logger::logInfo("LuaEng> LUA.StackSize = %d", cnt);
    return 0;
}


luaL_Reg LuaEngLib[] = {
    {"dumpStack", LuaDumpStack}, {"getRandom", LuaRandom}, {"showInfo", LuaEngInfo}, {NULL, NULL} // sentinel
};


s32 LuaOpenEngLib(lua_State* vm) {
    luaL_newlib(vm, LuaEngLib);
    return 1;
}

s32 LuaInclude(lua_State* vm) {
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


// regist class in a lua table
s32 LuaRegistClass(lua_State* vm, const luaL_Reg* func, const usz funcsz, const s8* className, const s8* namespac) {
    DASSERT(vm && func && funcsz && className);
    if (!vm || !func || 0 == funcsz || !className || 0 == className[0]) {
        Logger::logCritical("LuaRegistClass: bad params");
        return EE_INVALID_PARAM;
    }
    bool use_space = (namespac && 0 != namespac[0]);
    String reg_str(use_space ? namespac : "");
    if (use_space) {
        reg_str += '_';
    }
    reg_str += className;
    if (1 != luaL_newmetatable(vm, reg_str.c_str())) {
        Logger::logCritical("LuaRegistClass: err, class[%s] had been registed", reg_str.c_str());
        return EE_ERROR;
    }
    s32 metatable = lua_gettop(vm);
    lua_pushvalue(vm, -1);           // repush metatable pointer
    lua_setfield(vm, -2, "__index"); // pop 1
    /* mate function:
    lua_pushstring(vm, "__index");
    lua_pushstring(vm, "__newindex");
    lua_pushstring(vm, "__gc");
    lua_pushstring(vm, "__tostring");
    lua_pushstring(vm, "__eq");
    */
    for (usz i = 0; i < funcsz; ++i) {
        if (nullptr == func[i].name || 0 == func[i].name[0] || nullptr == func[i].func) {
            continue;
        }

        if (App4Char2S32("new") != App4Char2S32(func[i].name)) {
            lua_pushcfunction(vm, func[i].func);
            lua_setfield(vm, metatable, func[i].name);
        } else {
            Logger::logCritical("LuaRegistClass: reg_calss = %s::%s", namespac, className);
            if (use_space) {
                lua_getglobal(vm, namespac);
                if (lua_isnil(vm, -1)) { // Create namespace if not exsit
                    lua_pop(vm, 1);
                    lua_newtable(vm);
                    lua_pushvalue(vm, -1);
                    lua_setglobal(vm, namespac);
                }
                lua_pushcfunction(vm, func[i].func); // lua_pushvalue(vm, metatable);
                lua_setfield(vm, -2, className);
                lua_pop(vm, 1);
                // ScriptManager::dumpStack(vm);
            } else {
                lua_pushcfunction(vm, func[i].func); // lua_pushvalue(vm, metatable);
                lua_setglobal(vm, className);
            }
        }
    }
    lua_pop(vm, 1); // pop metatable
    DASSERT(0 == lua_gettop(vm));

    return EE_OK;
}

} // namespace script
} // namespace app
