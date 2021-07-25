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


#ifndef APP_CFILEWRITER_H
#define	APP_CFILEWRITER_H

#include <stdarg.h>
#include "Strings.h"

namespace app {


class FileWriter {
public:
    FileWriter();

    ~FileWriter();

    void close();

    // @return if file is open
    DFINLINE bool isOpen() const {
        return nullptr != mFile;
    }

    bool flush();

    /** @return how much was wrote*/
    u64 write(const void* buffer, u64 sizeToWrite);

    u64 writeParams(const s8* format, ...);
    u64 writeParams(const s8* format, va_list& param);
    u64 writeWParams(const wchar_t* format, ...);
    u64 writeWParams(const wchar_t* format, va_list& param);


    //! changes position in file, returns true if successful
    //! if relativeMovement==true, the pos is changed relative to current pos,
    //! otherwise from begin of file
    bool seek(s64 finalPos, bool relativeMovement);


    s64 getPos() const;


    bool openFile(const String& fileName, bool append);


    const String& getFileName() const {
        return mFilename;
    }

    s64 getFileSize()const {
        return mFileSize;
    }

protected:
    String mFilename;
    FILE* mFile;
    s64 mFileSize;

private:
    FileWriter(const FileWriter&) = delete;
    FileWriter(const FileWriter&&) = delete;
    FileWriter& operator=(const FileWriter&) = delete;
    FileWriter& operator=(const FileWriter&&) = delete;
};


} // end namespace app

#endif //APP_CFILEWRITER_H

