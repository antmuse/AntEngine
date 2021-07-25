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


#ifndef APP_ENGINE_H
#define	APP_ENGINE_H

#include "Strings.h"
#include "TVector.h"
#include "EngineConfig.h"
#include "Loop.h"
#include "Logger.h"
#include "MsgHeader.h"
#include "MapFile.h"
#include "ThreadPool.h"
#include "Net/TlsContext.h"

namespace app {

enum ECommandType {
    ECT_RESP_BIT = 0x8000,

    ECT_EXIT = 1,
    ECT_EXIT_RESP = ECT_RESP_BIT | ECT_EXIT,

    ECT_VERSION = 0xF001
};
struct CommandExit : public MsgHeader {
    void pack() {
        finish(ECT_EXIT, ++gSharedSN, ECT_VERSION);
    }
    void packResp(u32 sn) {
        finish(ECT_EXIT_RESP, sn, ECT_VERSION);
    }
};

class Engine {
public:
    static Engine& getInstance() {
        static Engine it;
        return it;
    }

    bool init(const s8* fname);

    bool uninit();

    const String& getAppPath()const {
        return mAppPath;
    }

    const String& getAppName()const {
        return mAppName;
    }

    ThreadPool& getThreadPool() {
        return mThreadPool;
    }

    Loop& getLoop() {
        return mLoop;
    }

    net::TlsContext& getTlsContext() {
        return mTlsENG;
    }

    void clear();

    void postCommand(s32 val);

protected:
    Engine();
    ~Engine();

private:
    Engine(const Engine&) = delete;
    Engine(const Engine&&) = delete;
    const Engine& operator=(const Engine&) = delete;
    const Engine& operator=(const Engine&&) = delete;
    String mAppPath;
    String mAppName;
    Loop mLoop;
    MapFile mMapfile;
    ThreadPool mThreadPool;
    s32 mPPID;
    s32 mPID;
    bool mMain; //Ö÷½ø³Ì
    EngineConfig mConfig;
    net::TlsContext mTlsENG;
};


} //namespace app

#endif //APP_ENGINE_H
