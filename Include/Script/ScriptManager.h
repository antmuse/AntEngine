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
#include "TList.h"
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
    *@brief execute main() function in current script file
    */
    bool execute(const String& iName, const s8* const pBuffet, usz fSize);

    /**
    *@brief Executes the function with the given name and given arguments.
    *@return True on success, false on failure.
    */
    bool execute(const String& iName, const s8* const pBuffet, usz fSize, const s8* const pFunc);

    //! Adds the given Script to the ScriptManager
    //! @param script        Pointer to the Script to add.
    //! @return True if addition was successful, false if addition was a failure.
    bool add(Script* pScript);

    /**
    * Creates an script with the given name, loaded from the given file.
    * @param fileName      Filename of the file to load.
    * @param grab          Should the ScriptManager add the loaded script to the internal list?
    * @return Pointer to the Script on success, NULL on failure.
    */
    Script* createScript(const String& fileName, bool grab = false);

    //! Gets the Script with the given ID.
    //! @return Pointer to the Script if found, else NULL.
    Script* getScript(const s32 id);

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
    bool remove(const s32 id);

    //! Removes the given Script with the given name.
    //! @param name          Name of the Script to remove.
    //! @return True if removal was successful, false if removal was a failure.
    bool remove(const String& name);

private:
    s32 mParamCount;
    lua_State* mLuaState;
    TList<Script*> mAllScript;

    ScriptManager();
    virtual ~ScriptManager();
    void initialize();
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPTMANAGER_H
