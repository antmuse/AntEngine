#include "Script/Script.h"
#include "Script/HLua.h"
#include "Logger.h"
#include "Engine.h"
#include "FileReader.h"
#include "Script/ScriptManager.h"

namespace app {
namespace script {

Script::Script() {
}

Script::~Script() {
}

bool Script::isLoaded(lua_State* vm, const String& iName) {
    lua_getglobal(vm, iName.c_str());
    bool ret = lua_isnil(vm, -1) ? false : true;
    lua_pop(vm, 1);
    return ret;
}

bool Script::getLoaded(lua_State* vm, const String& iName) {
    lua_getglobal(vm, iName.c_str());
    return lua_isnil(vm, -1) ? false : true;
}

bool Script::loadBuf(lua_State* vm, const String& iName, const s8* buf, usz bsz, bool reload, bool pop) {
    if (!reload) {
        if (getLoaded(vm, iName)) {
            if (pop) {
                lua_pop(vm, 1);
            }
            return true;
        }
    }
    bool ret = compile(vm, iName, buf, bsz, pop);
    mName = iName;
    return ret;
}

bool Script::load(lua_State* vm, const String& iPath, const String& iName, bool reload, bool pop) {
    if (!reload) {
        if (getLoaded(vm, iName)) {
            if (pop) {
                lua_pop(vm, 1);
            }
            return true;
        }
    }
    FileReader file;
    if (!file.openFile(iPath + iName)) {
        Logger::logError("Script::load, open fail, script = %s", iName.c_str());
        return false;
    }
    usz bufsz = file.getFileSize();
    s8* buf = new s8[bufsz + 1];
    if (bufsz != file.read(buf, bufsz)) {
        Logger::logError("Script::load, read fail, script = %s", iName.c_str());
        //file.close();
        return false;
    }
    buf[bufsz] = '\0';
    bool ret = compile(vm, iName, buf, bufsz, pop);
    mName = iName;
    delete[] buf;
    return ret;
}

void Script::unload(lua_State* vm) {
    if (vm) {
        lua_pushnil(vm);
        lua_setglobal(vm, mName.c_str());
    }
}

bool Script::compile(lua_State* vm, const String& iName, const s8* buf, usz bufsz, bool pop) {
    if (!vm || !buf || 0 == bufsz || 0 == iName.getLen()) {
        return false;
    }
    u32 cnt = lua_gettop(vm);
    if (0 != luaL_loadbuffer(vm, buf, bufsz, iName.c_str())) {
        Logger::logError("Script::compile, load err = %s, name=%s\n",
            lua_tostring(vm, -1), iName.c_str());
        lua_pop(vm, 1);  // pop lua err
        return false;
    }
    cnt = lua_gettop(vm);
    if (!pop) {
        lua_pushvalue(vm, -1);
    }
    lua_setglobal(vm, iName.c_str());  //set and pop 1
    cnt = lua_gettop(vm);
    Logger::logInfo("Script::compile, success script = %s", iName.c_str());
    return true;
}

bool Script::exec(lua_State* vm, const s8* func,
    s32 nargs, s32 nresults, s32 errfunc) {
    const s32 cnt = lua_gettop(vm);
    if (!getLoaded(vm, mName)) {
        return false;
    }

    if (func) {
        lua_getglobal(vm, func);
    }

    /**
     * lua_pcall参数:
     * 传入nargs个参数,期望得到nresults个返回值,errfunc表示错误处理函数在栈中的索引值,
     * 压入结果前会弹出函数和参数*/
    s32 expr = lua_pcall(vm, nargs, nresults, errfunc);
    if (expr) {
        Logger::logError("Script::exec, call err = %s, name=%s\n",
            lua_tostring(vm, -1), mName.c_str());
        lua_settop(vm, cnt);  // pop lua err
        return false;
    }
    lua_settop(vm, cnt);
    return true;
}


bool Script::execWith(lua_State* vm, const s8* funcName, s32 nResults, const s8* cFormat, va_list& vlist) {
    const s32 cnt = lua_gettop(vm);
    if (!getLoaded(vm, mName)) {
        return false;
    }
    f64	nNumber = 0;
    s32 nInteger = 0;
    s8* cString = nullptr;
    void* pPoint = nullptr;

    s32 param_cnt = 0;
    lua_CFunction CFunc = nullptr;

    if (funcName) { //在堆栈中加入需要调用的函数名
        lua_getglobal(vm, funcName);
        if (!lua_isfunction(vm, -1)) {
            Logger::logInfo("ScriptManager::execWith, top=%d-%d, func=%s", cnt, lua_gettop(vm), funcName);
            lua_settop(vm, cnt);  // pop codeblock and err
            return false;
        }
    }

    if (cFormat) {
        for (s32 i = 0; '\0' != cFormat[i]; ++i) {
            switch (cFormat[i]) {
            case 'f':  // 输入的数据是Double型
            {
                nNumber = va_arg(vlist, f64);
                lua_pushnumber(vm, nNumber);
                param_cnt++;
                break;
            }
            case 'd':  // 输入的数据为整形
            {
                nInteger = va_arg(vlist, s32);
                lua_pushinteger(vm, nInteger);
                param_cnt++;
                break;
            }
            case 's':  // 字符串型
            {
                cString = va_arg(vlist, s8*);
                lua_pushstring(vm, cString);
                param_cnt++;
                break;
            }
            case 'N':  // nullptr
            {
                lua_pushnil(vm);
                param_cnt++;
                break;
            }
            case 'F':  // 输入的是CFun形，即内部函数形
            {
                CFunc = va_arg(vlist, lua_CFunction);
                lua_pushcfunction(vm, CFunc);
                param_cnt++;
                break;
            }
            case 'v':  // 输入的是堆栈中Index为nIndex的数据类型
            {
                nNumber = va_arg(vlist, s32);
                s32 nIndex1 = (s32)nNumber;
                lua_pushvalue(vm, nIndex1);
                param_cnt++;
                break;
            }
            default: break;
            } // switch
        } // for
    } //if

    s32 ret = lua_pcall(vm, param_cnt, nResults, 0);
    if (ret != 0) {
        Logger::logInfo("ScriptManager::execWith, top=%d, [%s] %s\n", lua_gettop(vm), funcName, lua_tostring(vm, -1));
        lua_settop(vm, cnt);  // pop lua err
        return false;
    }
    lua_settop(vm, cnt);
    return	true;
}


bool Script::execFunc(lua_State* vm, const s8* funcName, s32 nResults, const s8* fmtstr, ...) {
    va_list vlist;
    va_start(vlist, fmtstr);
    bool ret = execWith(vm, funcName, nResults, fmtstr, vlist);
    va_end(vlist);
    return ret;
}


}//namespace script
}//namespace app
