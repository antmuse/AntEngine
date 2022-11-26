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
#include "RefCount.h"

namespace app {
namespace script {

class Script : public RefCountAtomic {
public:
    /**
    *@param iName  Name of the script.
    *@param iPath   path of the script file.
    */
    Script(const String& iName, const String& iPath);

    virtual ~Script();

    //! Gets the name of this script.
    //const stringc& getName() const;

    //! Loads a script file and builds it.
    //! @param fileName      Filename of the file from where the script should be retrieved.
    //! @return True on successful loading, else false on failure.
    bool load(bool isNew);

    //! Unloads the loaded script.
    //! @return True on success, false on failure.
    bool unload();
    /**
    * execute main() function in per script file
    */
    bool execute();

    //! Executes the function with the given name and with the given arguments.
    //! @return True on success, false on failure.
    bool execute(const String& func);


    bool execute(const String& func, s8* fmtstr, ...);
    bool execute2(const s8* cFuncName, const s8* cFormat, va_list& param, s32 nResults);


    //! Returns the name of the node.
    /** \return Name as character string. */
    const s8* getName() const {
        return mName.c_str();
    }

    //const stringc& getName() const {
    //    return Name;
    //}

    //! Sets the name of the node.
    /** \param name New name of the scene node. */
    void setName(const s8* name) {
        mName = name;
    }


    //! Sets the name of the node.
    /** \param name New name of the scene node. */
    void setName(const String& name) {
        mName = name;
    }

    /**
    *@brief Get the id of the script node. This id can be used to identify the node.
    *@return The id.
    */
    s32 getID() const {
        return mID;
    }


private:
    s32 mID;            ///< ID of the script node.
    s8* pFileBuffer;    ///< The buffer of script file.
    usz fileSize;
    String mName;
    String mPath;
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPT_H

