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

#ifndef APP_SCRIPTMANAGER_H
#define	APP_SCRIPTMANAGER_H

#include "Nocopy.h"
#include "TMap.h"
#include "Script/Script.h"

struct lua_State;

namespace app {
namespace script {

class ScriptManager : public Nocopy {
public:
    static ScriptManager& getInstance();

    lua_State* getEngine() const;

    void pushParam(s32 prm);
    void pushParam(s8* prm);
    void pushParam(void* pNode);

    /**
    *@brief pop state
    *@param iSum pop iSum parameters out
    */
    void popParam(s32 iSum);

    /**
    * @param fileName relative filename of the file to load.
    */
    bool include(const String& fileName);

    bool exec(const Script& it, const s8* func = nullptr,
        s32 nargs = 0, s32 nresults = 0, s32 errfunc = 0);

    /**
    *@brief exec script buffer with name
    *@param func call function if not null, else run the script buf. 
    */
    bool exec(const String& iName, const s8* const pBuffet, usz fSize,
        const s8* func = nullptr, s32 nargs = 0, s32 nresults = 0, s32 errfunc = 0);

    bool callFunc(const s8* cFuncName, s32 nResults, const s8* fmtstr = nullptr, ...);

    bool callWith(const s8* cFuncName, s32 nResults, const s8* cFormat, va_list& vlist);

    //! Adds the given Script to the ScriptManager
    //! @param script        Pointer to the Script to add.
    //! @return True if addition was successful, false if addition was a failure.
    bool add(Script* pScript);

    /**
    * Create an script with the given file.
    * @param fileName relative filename of the file to load.
    * @return Pointer to the Script on success and must been drop(), else nullptr.
    */
    Script* createScript(const String& fileName);

    /**
    * Create an script with the given file, and add to manager if success.
    * @param fileName relative filename of the file to load.
    * @return Pointer to the Script on success and must not been drop(), else nullptr.
    */
    Script* loadScript(const String& fileName);

    //! Gets the Script with the given ID.
    //! @return Pointer to the Script if found, else NULL.
    Script* getScript(const u32 id);

    //! Gets the Script with the given name.
    //! @return Pointer to the Script if found, else NULL.
    Script* getScript(const String& name);

    void removeAll();

    //! Removes the given Script.
    //! @param script        Pointer to the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(Script* pScript);

    //! Removes the given Script with the given ID.
    //! @param id            ID of the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(const u32 id);

    //! Removes the given Script with the given name.
    //! @param name          Name of the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(const String& name);

    const String& getScriptPath() const {
        return mScriptPath;
    }

    bool loadFirstScript();

    // gc
    usz getMemory();

private:
    s32 mParamCount;
    lua_State* mLuaState;
    TMap<String, Script*> mAllScript;
    String mScriptPath;

    ScriptManager();
    virtual ~ScriptManager();
    void initialize();
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPTMANAGER_H
