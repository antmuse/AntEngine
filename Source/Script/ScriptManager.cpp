#include "Script/ScriptManager.h"
#include "Logger.h"
#include "Script/ScriptFunc.h"

namespace app {
namespace script {

static const s8* G_LOAD_INFO =
"local ver = GVersion\n"
"local ret = nil\n"
"function EngInfo(str)\n"
"  if str>100 then\n"
"    Log(Info, \"Lua=5.4.4, VER= \" .. tostring(ver))\n"
"  else\n"
"    Log(Info, \"Lua=5.4.4, VER2 = \" .. tostring(ver))\n"
"  end\n"
"  return ret\n"
"end\n"
"EngInfo(101)";


ScriptManager::ScriptManager() {
    initialize();
}

ScriptManager::~ScriptManager() {
    removeAll();
    lua_close(mLuaState);
    mLuaState = nullptr;
}

ScriptManager& ScriptManager::getInstance() {
    static ScriptManager ret;
    return ret;
}

void ScriptManager::initialize() {
    mLuaState = luaL_newstate();
    luaopen_base(mLuaState);   //open basic library
    luaopen_table(mLuaState);  //open table library
    //luaopen_io(mLuaState);   //open I/O library
    luaopen_string(mLuaState); //open string lib
    luaopen_math(mLuaState);   //open math lib

    //luaL_newlib(mLuaState,regLoader);

    //base data struct
    lua_register(mLuaState, "Color", LuaColor);

    //fuctions
    lua_register(mLuaState, "Log", LuaLog);

    //global val
    // lua_pushnumber(mLuaState, 101.0);
    lua_pushinteger(mLuaState, 1000ULL);
    lua_setglobal(mLuaState, "GVersion");
    lua_pushstring(mLuaState, "1.0.0.0");
    lua_setglobal(mLuaState, "GVerName");
    for (s32 i = ELL_DEBUG; i < ELL_COUNT; ++i) {
        lua_pushinteger(mLuaState, i);
        lua_setglobal(mLuaState, AppLogLevelNames[i]);
    }

    // test
    bool ret = execute("ScriptManager", G_LOAD_INFO, strlen(G_LOAD_INFO));
    Logger::logInfo("ScriptManager::initialize, ret = %s", ret ? "ok" : "no");
}

lua_State* ScriptManager::getEngine() const {
    return mLuaState;
}

void ScriptManager::pushParam(s32 prm) {
    ++mParamCount;
    lua_pushnumber(mLuaState, prm);
}

void ScriptManager::pushParam(s8* prm) {
    ++mParamCount;
    lua_pushstring(mLuaState, prm);
}

void ScriptManager::pushParam(void* pNode) {
    ++mParamCount;
    lua_pushlightuserdata(mLuaState, pNode);
}

void ScriptManager::popParam(s32 iSum) {
    lua_pop(mLuaState, iSum);
    --mParamCount;
}

bool ScriptManager::execute(const String& iName, const s8* const pBuffer, usz fSize) {
    Logger::logInfo("ScriptManager::execute, script=%s", iName.c_str());
    if (0 != luaL_loadbuffer(mLuaState, pBuffer, fSize, iName.c_str())) {
        printf("Script> func_name = %s\n", lua_tostring(mLuaState, -1));
        lua_pop(mLuaState, 1);
        return false;
    }
    s32 expr = lua_pcall(mLuaState, 0, 0, 0);
    if (expr) {
        printf("Script> func_name = %s\n", lua_tostring(mLuaState, -1));
        lua_pop(mLuaState, 1);
        return false;
    }
    return true;
}

bool ScriptManager::execute(const String& iName, const s8* const pBuffer, usz fSize, const s8* const pFunc) {
    Logger::logInfo("ScriptManager::execute, script=%s", iName.c_str());
    if (0 != luaL_loadbuffer(mLuaState, pBuffer, fSize, iName.c_str())) {
        Logger::logError("ScriptManager::execute, load = %s\n", lua_tostring(mLuaState, -1));
        lua_pop(mLuaState, 1);
        return false;
    }
    lua_getglobal(mLuaState, pFunc);
    s32 expr = lua_pcall(mLuaState, 0, 1, 0);
    if (expr) {
        Logger::logError("ScriptManager::execute, call = %s\n", lua_tostring(mLuaState, -1));
        lua_pop(mLuaState, 1);
        return false;
    }
    //lua_pop(mLuaState,1);
    return true;
}

bool ScriptManager::add(Script* script) {
    if (!script) {
        return false;
    }
    // Look if a script with given name doesn't already exist.
    if (getScript(script->getName())) {
        return false;
    }
    mAllScript.pushBack(script);
    script->grab();
    return true;
}

Script* ScriptManager::createScript(const String& fileName, bool grab) {
    String keyname(fileName);
    keyname.deletePathFromFilename();

    // Look if a script with given name doesn't already exist.
    if (grab && getScript(keyname)) {
        return nullptr;
    }

    // load the script.
    Script* ret = new Script(keyname, fileName);
    bool success = ret->load(true);
    if (!success) {
        ret->drop();
        Logger::logInfo("ScriptManager::loadScript", "Can not find script : %s", fileName.c_str());
        return nullptr;
    }
    Logger::logInfo("ScriptManager::loadScript", "find script : %s", keyname.c_str());
    if (grab) {
        mAllScript.pushBack(ret);
        ret->grab();
    }
    return ret;
}

Script* ScriptManager::getScript(const s32 id) {
    for (TList<Script*>::Iterator git = mAllScript.begin(); git != mAllScript.end(); git++) {
        if ((*git)->getID() == id) {
            return *git;
        }
    }
    return 0;
}

Script* ScriptManager::getScript(const String& name) {
    for (TList<Script*>::Iterator git = mAllScript.begin(); git != mAllScript.end(); git++) {
        if (name == (*git)->getName()) {
            return *git;
        }
    }
    return 0;
}

void ScriptManager::removeAll() {
    for (TList<Script*>::Iterator it = mAllScript.begin(); it != mAllScript.end(); it++) {
        (*it)->drop();
    }
    mAllScript.clear();
}

bool ScriptManager::remove(Script* script) {
    if (!script) {
        return false;
    }
    for (TList<Script*>::Iterator it = mAllScript.begin(); it != mAllScript.end(); it++) {
        if ((*it) == script) {
            (*it)->drop();
            mAllScript.erase(it);
            return true;
        }
    }
    return false;
}

bool ScriptManager::remove(const s32 id) {
    for (TList<Script*>::Iterator git = mAllScript.begin(); git != mAllScript.end(); git++) {
        if ((*git)->getID() == id) {
            (*git)->drop();
            mAllScript.erase(git);
            return true;
        }
    }
    return false;
}

bool ScriptManager::remove(const String& name) {
    for (TList<Script*>::Iterator git = mAllScript.begin(); git != mAllScript.end(); git++) {
        if (name == (*git)->getName()) {
            (*git)->drop();
            mAllScript.erase(git);
            return true;
        }
    }
    return false;
}

}//namespace script
}//namespace app
