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


#ifndef APP_HANDLEFILE_H
#define	APP_HANDLEFILE_H

#include "Handle.h"
#include "Loop.h"


namespace app {

#if defined(DOS_WINDOWS)
using FD = void*;
#else //defined(DOS_LINUX) || defined(DOS_ANDROID)
using FD = s32;
#endif



/**
* @class HandleFile
* @brief asynchronous I/O for file
*/
class HandleFile : public HandleTime {
public:
    HandleFile();

    ~HandleFile();

    FD getHandle()const {
        return mFile;
    }

    const String& getFileName()const {
        return mFilename;
    }

    usz getFileSize()const {
        return mFileSize;
    }

    /**
    * @param offset д����ʼ��
    */
    s32 write(RequestFD* it, usz offset = 0);

    /**
    * @param offset ��ȡ��ʼ��
    */
    s32 read(RequestFD* it, usz offset = 0);

    s32 close();

    /**
    * @brief open file
    * @param fname file name
    * @param flag 0=not share, 1=share read, 2=share write, 4=create if not exists
    * @return 0 if success, else failed.
    */
    s32 open(const String& fname, s32 flag = 1);

    //�ضϻ���չ�ļ�
    bool setFileSize(usz fsz);

protected:
    friend class app::Loop;
    FD mFile;
    usz mFileSize;
    String mFilename;

#if defined(DOS_LINUX) || defined(DOS_ANDROID)
#if !defined(DUSE_IO_URING)
    void stepByPool(RequestFD* it);
    void stepByLoop(RequestFD* it);
#endif
#endif
};


} //namespace app

#endif //APP_HANDLEFILE_H
