#include "Script/ScriptManager.h"
#include <stdarg.h>
#include "Logger.h"
#include "Engine.h"
#include "Script/ScriptFunc.h"
#include "Script/ScriptFileHandle.h"

namespace app {
namespace script {

static const s8* G_LOAD_INFO =
"--just for test\n"
"local ver = GVersion\n"
"function EngInfo(val)\n"
"  Eng.Info()\n"
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
    if (mLuaState) {
        lua_close(mLuaState);
        mLuaState = nullptr;
    }
}

ScriptManager& ScriptManager::getInstance() {
    static ScriptManager ret;
    return ret;
}

void ScriptManager::initialize() {
    mScriptPath = Engine::getInstance().getAppPath();
    mScriptPath += "Script/";

    mLuaState = luaL_newstate();
    luaL_openlibs(mLuaState);

    // fix  package.path, package.cpath
    String tmp = mScriptPath;
    tmp += "?.lua";
    lua_getglobal(mLuaState, "package");
    lua_pushstring(mLuaState, "path");
    lua_pushstring(mLuaState, tmp.c_str());
    lua_settable(mLuaState, -3);
    lua_getglobal(mLuaState, "package");
    lua_pushstring(mLuaState, "cpath");
    lua_pushstring(mLuaState, tmp.c_str());
    lua_settable(mLuaState, -3);
    //lua_newtable(mLuaState); // same as lua_createtable(L, 0, 0);

    //lib Eng
    luaL_requiref(mLuaState, "Eng", LuaOpenEngLib, 1);          // 1=global
    luaL_requiref(mLuaState, "HandleFile", LuaOpenLibFile, 1);

    //base data struct
    lua_register(mLuaState, "Color", LuaColor);

    //global fuctions
    lua_register(mLuaState, "Log", LuaLog);
    lua_register(mLuaState, "Include", LuaInclude);

    //global val
    lua_pushinteger(mLuaState, 1000ULL);
    lua_setglobal(mLuaState, "GVersion");
    lua_pushstring(mLuaState, "1.0.0.0");
    lua_setglobal(mLuaState, "GVerName");
    lua_pushstring(mLuaState, getScriptPath().c_str());
    lua_setglobal(mLuaState, "GPath");
    for (s32 i = ELL_DEBUG; i < ELL_COUNT; ++i) {
        lua_pushinteger(mLuaState, i);
        lua_setglobal(mLuaState, AppLogLevelNames[i]);
    }
}

usz ScriptManager::getMemory() {
    return lua_gc(mLuaState, LUA_GCCOUNT, 0) * 1024ULL;
}


bool ScriptManager::loadFirstScript() {
    // test
    bool ret = exec("ScriptManager", G_LOAD_INFO, strlen(G_LOAD_INFO), nullptr, 0, 1, 0);
    Logger::logInfo("ScriptManager::initialize, ret = %s", ret ? "ok" : "no");

    Script* nd = loadScript("init.lua");
    callFunc("main");
    // callFunc("main2", 0, "d", 2345);
    //exec(nd, "main");
    return true;
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

bool ScriptManager::include(const String& fileName) {
    //TODO
    //Script* nd = ;
    //if (nd) {
    //    return exec(nd->getName(), nd->getBuffer(), nd->getBufferSize(), nullptr, 0, LUA_MULTRET, 0);;
    //}

    // luaL_dofile();
    // luaL_loadfile();
    // luaL_loadfile();
    // lua_load(mLuaState, LuaReader, nullptr, "chunkname", "rb");
    // lua_pcall(mLuaState, 0,LUA_MULTRET,0);
    return nullptr != loadScript(fileName);
}

bool ScriptManager::exec(const Script& it, const s8* func,
    s32 nargs, s32 nresults, s32 errfunc) {
    return exec(it.getName(), it.getBuffer(), it.getBufferSize(), func, nargs, nresults, errfunc);
}

bool ScriptManager::exec(const String& iName, const s8* const pBuffer, usz fSize,
    const s8* func, s32 nargs, s32 nresults, s32 errfunc) {
    if (!pBuffer || 0 == fSize) {
        return false;
    }
    if (0 != luaL_loadbuffer(mLuaState, pBuffer, fSize, iName.c_str())) {
        Logger::logError("ScriptManager::exec, load err = %s, name=%s\n",
            lua_tostring(mLuaState, -1), iName.c_str());
        lua_pop(mLuaState, 1);  // pop lua err
        return false;
    }

    if (func) {
        lua_getglobal(mLuaState, func);
    }

    /**
     * lua_pcall参数:
     * 传入nargs个参数,期望得到nresults个返回值,errfunc表示错误处理函数在栈中的索引值,
     * 压入结果前会弹出函数和参数*/
    s32 expr = lua_pcall(mLuaState, nargs, nresults, errfunc);
    if (expr) {
        Logger::logError("ScriptManager::exec, call err = %s, name=%s\n",
            lua_tostring(mLuaState, -1), iName.c_str());
        lua_pop(mLuaState, 1);  // pop lua err
        return false;
    }
    return true;
}


bool ScriptManager::callWith(const s8* cFuncName, s32 nResults, const s8* cFormat, va_list& vlist) {
    if (!cFuncName) {
        return false;
    }
    f64	nNumber = 0;
    s32 nInteger = 0;
    s8* cString = nullptr;
    void* pPoint = nullptr;

    s32 param_cnt = 0;
    lua_CFunction CFunc = nullptr;

    lua_getglobal(mLuaState, cFuncName); //在堆栈中加入需要调用的函数名

    if (cFormat) {
        for (s32 i = 0; '\0' != cFormat[i]; ++i) {
            switch (cFormat[i]) {
            case 'f':  // 输入的数据是Double型
            {
                nNumber = va_arg(vlist, f64);
                lua_pushnumber(mLuaState, nNumber);
                param_cnt++;
                break;
            }
            case 'd':  // 输入的数据为整形
            {
                nInteger = va_arg(vlist, s32);
                lua_pushinteger(mLuaState, nInteger);
                param_cnt++;
                break;
            }
            case 's':  // 字符串型
            {
                cString = va_arg(vlist, s8*);
                lua_pushstring(mLuaState, cString);
                param_cnt++;
                break;
            }
            case 'N':  // nullptr
            {
                lua_pushnil(mLuaState);
                param_cnt++;
                break;
            }
            case 'F':  // 输入的是CFun形，即内部函数形
            {
                CFunc = va_arg(vlist, lua_CFunction);
                lua_pushcfunction(mLuaState, CFunc);
                param_cnt++;
                break;
            }
            case 'v':  // 输入的是堆栈中Index为nIndex的数据类型
            {
                nNumber = va_arg(vlist, s32);
                s32 nIndex1 = (s32)nNumber;
                lua_pushvalue(mLuaState, nIndex1);
                param_cnt++;
                break;
            }
            default: break;
            } // switch
        } // for
    } //if

    s32 ret = lua_pcall(mLuaState, param_cnt, nResults, 0);
    if (ret != 0) {
        Logger::logInfo("ScriptManager::callWith, [%s] %s\n", cFuncName, lua_tostring(mLuaState, -1));
        lua_pop(mLuaState, 1);  // pop lua err
        return false;
    }
    return	true;
}


bool ScriptManager::callFunc(const s8* cFuncName, s32 nResults, const s8* fmtstr, ...) {
    va_list vlist;
    va_start(vlist, fmtstr);
    bool ret = callWith(cFuncName, nResults, fmtstr, vlist);
    va_end(vlist);
    return ret;
}


bool ScriptManager::add(Script* script) {
    if (!script) {
        return false;
    }
    if (getScript(script->getName())) {
        return false;
    }
    script->grab();
    mAllScript.insert(script->getName(), script);
    return true;
}

Script* ScriptManager::createScript(const String& keyname) {
    String filenm(240);
    filenm = getScriptPath();
    filenm += keyname;

    Script* ret = new Script(keyname, filenm);
    bool success = ret->load(true);
    if (!success) {
        ret->drop();
        Logger::logError("ScriptManager::createScript,fail load script= %s", keyname.c_str());
        return nullptr;
    }
    if (!exec(*ret)) {
        ret->drop();
        Logger::logError("ScriptManager::createScript,exec script= %s", keyname.c_str());
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

Script* ScriptManager::getScript(const u32 id) {
    for (TMap<String, Script*>::Iterator git = mAllScript.getIterator(); !git.atEnd(); ++git) {
        if (git->getValue()->getID() == id) {
            return git->getValue();
        }
    }
    return nullptr;
}

Script* ScriptManager::getScript(const String& name) {
    TMap<String, Script*>::Node* nd = mAllScript.find(name);
    return nd ? nd->getValue() : nullptr;
}

void ScriptManager::removeAll() {
    for (TMap<String, Script*>::Iterator it = mAllScript.getIterator(); !it.atEnd(); ++it) {
        it->getValue()->drop();
    }
    mAllScript.clear();
}

bool ScriptManager::remove(Script* script) {
    if (!script) {
        return false;
    }
    return remove(script->getName());
}

bool ScriptManager::remove(const u32 id) {
    for (TMap<String, Script*>::Iterator git = mAllScript.getIterator(); !git.atEnd(); ++git) {
        if (git->getValue()->getID() == id) {
            git->getValue()->drop();
            mAllScript.remove(git->getKey());
            return true;
        }
    }
    return false;
}

bool ScriptManager::remove(const String& name) {
    TMap<String, Script*>::Node* nd = mAllScript.find(name);
    if (!nd) {
        return false;
    }
    nd->getValue()->drop();
    mAllScript.remove(nd);
    return true;
}

}//namespace script
}//namespace app
