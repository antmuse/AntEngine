#include "Script/ScriptFileHandle.h"
#include "Logger.h"
#include "Engine.h"
#include "HandleFile.h"
#include "Script/ScriptManager.h"
#include "Script/Script.h"

namespace app {
namespace script {

const s8* const G_LUA_FILE = "HandleFile";

int LuaOpenFile(lua_State* val) {
    s32 cnt = lua_gettop(val);
    if (3 != cnt || !lua_isuserdata(val, 1) || !lua_isstring(val, 2) || !lua_isinteger(val, 3)) {
        Logger::logError("LuaEng> LuaNewFile err, cnt=%u", cnt);
        lua_pushboolean(val, 0);
        return 1;
    }

    String path(lua_tostring(val, 2));
    cnt = static_cast<s32>(lua_tointeger(val, 3));
    HandleFile* nd = reinterpret_cast<HandleFile*>(lua_touserdata(val, 1));
    if (!nd) {
        Logger::logError("LuaEng> LuaNewFile err, file=%s", path.c_str());
        lua_pushboolean(val, 0);
        return 1;
    }

    if (EE_OK != nd->open(path, cnt)) {
        //nd->drop();
        Logger::logError("LuaEng> LuaNewFile err, file=%s", path.c_str());
        lua_pushboolean(val, 0);
        return 1;
    }

    lua_pushboolean(val, 1);
    return 1;
}

int LuaNewFile(lua_State* val) {
    HandleFile* nd = new HandleFile();
    nd->grab();
    lua_pushlightuserdata(val, nd);
    luaL_getmetatable(val, G_LUA_FILE);
    lua_setmetatable(val, -2);
    return 1;
}

int LuaGetFileName(lua_State* val) {
    HandleFile* nd = reinterpret_cast<HandleFile*>(lua_touserdata(val, 1));
    if (nd) {
        Logger::logInfo("LuaEng> LuaGetFileName, file=%s", nd->getFileName().c_str());
        lua_pushfstring(val, nd->getFileName().c_str());
    } else {
        lua_pushfstring(val, "");
        Logger::logError("LuaEng> LuaGetFileName err, file=null");
    }
    return 1;
}

int LuaDelFile(lua_State* val) {
    HandleFile* nd = reinterpret_cast<HandleFile*>(lua_touserdata(val, 1));
    if (nd) {
        if (0 == nd->drop()) {
            Logger::logInfo("LuaEng> LuaDelFile, file=%s", nd->getFileName().c_str());
            delete nd;
        }
    } else {
        Logger::logError("LuaEng> LuaDelFile err, file=null");
    }
    return 0;
}

int LuaFile2Str(lua_State* val) {
    HandleFile* nd = reinterpret_cast<HandleFile*>(lua_touserdata(val, 1));
    if (nd) {
        Logger::logInfo("LuaEng> LuaFile2Str, file=%s", nd->getFileName().c_str());
        lua_pushfstring(val, "LuaFile2Str, file=%s", nd->getFileName().c_str());
    } else {
        Logger::logError("LuaEng> LuaDelFile err, file=null");
        lua_pushfstring(val, "LuaDelFile err, file=null");
    }
    return 1;
}

LUALIB_API luaL_Reg LuaLibFile[] = {
    {"new", LuaNewFile},
    {"open", LuaOpenFile},
    {"getFileName", LuaGetFileName},
    {"__gc", LuaDelFile},
    {"__tostring", LuaFile2Str},
    {NULL, NULL}    //sentinel
};


int LuaOpenLibFile(lua_State* val) {
    luaL_newmetatable(val, G_LUA_FILE);
    lua_pushvalue(val, -1);
    lua_setfield(val, -2, "__index");
    luaL_setfuncs(val, LuaLibFile, 0);

    /*
    luaL_newmetatable( L, ¡°ObjectMetatable");
    lua_pushstring( L, "__index" );
    lua_pushcfunction( L, GetValue );
    lua_rawset( L, -3 ); // ObjectMetatable.__index = GetHorizonValue

    lua_pushstring( L, "__newindex" );
    lua_pushcfunction( L, SetValue );
    lua_rawset( L, -3 ); // ObjectMetatable.__newindex = GetHorizonValue
    */

    luaL_newlib(val, LuaLibFile);
    return 1;
}


}//namespace script
}//namespace app
