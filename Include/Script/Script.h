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

#include "RefCount.h"
#include "Strings.h"

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

    /**
     * @brief Loads a script file and builds it.
     * @param reload force to reload
     * @return true if successful loading, else false.
     */
    bool load(bool reload);

    void unload();

    const String& getName() const {
        return mName;
    }

    void setName(const s8* name) {
        mName = name;
    }

    void setName(const String& name) {
        mName = name;
    }

    u32 getID() const {
        return mID;
    }

    const s8* getBuffer() const {
        return mBuffer;
    }

    usz getBufferSize() const {
        return mBufferSize;
    }

private:
    u32 mID;
    s8* mBuffer;
    usz mBufferSize;
    String mName;
    String mPath;
};


}//namespace script
}//namespace app
#endif	// APP_SCRIPT_H

