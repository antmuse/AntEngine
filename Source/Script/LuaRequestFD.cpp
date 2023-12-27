#include "Logger.h"
#include "Loop.h"
#include "HandleFile.h"
#include "Script/LuaFunc.h"
#include "Script/LuaFunc.h"

namespace app {
namespace script {

const s8* const G_LUA_REQ = "RequestFD";

/**
 * @brief usage:
 * RequestFD.new()      //default 4K
 * RequestFD.new(566)   //1K
 * RequestFD.new(2048)  //2K
 */
s32 LuaReqNew(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    u32 sz = 4096;
    if (1 == cnt && lua_isinteger(vm, 1)) {
        sz = AppMax(static_cast<u32>(lua_tointeger(vm, 1)), 512U);
    }

    /*lightuserdata not support metatable:
    RequestFD* nd = RequestFD::newRequest(sz);
    lua_pushlightuserdata(vm, nd);*/
    void** vv = static_cast<void**>(lua_newuserdata(vm, sizeof(void*)));
    *vv = RequestFD::newRequest(sz);

    luaL_getmetatable(vm, G_LUA_REQ);
    lua_setmetatable(vm, -2);
    return 1;
}

s32 LuaReqWrite(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (1 != cnt || !lua_isstring(vm, 1)) {

        lua_pushinteger(vm, 0);
        return 1;
    }

    const s8* buf = lua_tostring(vm, 1);
    u32 blen = static_cast<u32>(strlen(buf));
    u32 wt = 0;
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));
    if (nd && nd->getWriteSize() >= blen) {
        memcpy(nd->mData + nd->mUsed, buf, blen);
        wt = blen;
    }


    lua_pushinteger(vm, wt);
    return 1;
}

s32 LuaReqGetData(lua_State* vm) {
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));

    lua_pushlightuserdata(vm, nd ? nd->mData : nullptr);
    return 1;
}

s32 LuaReqDel(lua_State* vm) {
    RequestFD** vv = reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));
    if (*vv) {
        RequestFD::delRequest(*vv);
        *vv = nullptr;
    }
    return 0;
}

s32 LuaReqGetSize(lua_State* vm) {
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));
    lua_pushinteger(vm, nd ? nd->mUsed : -1);
    return 1;
}

s32 LuaReqGetErr(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));

    lua_pushinteger(vm, nd ? nd->mError : EE_ERROR);
    return 1;
}

/**
 * @brief usage:
 * RequestFD.clearData()      //clear all
 * RequestFD.clearData(52)    //clear 52
 */
s32 LuaReqClearData(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    u32 sz = ~0U;
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));
    if (2 == cnt && lua_isinteger(vm, 2)) {
        sz = static_cast<u32>(lua_tointeger(vm, 2));
    }
    if (nd) {
        nd->clearData(sz);
    }

    return 0;
}

s32 LuaReqGetAllocated(lua_State* vm) {
    RequestFD* nd = *reinterpret_cast<RequestFD**>(lua_touserdata(vm, 1));
    lua_pushinteger(vm, nd ? nd->mAllocated : 0);
    return 1;
}

luaL_Reg LuaLibReq[] = {
    {"new", LuaReqNew},
    {"del", LuaReqDel},
    {"__gc", LuaReqDel},
    {"getSize", LuaReqGetSize},
    {"getAllocated", LuaReqGetAllocated},
    {"getData", LuaReqGetData},
    {"write", LuaReqWrite},
    {"getErr", LuaReqGetErr},
    {"clearData", LuaReqClearData},
    {NULL, NULL}
};


s32 LuaRegRequest(lua_State* vm) {
    return LuaRegistClass(vm, LuaLibReq, DSIZEOF(LuaLibReq), G_LUA_REQ, nullptr);
}


}//namespace script
}//namespace app
