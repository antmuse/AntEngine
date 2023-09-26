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


#include "Logger.h"
#include "Timer.h"
#include "FileWriter.h"
#include "Engine.h"


namespace app {


// log eg: 2022-12-01 20:50:39 [1,PID] Engine::init
const s32 G_PRE_LEN = 23; // pre len for PID position

s8 Logger::mText[MAX_TEXT_BUFFER_SIZE];
TVector<ILogReceiver*> Logger::mAllReceiver;
std::mutex Logger::mMutex;
s32 Logger::mPID = 0;
s32 Logger::mPID_len = 0;

#ifdef DDEBUG
ELogLevel Logger::mMinLevel = ELL_DEBUG;
#else  // for release version
ELogLevel Logger::mMinLevel = ELL_INFO;
#endif // release version

static const s8* G_LOG_FMT = "%Y-%m-%d %H:%M:%S";

class LogPrinter : public ILogReceiver {
public:
    LogPrinter() {
    }
    virtual ~LogPrinter() {
    }

    virtual void log(usz len, const s8* msg) {
#if defined(DOS_ANDROID)
        __android_log_print(ANDROID_LOG_DEBUG, "AntEngine", msg);
#else
        printf("%s", msg);
#endif
    }

    virtual void flush() {
    }
};

class LogFile : public ILogReceiver {
public:
    LogFile() : mFileSize(0), mTodayID(0), mFormat("/%Y-%m-%d.") {
        open();
    }

    LogFile(const s8* fmt) : mFileSize(0), mTodayID(0), mFormat(fmt ? fmt : "/%Y-%m-%d.") {
        open();
    }

    virtual ~LogFile() {
    }

    virtual void log(usz len, const s8* msg) {
        const s16 day = getDay(msg);
        if (day != mTodayID) { // split log by day
            // mFile.close();
            open();
        }
        mFileSize += mFile.write(msg, len);
    }

    virtual void flush() {
        mFile.flush();
    }

private:
    String mFormat;
    s16 mTodayID;
    FileWriter mFile;
    usz mFileSize;

    void open() {
        Engine& eng = Engine::getInstance();
        String flog(eng.getConfig().mLogPath);
        const usz fmtmax = 260;
        usz addsz = fmtmax + mFormat.getLen();
        flog.reserve(flog.getLen() + eng.getAppName().getLen() + addsz);
        usz len = flog.getLen();
        flog.setLen(len + Timer::getTimeStr((s8*)flog.c_str() + len, fmtmax, mFormat.c_str()));
        mTodayID = App2Char2S16(flog.c_str() + len + 9);
        flog += eng.getAppName();
        flog.deleteFilenameExtension();
        flog += ".log";
        mFile.openFile(flog, true);
        mFileSize = mFile.getFileSize();
    }
};



Logger::Logger() {
    memset(mText, 0, sizeof(mText));
    // AppStrConverterInit();
}


Logger::~Logger() {
    clear();
}


Logger& Logger::getInstance() {
    static Logger it;
    return it;
}

void Logger::setPID(s32 id) {
    // std::unique_lock<std::mutex> ak(mMutex);
    mPID = id;
    mPID_len = snprintf(mText + G_PRE_LEN, sizeof(mText) - G_PRE_LEN, "%d", id);
}

void Logger::flush() {
    mMutex.lock();

    usz max = mAllReceiver.size();
    for (usz i = 0; i < max; ++i) {
        mAllReceiver[i]->flush();
    }

    mMutex.unlock();
}


void Logger::log(ELogLevel iLevel, const s8* iMsg, ...) {
    if (iLevel >= mMinLevel) {
        va_list args;
        va_start(args, iMsg);
        postLog(iLevel, iMsg, args);
        va_end(args);
    }
}


void Logger::logCritical(const s8* msg, ...) {
    if (ELL_CRITICAL >= mMinLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELL_CRITICAL, msg, args);
        va_end(args);
    }
}


void Logger::logError(const s8* msg, ...) {
    if (ELL_ERROR >= mMinLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELL_ERROR, msg, args);
        va_end(args);
    }
}


void Logger::logWarning(const s8* msg, ...) {
    if (ELL_WARN >= mMinLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELL_WARN, msg, args);
        va_end(args);
    }
}


void Logger::logInfo(const s8* msg, ...) {
    if (ELL_INFO >= mMinLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELL_INFO, msg, args);
        va_end(args);
    }
}



void Logger::logDebug(const s8* msg, ...) {
    if (ELL_DEBUG >= mMinLevel) {
        va_list args;
        va_start(args, msg);
        postLog(ELL_DEBUG, msg, args);
        va_end(args);
    }
}


void Logger::setLogLevel(const ELogLevel logLevel) {
    mMinLevel = logLevel;
}


void Logger::postLog(const ELogLevel logLevel, const s8* msg, va_list args) {
    if (msg) {
        mMutex.lock();

        usz used = Timer::getTimeStr(mText, sizeof(mText), G_LOG_FMT);
        mText[used++] = ' ';
        mText[used++] = '[';
        mText[used++] = '0' + logLevel;
        mText[used++] = ',';
        used += mPID_len;
        mText[used++] = ']';
        mText[used++] = ' ';

        used += vsnprintf(mText + used, sizeof(mText) - used, msg, args);
        if (used > sizeof(mText) - 2) {
            used = sizeof(mText) - 2;
        }
        mText[used++] = '\n';
        mText[used] = 0;

        usz max = mAllReceiver.size();
        for (usz i = 0; i < max; ++i) {
            mAllReceiver[i]->log(used, mText);
        }

        mMutex.unlock();
    }
}

const ILogReceiver* Logger::addPrintReceiver() {
    mMutex.lock();
    static bool on = false;
    if (on) {
        mMutex.unlock();
        return nullptr;
    }
    on = true;
    mMutex.unlock();

    return add(new LogPrinter());
}

const ILogReceiver* Logger::addFileReceiver() {
    mMutex.lock();
    static bool on = false;
    if (on) {
        mMutex.unlock();
        return nullptr;
    }
    on = true;
    mMutex.unlock();

    return add(new LogFile());
}

const ILogReceiver* Logger::add(ILogReceiver* iLog) {
    if (iLog) {
        mMutex.lock();
        mAllReceiver.pushBack(iLog);
        mMutex.unlock();
    }
    return iLog;
}


void Logger::remove(const ILogReceiver* iLog) {
    if (iLog) {
        mMutex.lock();
        for (usz it = 0; it < mAllReceiver.size(); ++it) {
            if (iLog == mAllReceiver[it]) {
                delete mAllReceiver[it];
                mAllReceiver.quickErase(it);
                break;
            }
        }
        mMutex.unlock();
    }
}


void Logger::clear() {
    mMutex.lock();
    for (usz it = 0; it < mAllReceiver.size(); ++it) {
        delete mAllReceiver[it];
    }
    mAllReceiver.resize(0);
    mMutex.unlock();
}


} // namespace app
