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


#ifndef APP_LOGGER_H
#define	APP_LOGGER_H

#include <stdarg.h>
#include <mutex>
#include "TVector.h"


#if defined(DDEBUG) && defined(DOS_WINDOWS)
#define DLOG(TYPE, FORMAT, ...)  Logger::log(TYPE, FORMAT, __VA_ARGS__)
#else
#define DLOG(TYPE, FORMAT, ...)
#endif

namespace app {

//error code of app
enum EErrorCode {
    EE_OK = 0,
    EE_NO_OPEN = -1,         //未打开FD
    EE_NO_READABLE = -2,     //FD不可读
    EE_NO_WRITEABLE = -3,    //FD不可写
    EE_CLOSING = -4,         //FD已关闭
    EE_INVALID_HANDLE = -5,  //FD无效
    EE_INVALID_PARAM = -6,   //参数无效
    EE_RETRY = -7,
    EE_TIMEOUT = -8,
    EE_INTR = -10,           //中断
    EE_POSTED = -11,         //发起异步请求成功
    EE_TOO_MANY_FD = -24,    //打开过多句柄, ulimit -n
    EE_ERROR = -1
};


enum ELogLevel {
    ELL_DEBUG,
    ELL_INFO,
    ELL_WARN,
    ELL_ERROR,
    ELL_CRITICAL,
    ELL_COUNT
};

/// Contains strings for each log level to make them easier to print to a stream.
const s8* const AppLogLevelNames[] = {
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Critical",
    0
};




class ILogReceiver {
public:
    ILogReceiver() {    }

    virtual ~ILogReceiver() {    }

    virtual void log(usz len, const s8* msg) = 0;

    virtual void flush() = 0;
};



class Logger {
public:
    static Logger& getInstance();

    static void flush();

    static void log(ELogLevel iLevel, const s8* msg, ...);


    static void logCritical(const s8* msg, ...);


    static void logError(const s8* msg, ...);


    static void logWarning(const s8* msg, ...);


    static void logInfo(const s8* msg, ...);


    static void logDebug(const s8* msg, ...);


    static const ELogLevel getLogLevel() {
        return mMinLevel;
    }

    static void setLogLevel(const ELogLevel logLevel);

    /**
    * @brief Register a Log Receiver.
    * @param iLog: The log receiver to Regist. */
    static void add(ILogReceiver* iLog);

    static void addPrintReceiver();

    static void addFileReceiver(const s8* format = "Log/%Y-%m-%d_%H-%M-%S.");

    /**
    * @brief Unregister a Log Receiver.
    * @param iLog: The log receiver to remove. */
    static void remove(const ILogReceiver* iLog);

    /**
    * @brief remove all log receivers.
    */
    static void clear();


protected:
    static void postLog(const s8* msg, va_list args);
    const static u16 MAX_TEXT_BUFFER_SIZE = 4096;
    static s8 mText[MAX_TEXT_BUFFER_SIZE];
    static ELogLevel mMinLevel;
    static std::mutex mMutex;
    static TVector<ILogReceiver*> mAllReceiver;

private:
    Logger();
    ~Logger();
    Logger(Logger& it) = delete;
    Logger(Logger&& it) = delete;
    Logger& operator=(Logger& it) = delete;
    Logger& operator=(Logger&& it) = delete;
};


} //namespace app

#endif	/* APP_LOGGER_H */

