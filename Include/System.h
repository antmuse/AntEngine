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


#ifndef APP_SYSTEM_H
#define APP_SYSTEM_H

#include "Strings.h"
#include "TVector.h"

namespace app {

class FileInfo {
public:
    s64 mSize;
    s64 mLastSaveTime;
    s8 mFlag;   //0=file, 1=path
    s8 mFileName[260];
};

class System {
public:

#if defined(DOS_WINDOWS)
    static s32 convNtstatus2NetError(s32 status);
#endif

    static s32 daemon();

    static s32 getSignal() {
        return gSignal;
    }

    /**
     * @return 程序所在磁盘扇区大小，字节*/
    static u32 getDiskSectorSize();

    static u32 getCoreCount();

    static u32 getPageSize();

    /**
    *@brief Load the socket lib, windows only, else useless.
    *@return 0 if successed, else failed.
    */
    static s32 loadNetLib();

    /**
    *@brief Unload the socket lib, windows only, else useless.
    *@return 0 if successed, else failed.
    */
    static s32 unloadNetLib();

    static s32 getNetError();

    static s32 getError();

    static void setError(s32 it);

    static s32 getAppError();
    static s32 getAppError(s32 err);

    static s32 getPID();

    /**
     * @brief create a path
     * @param val The path to create, \p val can include filename.
     *        eg: if val="/home/my/dat/filenm.log", then create path="/home/my/dat/"
     *            or just pass val="/home/my/dat/"
     */
    static s32 createPath(const String& val);

    static s32 removeFile(const String& it);

    //@return -1=不存在，0=file, 1=path
    static s32 isExist(const String& it);

    static void getPathNodes(const String& pth, usz pos, TVector<FileInfo>& out);


    static s32 createProcess(usz socket, void*& handle);
    static void waitProcess(void* handle);

    static String getWorkingPath();

private:
    System();
    ~System();
    System(const System&) = delete;
    System(const System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(const System&&) = delete;
    static s32 gSignal;

    static bool isPathDiv(tchar p) {
        return DSTR('/') == p || DSTR('\\') == p;
    }
};


}// end namespace app


#endif //APP_SYSTEM_H