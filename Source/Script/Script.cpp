#include "Script/Script.h"
#include <stdarg.h>
#include <atomic>
#include "Script/HLua.h"
#include "Logger.h"
#include "Engine.h"
#include "FileReader.h"
#include "Script/ScriptManager.h"

namespace app {
namespace script {

Script::Script(const String& iName, const String& iPath) :
    mName(iName), mPath(iPath) {
    static std::atomic<s32> mIDCount(0);
    mID = mIDCount++;
    pFileBuffer = 0;
    fileSize = 0;
#ifdef DDEBUG
    Logger::logInfo("Script::Script,name = %s\n", mName.c_str());
#endif
}

Script::~Script() {
    unload();
#ifdef DDEBUG
    Logger::logInfo("Script::~Script,name = %s\n", mName.c_str());
#endif
}

bool Script::load(bool isNew) {
    if (isNew) {
        unload();
    } else {
        return true;
    }
    // Open the given file and load the contents to the buffer.
    FileReader file;
    if (file.openFile(mPath)) {
        Logger::logInfo("Script::load, script = %s", mPath.c_str());
        fileSize = file.getFileSize();
        pFileBuffer = new s8[fileSize + 1];
        memset(pFileBuffer, 0, fileSize + 1);
        file.read(pFileBuffer, fileSize);
        file.close();
    }
    return true;
}

bool Script::unload() {
    if (pFileBuffer) {
        delete[] pFileBuffer;
        fileSize = 0;
        pFileBuffer = 0;
    }
    return true;
}

bool Script::execute() {
    return ScriptManager::getInstance().execute(mName, pFileBuffer, fileSize);
}

bool Script::execute(const String& func) {
    return ScriptManager::getInstance().execute(mName, pFileBuffer, fileSize, func.c_str());
}


bool Script::execute(const String& func, s8* fmtstr, ...) {
    bool ret = false;
    s32 result = 0;
    va_list vlist;
    va_start(vlist, fmtstr);
    ret = execute2(func.c_str(), fmtstr, vlist, 0);
    va_end(vlist);
    return ret;
}


bool Script::execute2(const s8* cFuncName, const s8* cFormat, va_list& vlist, s32 nResults) {
    lua_State* mState = 0; //TODO>>
    if (!mState)
        return false;


    double	nNumber = 0;
    int		nInteger = 0;
    char* cString = NULL;
    void* pPoint = NULL;
    int		i = 0;
    int		nArgnum = 0;
    lua_CFunction CFunc = NULL;

    lua_getglobal(mState, cFuncName); //在堆栈中加入需要调用的函数名

    while (cFormat && cFormat[i] != '\0') {
        switch (cFormat[i]) {
        case 'n'://输入的数据是double形 NUMBER，Lua来说是Double型
        {
            nNumber = va_arg(vlist, double);
            lua_pushnumber(mState, nNumber);
            nArgnum++;
        }
        break;

        case 'd'://输入的数据为整形
        {
            nInteger = va_arg(vlist, int);
            lua_pushinteger(mState, nInteger);
            nArgnum++;
        }
        break;

        case 's'://字符串型
        {
            cString = va_arg(vlist, char*);
            lua_pushstring(mState, cString);
            nArgnum++;
        }
        break;

        case 'N'://NULL
        {
            lua_pushnil(mState);
            nArgnum++;
        }
        break;

        case 'f'://输入的是CFun形，即内部函数形
        {
            CFunc = va_arg(vlist, lua_CFunction);
            lua_pushcfunction(mState, CFunc);
            nArgnum++;
        }
        break;

        case 'v'://输入的是堆栈中Index为nIndex的数据类型
        {
            nNumber = va_arg(vlist, int);
            int nIndex1 = (int)nNumber;
            lua_pushvalue(mState, nIndex1);
            nArgnum++;
        }
        break;

        }

        i++;
    }

    int nRetcode = lua_pcall(mState, nArgnum, nResults, 0);

    if (nRetcode != 0) {
        printf("LUA_CALL_FUNC_ERROR [%s] %s\n", cFuncName, lua_tostring(mState, -1));
        lua_pop(mState, 1);
        return false;
    }

    return	true;
}

}//namespace script
}//namespace app
