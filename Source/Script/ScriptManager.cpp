#include "Script/ScriptManager.h"
#include <stdarg.h>
#include "Logger.h"
#include "Engine.h"
#include "Script/LuaFunc.h"
#include "Script/LuaRegClass.h"

namespace app {
namespace script {

static const void* const G_LUA_THREAD = "GLUA_THREAD_TABLE";

static const s8* G_LOAD_INFO = "--just for test\n"
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


/**
 * LuaThread
 */
LuaThread::LuaThread() :
    mSubVM(nullptr), mRef(LUA_NOREF), mParamCount(0), mRetCount(0), mStatus(0), mCaller(nullptr), mUserData(nullptr) {
}
LuaThread::~LuaThread() {
}
void LuaThread::operator()() {
    if (mCaller) {
        mCaller(mUserData);
    }
}



ScriptManager::ScriptManager() {
    initialize();
}

ScriptManager::~ScriptManager() {
    uninit();
}

ScriptManager& ScriptManager::getInstance() {
    static ScriptManager ret;
    return ret;
}


void ScriptManager::uninit() {
    removeAll();
    if (mRootVM) {
        lua_close(mRootVM);
        mRootVM = nullptr;
    }
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
    lua_settable(mRootVM, -3); // pop 2
    lua_pushstring(mRootVM, "cpath");
    lua_pushstring(mRootVM, tmp.c_str());
    lua_settable(mRootVM, -3); // pop 2
    lua_pop(mRootVM, 1);       // pop package

    // global fuctions
    lua_register(mRootVM, "Log", LuaLog);
    lua_register(mRootVM, "Include", LuaInclude);

    // global val
    lua_pushinteger(mRootVM, 1000ULL);
    lua_setglobal(mRootVM, "GVersion"); // set and pop 1
    lua_pushstring(mRootVM, "1.0.0.0");
    lua_setglobal(mRootVM, "GVerName");
    lua_pushstring(mRootVM, getScriptPath().c_str());
    lua_setglobal(mRootVM, "GPath");
    for (s32 i = ELL_DEBUG; i < ELL_COUNT; ++i) {
        lua_pushinteger(mRootVM, i);
        lua_setglobal(mRootVM, AppLogLevelNames[i]);
    }

    // lib Eng
    luaL_requiref(mRootVM, "Eng", LuaOpenEngLib, 1); // 1=global
    lua_pop(mRootVM, 1);                             // pop lib

    s32 cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);

    LuaRegRequest(mRootVM);
    LuaRegFile(mRootVM);
    LuaRegColor(mRootVM);

    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);

    // register table for lua thread(grab,drop)
    lua_pushlightuserdata(mRootVM, const_cast<void*>(G_LUA_THREAD));
    lua_newtable(mRootVM);
    lua_rawset(mRootVM, LUA_REGISTRYINDEX);

    lua_atpanic(mRootVM, LuaPanic);

    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
}

usz ScriptManager::getMemory() {
    return lua_gc(mRootVM, LUA_GCCOUNT, 0) * 1024ULL;
}

s32 ScriptManager::makeGC() {
    return lua_gc(mRootVM, LUA_GCCOLLECT);
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
    nd.execFunc(mRootVM, "main"); // call the function after exec file
    cnt = lua_gettop(mRootVM);
    DASSERT(0 == cnt);
    return true;
}

lua_State* ScriptManager::getRootVM() const {
    return mRootVM;
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

void ScriptManager::setENV(lua_State* vm, bool popCTX, const s8* ctx_name) {
    luaL_checktype(vm, -1, LUA_TFUNCTION); // func of coroutine

    ctx_name = (ctx_name && ctx_name[0]) ? ctx_name : "VContext";
    lua_createtable(vm, 0, 2); // 1. ctx_table
    s32 cnt = lua_gettop(vm);

    /* create a new env
     * env = {}
     * env["_G"] = env
     * env.metatable = new table {__index = _G}
     */
    lua_createtable(vm, 0, 2); // 2. env
    lua_pushvalue(vm, -1);
    lua_setfield(vm, -2, "_G");

    lua_createtable(vm, 0, 1); // 3. new table for the new env
    // lua_pushnil(vm);
    lua_pushglobaltable(vm);
    lua_setfield(vm, -2, "__index");

    lua_setmetatable(vm, -2); // env.metatable = new table, pop1: new table

    lua_pushvalue(vm, cnt);         // repush ctx table
    lua_setfield(vm, -2, ctx_name); // set ctx_name = ctx

    // stack: func/ctx_table/env
    const char* err = lua_setupvalue(vm, -3, 1); // pop1: env
    printf("setENV: up value name = %s\n", err);

    if (popCTX) {
        lua_pop(vm, 1); // pop1: ctx_table
    }
    // LuaDumpStack(vm);
}

lua_State* ScriptManager::createThread() {
    s32 cnt = lua_gettop(mRootVM);

    // 1.new thread
    lua_State* vm = lua_newthread(mRootVM);

    // 2.reg table
    lua_pushlightuserdata(mRootVM, const_cast<void*>(G_LUA_THREAD));
    lua_rawget(mRootVM, LUA_REGISTRYINDEX);

    // 3.key
    lua_pushlightuserdata(mRootVM, static_cast<void*>(vm));

    // 4.value (the vm)
    lua_pushvalue(mRootVM, -3);

    // grab coroutine, and pop 2: key,value
    lua_settable(mRootVM, -3);

    lua_settop(mRootVM, cnt);
    // LuaDumpStack(mRootVM);
    // LuaDumpStack(vm);

    // lua_xmove(mRootVM, ret, 1);
    return vm;
}

void ScriptManager::deleteThread(lua_State*& vm) {
    if (!vm) {
        return;
    }
    s32 cnt = lua_gettop(mRootVM);

    // 1.reg table
    lua_pushlightuserdata(mRootVM, const_cast<void*>(G_LUA_THREAD));
    lua_rawget(mRootVM, LUA_REGISTRYINDEX);

    // 3.key
    lua_pushlightuserdata(mRootVM, static_cast<void*>(vm));

    // 4.value
    lua_pushnil(mRootVM);

    // drop coroutine, and pop 2: key,value
    lua_settable(mRootVM, -3);

    lua_settop(mRootVM, cnt);
    // ref = LUA_NOREF;
    vm = nullptr;

    // lua_close(vm);  //don't close
    // root VM not close, just gc collect object
    cnt = makeGC();
    printf("deleteThread: GC=%d\n", cnt);
}

void ScriptManager::getThread(lua_State* vm) {
    LuaDumpStack(vm);

    // 1.reg table
    lua_pushlightuserdata(mRootVM, const_cast<void*>(G_LUA_THREAD));
    lua_rawget(mRootVM, LUA_REGISTRYINDEX);

    // 2.key
    lua_pushlightuserdata(mRootVM, static_cast<void*>(vm));

    lua_gettable(mRootVM, -2);
    LuaDumpStack(vm);
}


void ScriptManager::resumeThread(LuaThread& co) {
    co.mStatus = Script::resumeThread(co.mSubVM, co.mParamCount, co.mRetCount);
}

} // namespace script
} // namespace app
