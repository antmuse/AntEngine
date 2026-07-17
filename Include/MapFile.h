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


#pragma once

#include <atomic>
#include "TString.h"

namespace app {

class MapFile {
public:
    MapFile();

    virtual ~MapFile();

    void* createMem(usz iSize, const s8* iMapName, bool iReadOnly, bool share, bool not_anonymous);

    void* createMapfile(usz iSize, const s8* iFileName, bool iReadOnly, bool needDEL, bool share);

    void* openMem(const s8* iMapName, bool iReadOnly, bool not_anonymous);

    void closeAll();

    bool flush();

    bool isOpen()const {
        return nullptr != mMemory;
    }

    bool isCreator() const {
        return 0 != (EMF_CREATOR & mFlag);
    }

    s8* getMem()const {
        return (s8*)mMemory;
    }

    usz getMemSize()const {
        return mMemSize;
    }

    usz getFileSize()const;

    const s8* getFileName()const {
        return mFileName;
    }

    const s8* getMemName()const {
        return mMemName;
    }

protected:
#if defined(DOS_WINDOWS)
    void* mFile;
    void* mMapHandle;
#else
    s32 mFile;
    s32 createMemFD(const s8* iMapName, usz iSize);
    void fixMemName(s8* mname);
#endif
    void* mMemory;
    usz mMemSize;
    usz mFileSize;
    s32 mFlag;          //EMemFlag, 1=read,2=write,4=shared,8=creator
    s8 mFileName[260];
    s8 mMemName[64];

    enum EMemFlag {
        EMF_READ = 0x1,
        EMF_WRITE = 0x2,
        EMF_SHARE = 0x4,
        EMF_NEED_DEL = 0x8,       // del file when close
        EMF_NOT_ANONYMOUS = 0x10, // not anonymous for linux only
        EMF_CREATOR = 0x20
    };

private:
    bool createFile(usz iSize);
    /**
     * @return 0 if fail, 1 if created success, 2 if opened a exsist map
     */
    s32 createMap(const s8* iMapName, usz iSize);
    bool openMap(const s8* iMapName);

#if defined(DOS_WINDOWS)
    bool createView();
#endif
};


}//app