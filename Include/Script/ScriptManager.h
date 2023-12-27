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
#include "ThreadPool.h"
#include "Script/Script.h"

namespace app {
namespace script {

class LuaThread final {
public:
    lua_State* mSubVM;
    s32 mRef;
    s32 mParamCount;
    s32 mStatus;
    s32 mRetCount;
    FuncTask mCaller;
    void* mUserData;
    LuaThread();
    ~LuaThread();
    void operator()();
};

class ScriptManager : public Nocopy {
public:
    static ScriptManager& getInstance();

    lua_State* getRootVM() const;

    /**
    * @param fileName relative filename of the file to load.
    */
    bool include(const String& fileName);

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

    //! Gets the Script with the given name.
    //! @return Pointer to the Script if found, else NULL.
    Script* getScript(const String& name);

    void removeAll();

    //! Removes the given Script.
    //! @param script        Pointer to the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(Script* pScript);

    //! Removes the given Script with the given name.
    //! @param name          Name of the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(const String& name);

    const String& getScriptPath() const {
        return mScriptPath;
    }

    bool loadFirstScript();

    usz getMemory();
    s32 makeGC();

    lua_State* createThread();
    void deleteThread(lua_State*& vm);
    void getThread(lua_State* vm);

    static void setENV(lua_State* vm, bool pop_ctx = true, const s8* ctx_name = "VContext");

    static void resumeThread(LuaThread& co);

private:
    lua_State* mRootVM;
    TMap<String, Script*> mAllScript;
    String mScriptPath;

    ScriptManager();
    virtual ~ScriptManager();
    void initialize();
    void uninit();
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPTMANAGER_H
