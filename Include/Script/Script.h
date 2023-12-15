/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/

#ifndef APP_SCRIPT_H
#define	APP_SCRIPT_H

#include "Strings.h"

struct lua_State;

namespace app {
namespace script {

class Script {
public:
    Script();

    virtual ~Script();

    /**
     * @brief Loads a script file and builds it.
     * @param iName  Name of the script.
     * @param reload force to reload
     * @return true if successful loading, else false.
     */
    bool load(lua_State* vm, const String& iPath, const String& iName, bool reload, bool pop = true);

    bool loadBuf(lua_State* vm, const String& iName, const s8* buf, usz bsz, bool reload, bool pop = true);
    
    void unload(lua_State* vm);

    static bool isLoaded(lua_State* vm, const String& iName);

    static bool getLoaded(lua_State* vm, const String& iName);

    static bool compile(lua_State* vm, const String& iName, const s8* buf, usz bufsz, bool pop = false);

    static s32 resumeThread(lua_State* vm, s32 argc, s32& ret_cnt);

    bool exec(lua_State* vm, const s8* func = nullptr,
        s32 nargs = 0, s32 nresults = 0, s32 errfunc = 0);

    bool execFunc(lua_State* vm, const s8* cFuncName, s32 nResults = 0, const s8* fmtstr = nullptr, ...);

    bool execWith(lua_State* vm, const s8* cFuncName, s32 nResults, const s8* cFormat, va_list& vlist);

    const String& getName() const {
        return mName;
    }

    void setName(const s8* name) {
        mName = name;
    }

    void setName(const String& name) {
        mName = name;
    }

    static void setGlobalVal(lua_State* vm, const s8* key, const s8* val, usz vlen = 0);
    static void setGlobalVal(lua_State* vm, const s8* key, ssz val);
    static void setGlobalVal(lua_State* vm, const s8* key, f64 val);
    static void setGlobalVal(lua_State* vm, const s8* key, void* val);

    static void setUpVal(lua_State* vm, const s8* key, s32 funcIdx, s32 popUps);

    // Create new table and set _G field to itself.
    static void createGlobalTable(lua_State* vm, s32 narr = 0, s32 nrec = 0);

    static void createArray(lua_State* vm, const s8** arr, ssz cnt);

    static void popParam(lua_State* vm, s32 iSum);

    static void pushTable(lua_State* vm, const s8* key, const s8* val);
    static void pushTable(lua_State* vm, const s8* key, void* val);
    static void pushTable(lua_State* vm, const s8* key, usz len, void* val);
    static void pushTable(lua_State* vm, const s8* key, usz klen, const s8* val, usz vlen);
    static void pushTable(lua_State* vm, void* key, void* val);

    static void pushParam(lua_State* vm, void* val);
    static void pushParam(lua_State* vm, s32 val);
    static void pushParam(lua_State* vm, const s8* val);

private:
    String mName;
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPT_H

