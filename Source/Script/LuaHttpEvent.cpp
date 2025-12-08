#include "Logger.h"
#include "Loop.h"
#include "Net/HTTP/HttpEvtLua.h"
#include "Script/LuaFunc.h"
#include "Script/LuaFunc.h"

namespace app {
namespace script {

static const s8* const G_LUA_CLASSNAME = "HttpEvtLua";

s32 LuaHttpEventNew(lua_State* vm, HttpEvtLua* evt) {
    HttpEvtLua** vv = static_cast<HttpEvtLua**>(lua_newuserdata(vm, sizeof(void*)));
    *vv = evt;
    (*vv)->grab();
    luaL_getmetatable(vm, G_LUA_CLASSNAME);
    lua_setmetatable(vm, -2);
    return 1;
}
s32 LuaHttpEventNewDummy(lua_State* vm) {
    DLOG(ELL_ERROR, "LuaHttpEventNewDummy, can't go here.");
    lua_pushnil(vm);
    return 1;
}

s32 LuaHttpEventDel(lua_State* vm) {
    HttpEvtLua** vv = reinterpret_cast<HttpEvtLua**>(lua_touserdata(vm, 1));
    if (*vv) {
        (*vv)->drop();
        *vv = nullptr;
    }
    return 0;
}


s32 LuaHttpEventWriteLine(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (cnt < 3 || cnt > 4 || !lua_isuserdata(vm, 1) || !lua_isinteger(vm, 2) || !lua_isstring(vm, 3)
        || (4 == cnt && !lua_isinteger(vm, 4))) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    HttpEvtLua** nd = reinterpret_cast<HttpEvtLua**>(lua_touserdata(vm, 1));
    u16 num = lua_tointeger(vm, 2);
    const s8* val = lua_tostring(vm, 3);
    if (!nd || !(*nd) || !val) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    s64 bodyLen = 3 == cnt ? -1 : lua_tointeger(vm, 4); // use chunk mode if -1
    s32 ret = (*nd)->writeRespLine(num, val, bodyLen);
    lua_pushinteger(vm, ret);
    return 1;
}

s32 LuaHttpEventWriteHeader(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (3 != cnt || !lua_isuserdata(vm, 1) || !lua_isstring(vm, 2) || !lua_isstring(vm, 3)) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    HttpEvtLua** nd = reinterpret_cast<HttpEvtLua**>(lua_touserdata(vm, 1));
    const s8* name = lua_tostring(vm, 2);
    const s8* val = lua_tostring(vm, 3);
    if (!nd || !(*nd) || !name || !val) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    s32 ret = (*nd)->writeRespHeader(name, val);
    lua_pushinteger(vm, ret);
    return 1;
}

s32 LuaHttpEventWriteBody(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (2 != cnt || !lua_isuserdata(vm, 1) || !lua_isstring(vm, 2)) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    HttpEvtLua** nd = reinterpret_cast<HttpEvtLua**>(lua_touserdata(vm, 1));
    const s8* buf = lua_tostring(vm, 2);
    if (!nd || !(*nd) || !buf) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    s32 ret = (*nd)->writeRespBody(buf, strlen(buf));
    lua_pushinteger(vm, ret);
    return 1;
}



static s32 FuncYieldBack(lua_State* vm, s32 status, lua_KContext ctx) {
    DASSERT(status == LUA_YIELD);
    return static_cast<s32>(ctx); // ctx is the count of return values.
}
s32 LuaHttpEventSendPart(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (2 != cnt || !lua_isuserdata(vm, 1) || !lua_isinteger(vm, 2)) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    HttpEvtLua** nd = reinterpret_cast<HttpEvtLua**>(lua_touserdata(vm, 1));
    if (!nd || !(*nd)) {
        lua_pushinteger(vm, EE_INVALID_PARAM);
        return 1;
    }
    s32 step = lua_tointeger(vm, 2);
    s32 ret = (*nd)->sendResp(step);
    if (HttpEvtLua::RSTEP_BODY_END & step) {
        (*nd)->drop();
        (*nd) = nullptr;
    }
    lua_pushinteger(vm, ret); // return 1 val to lua
    if (EE_OK == ret) {
        lua_yieldk(vm, 0, (lua_KContext)(1), FuncYieldBack); // return here on success
    }
    // not yield on error
    DLOG(ELL_ERROR, "LuaHttpEventSendPart, ecode = %d", ret);
    return 1;
}



/*
 * @brief can't call new() to create G_LUA_CLASSNAME in lua.
 * just make a matetable for G_LUA_CLASSNAME.
 */
luaL_Reg LuaLibHttpEvent[] = {{"new", LuaHttpEventNewDummy}, {"del", LuaHttpEventDel}, {"__gc", LuaHttpEventDel},
    {"writeLine", LuaHttpEventWriteLine}, {"writeHead", LuaHttpEventWriteHeader}, {"writeBody", LuaHttpEventWriteBody},
    {"sendResp", LuaHttpEventSendPart}, {NULL, NULL}};


s32 LuaRegHttpEvent(lua_State* vm) {
    Script::setGlobalVal(vm, "HTTP_HEAD_END", (s64)HttpEvtLua::RSTEP_HEAD_END);
    Script::setGlobalVal(vm, "HTTP_BODY_PART", (s64)HttpEvtLua::RSTEP_BODY_PART);
    Script::setGlobalVal(vm, "HTTP_BODY_END", (s64)HttpEvtLua::RSTEP_BODY_END);
    return LuaRegistClass(vm, LuaLibHttpEvent, DSIZEOF(LuaLibHttpEvent), G_LUA_CLASSNAME, nullptr);
}


} // namespace script
} // namespace app
