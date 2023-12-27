#include "Logger.h"
#include "Engine.h"
#include "HandleFile.h"
#include "Script/ScriptManager.h"
#include "Script/LuaFunc.h"

namespace app {
namespace script {

const s8* const G_LUA_FILE = "HandleFile";

s32 LuaFileOpen(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    if (3 != cnt || !lua_isuserdata(vm, 1) || !lua_isstring(vm, 2) || !lua_isinteger(vm, 3)) {
        Logger::logError("LuaEng> LuaNewFile err, cnt=%u", cnt);
        lua_pushboolean(vm, 0);
        return 1;
    }

    String path(lua_tostring(vm, 2));
    cnt = static_cast<s32>(lua_tointeger(vm, 3));
    HandleFile* nd = *reinterpret_cast<HandleFile**>(lua_touserdata(vm, 1));
    if (!nd) {
        Logger::logError("LuaEng> LuaNewFile err, file=%s", path.c_str());
        lua_pushboolean(vm, false);
        return 1;
    }

    if (EE_OK != nd->open(path, cnt)) {
        Logger::logError("LuaEng> LuaNewFile err, file=%s", path.c_str());
        lua_pushboolean(vm, false);
        return 1;
    }
    
    lua_pushboolean(vm, true);
    return 1;
}

s32 LuaFileNew(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    HandleFile* nd = new HandleFile();
    nd->grab();
    //lua_pushlightuserdata(vm, nd);
    void** vv = static_cast<void**>(lua_newuserdata(vm, sizeof(void*)));
    *vv = nd;
    luaL_getmetatable(vm, G_LUA_FILE);
    lua_setmetatable(vm, -2);
    return 1;
}

s32 LuaFileGetName(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    HandleFile* nd = *reinterpret_cast<HandleFile**>(lua_touserdata(vm, 1));
    if (nd) {
        Logger::logInfo("LuaEng> LuaGetFileName, file=%s", nd->getFileName().c_str());
        lua_pushfstring(vm, nd->getFileName().c_str());
    } else {
        lua_pushfstring(vm, "");
        Logger::logError("LuaEng> LuaGetFileName err, file=null");
    }
    return 1;
}

//lua func name = HandleFile:__tostring
s32 LuaFile2Str(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    HandleFile* nd = *reinterpret_cast<HandleFile**>(lua_touserdata(vm, 1));
    if (nd) {
        lua_pushfstring(vm, "LuaFile2Str, file=%s", nd->getFileName().c_str());
    } else {
        lua_pushstring(vm, "LuaFile2Str err, self.file=null");
    }
    return 1;
}

s32 LuaFileClose(lua_State* vm) {
    HandleFile** vv = reinterpret_cast<HandleFile**>(lua_touserdata(vm, 1));
    if (nullptr == (*vv)) {
        return 0;
    }
    HandleFile* nd = *vv;
    nd->drop();
    nd->launchClose();
    *vv = nullptr;
    return 0;
}

void OnLuaFileRead(RequestFD* vm) {

}

s32 LuaFileRead(lua_State* vm) {
    s32 cnt = lua_gettop(vm);
    HandleFile* nd = *reinterpret_cast<HandleFile**>(lua_touserdata(vm, 1));
    RequestFD* buf = reinterpret_cast<RequestFD*>(lua_touserdata(vm, 2));
    if (!nd || !buf) {
        lua_pushboolean(vm, false);
        return 1;
    }
    buf->mCall = OnLuaFileRead;
    
    lua_pushboolean(vm, EE_OK == nd->read(buf));
    return 1;
}

luaL_Reg LuaLibFile[] = {
    {"new", LuaFileNew},
    {"__gc", LuaFileClose},
    {"open", LuaFileOpen},
    {"close", LuaFileClose},
    {"getFileName", LuaFileGetName},
    {"read", LuaFileRead},
    {"__tostring", LuaFile2Str},
    {NULL, NULL}
};


s32 LuaRegFile(lua_State* vm) {
    return LuaRegistClass(vm, LuaLibFile, DSIZEOF(LuaLibFile), G_LUA_FILE, nullptr);
}


}//namespace script
}//namespace app
