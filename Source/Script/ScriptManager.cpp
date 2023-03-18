#include "Script/ScriptManager.h"
#include <stdarg.h>
#include "Logger.h"
#include "Engine.h"
#include "Script/LuaFunc.h"
#include "Script/LuaRegClass.h"

namespace app {
namespace script {

static const s8* G_LOAD_INFO =
"--just for test\n"
"local ver = GVersion\n"
"function EngInfo(val)\n"
"  Eng.showInfo()\n"
"  Log(Info, \"LuaEng> package.path = \" .. package.path)\n"
"  if val>100 then\n"
"    Log(Info, \"LuaEng> GPath = \" .. GPath)\n"
"  else\n"
"    Log(Warn, \"LuaEng> GPath = \" .. GPath)\n"
"  end\n"
"end\n"
"EngInfo(ver)";


static const s8* LuaReader(lua_State* state, void* user, usz* sz) {
    return "";
}

static s32 LuaWriter(lua_State* state, const void* p, usz sz, void* user) {
    return 0;
}

ScriptManager::ScriptManager() {
    initialize();
}

ScriptManager::~ScriptManager() {
    removeAll();
    if (mRootVM) {
        lua_close(mRootVM);
        mRootVM = nullptr;
    }
}

ScriptManager& ScriptManager::getInstance() {
    static ScriptManager ret;
    return ret;
}

void ScriptManager::initialize() {
    mScriptPath = Engine::getInstance().getAppPath();
    mScriptPath += "Script/";

    mRootVM = luaL_newstate();
    luaL_openlibs(mRootVM);

    // fix  package.path, package.cpath
    String tmp = mScriptPath;
    tmp += "?.lua";
    lua_getglobal(mRootVM, "package");
    lua_pushstring(mRootVM, "path");
    lua_pushstring(mRootVM, tmp.c_str());
    lua_settable(mRootVM, -3); //pop 2
    lua_pushstring(mRootVM, "cpath");
    lua_pushstring(mRootVM, tmp.c_str());
    lua_settable(mRootVM, -3); //pop 2
    lua_pop(mRootVM, 1);       //pop package

    //base data struct
    lua_register(mRootVM, "Color", LuaColor);

    //global fuctions
    lua_register(mRootVM, "Log", LuaLog);
    lua_register(mRootVM, "Include", LuaInclude);

    //global val
    lua_pushinteger(mRootVM, 1000ULL);
    lua_setglobal(mRootVM, "GVersion");  //set and pop 1
    lua_pushstring(mRootVM, "1.0.0.0");
    lua_setglobal(mRootVM, "GVerName");
    lua_pushstring(mRootVM, getScriptPath().c_str());
    lua_setglobal(mRootVM, "GPath");
    for (s32 i = ELL_DEBUG; i < ELL_COUNT; ++i) {
        lua_pushinteger(mRootVM, i);
        lua_setglobal(mRootVM, AppLogLevelNames[i]);
    }

    //lib Eng
    luaL_requiref(mRootVM, "Eng", LuaOpenEngLib, 1);          // 1=global
    lua_pop(mRootVM, 1); //pop lib

    s32 cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);

    LuaRegRequest(mRootVM);
    LuaRegFile(mRootVM);

    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
}

usz ScriptManager::getMemory() {
    return lua_gc(mRootVM, LUA_GCCOUNT, 0) * 1024ULL;
}


bool ScriptManager::loadFirstScript() {
    Script nd;
    if (!nd.loadBuf(mRootVM, "ScriptManager", G_LOAD_INFO, strlen(G_LOAD_INFO), true)) {
        return false;
    }
    s32 cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
    bool ret = nd.exec(mRootVM, nullptr, 0, 0, 0);
    Logger::logInfo("ScriptManager::initialize, ret = %s", ret ? "ok" : "no");
    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);

    if (!nd.load(mRootVM, getScriptPath(), "init.lua", true)) {
        return false;
    }
    nd.exec(mRootVM); // exec the file
    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
    nd.execFunc(mRootVM, "main");  // call the function after exec file
    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
    return true;
}

lua_State* ScriptManager::getRootVM() const {
    return mRootVM;
}

void ScriptManager::pushParam(s32 prm) {
    ++mParamCount;
    lua_pushnumber(mRootVM, prm);
}

void ScriptManager::pushParam(s8* prm) {
    ++mParamCount;
    lua_pushstring(mRootVM, prm);
}

void ScriptManager::pushParam(void* pNode) {
    ++mParamCount;
    lua_pushlightuserdata(mRootVM, pNode);
}

void ScriptManager::popParam(s32 iSum) {
    lua_pop(mRootVM, iSum);
    --mParamCount;
}

bool ScriptManager::include(const String& fileName) {
    if (!Script::isLoaded(mRootVM, fileName)) {
        Script nd;
        if (!nd.load(mRootVM, getScriptPath(), fileName, true) || !nd.exec(mRootVM)) {
            return false;
        }
    }
    return true;
}

bool ScriptManager::add(Script* script) {
    if (!script) {
        return false;
    }
    if (getScript(script->getName())) {
        return false;
    }
    mAllScript.insert(script->getName(), script);
    return true;
}

Script* ScriptManager::createScript(const String& keyname) {
    Script* ret = new Script();
    bool success = ret->load(mRootVM, getScriptPath(), keyname, true);
    if (!success) {
        Logger::logError("ScriptManager::createScript,fail load script= %s", keyname.c_str());
        return nullptr;
    }
    Logger::logInfo("ScriptManager::createScript,load script= %s", keyname.c_str());
    return ret;
}

Script* ScriptManager::loadScript(const String& fileName) {
    Script* ret = getScript(fileName);
    if (ret) {
        return ret;
    }
    ret = createScript(fileName);
    if (ret) {
        mAllScript.insert(ret->getName(), ret);
    }
    return ret;
}

Script* ScriptManager::getScript(const String& name) {
    TMap<String, Script*>::Node* nd = mAllScript.find(name);
    return nd ? nd->getValue() : nullptr;
}

void ScriptManager::removeAll() {
    for (TMap<String, Script*>::Iterator it = mAllScript.getIterator(); !it.atEnd(); ++it) {
        delete it->getValue();
    }
    mAllScript.clear();
}

bool ScriptManager::remove(Script* script) {
    if (!script) {
        return false;
    }
    return remove(script->getName());
}

bool ScriptManager::remove(const String& name) {
    TMap<String, Script*>::Node* nd = mAllScript.find(name);
    if (!nd) {
        return false;
    }
    delete nd->getValue();
    mAllScript.remove(nd);
    return true;
}

lua_State* ScriptManager::createThread() {
    /* 创建Lua协程，返回的新lua_State跟原有的lua_State共享所有的全局对象（如表），
     * 但是有一个独立的执行栈。 协程依赖垃圾回收销毁 */
    lua_State* ret = lua_newthread(mRootVM);

    /* 将lua虚拟机VM栈上的入口函数闭包移到新创建的协程栈上，
       这样新协程就有了虚拟机已经解析完毕的代码了。*/
       //lua_xmove(mRootVM, ret, 1);
    return ret;
}

s32 ScriptManager::resumeThread(lua_State* vm, s32& ret_cnt) {
    ret_cnt = 0;
    s32 ret = lua_resume(vm, NULL, 0, &ret_cnt);
    if (LUA_YIELD != ret) {
        Logger::logError("ScriptManager::resumeThread, fail ret = %d", ret);
        return EE_ERROR;
    }
    return EE_OK;
}

void ScriptManager::deleteThread(lua_State* vm) {

}

}//namespace script
}//namespace app
