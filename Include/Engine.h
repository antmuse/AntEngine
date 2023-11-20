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

#include <atomic>
#include "Strings.h"
#include "TVector.h"
#include "EngineConfig.h"
#include "Loop.h"
#include "Logger.h"
#include "MapFile.h"
#include "MemSlabPool.h"
#include "Net/TlsContext.h"
#include "Script/ScriptManager.h"

namespace app {

enum ProcessStatus {
    EPS_INIT = 0,
    EPS_RUNNING = 1,
    EPS_EXITING = 2,
    EPS_EXITED = 4,
    EPS_RESPAWN = 8,

    EPS_LAST
};

enum ECommandType {
    ECT_RESP_BIT = 0x8000,

    ECT_EXIT = 1,
    ECT_EXIT_RESP = ECT_RESP_BIT | ECT_EXIT,

    ECT_ACTIVE = 2,
    ECT_ACTIVE_RESP = ECT_RESP_BIT | ECT_ACTIVE,

    ECT_TASK = 3,
    ECT_RESPAWN = 4,

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

//ping-pong
struct CommandActive : public MsgHeader {
    void pack() {
        finish(ECT_ACTIVE, ++gSharedSN, ECT_VERSION);
    }
    void packResp(u32 sn) {
        finish(ECT_ACTIVE_RESP, sn, ECT_VERSION);
    }
};

struct CommandTask : public MsgHeader {
    FuncTask mCall;
    const void* mThis;
    void* mData;


    template<class P>
    void pack(void(*func)(P*), P* dat) {
        init(ECT_TASK, sizeof(*this), 0, ECT_VERSION);
        mCall = func;
        mThis = nullptr;
        mData = dat;
    }

    template<class T, class P>
    void pack(void(T::* func)(P*), const void* it, P* dat) {
        init(ECT_TASK, sizeof(*this), 0, ECT_VERSION);
        void* fff = reinterpret_cast<void*>(&func);
        mCall = *(FuncTask*)fff;
        mThis = it;
        mData = dat;
    }
    
    void operator()() {
        if (mThis) {
            ((FuncTaskClass)mCall)(mThis, mData);
        } else {
            mCall(mData);
        }
    }
};


struct Process {
    s32 mID;
    s32 mStatus;
    void* mHandle;
    bool mAlive;
    net::Socket mSocket; //write socket of pair
};


struct EngineStats {
    std::atomic<ssz> mTotalHandles;
    std::atomic<ssz> mClosedHandles;
    std::atomic<ssz> mFlyRequests;
    std::atomic<ssz> mInBytes;
    std::atomic<ssz> mOutBytes;
    std::atomic<ssz> mInPackets;
    std::atomic<ssz> mOutPackets;
    std::atomic<ssz> mHeartbeat;
    std::atomic<ssz> mHeartbeatResp;
    void clear() {
        memset(this, 0, sizeof(*this));
    }
};

struct EngineData {
    EngineStats mStats;
    Process* mAllProcess;
    s32 mProcessCount;
};

class Engine {
public:
    static Engine& getInstance() {
        static Engine it;
        return it;
    }
    bool init(const s8* fname, bool child = false);
    void run();
    bool step();
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

    // @return the default TLS context
    net::TlsContext& getTlsContext() {
        return mTlsENG;
    }

    void clear();

    void postCommand(s32 val);

    TVector<Process>& getChilds() {
        return mChild;
    }

    usz getChildCount()const {
        return mChild.size();
    }

    script::ScriptManager& getScriptManager() {
        return script::ScriptManager::getInstance();
    }

    const EngineConfig& getConfig()const {
        return mConfig;
    }

    s32 getPID()const{
        return mPID;
    }

    bool isMainProcess() const {
        return mMain;
    }

    /** @return Engine's statistics datas in shared mem. @see mMapfile. */
    EngineStats& getEngineStats() {
        return reinterpret_cast<EngineData*>(mMapfile.getMem())->mStats;
    }

    MemSlabPool& getMemSlabPool() {
        return *reinterpret_cast<MemSlabPool*>(mMapfile.getMem() + sizeof(EngineData));
    }

    usz getMemSlabPoolSize() const {
        return mConfig.mMemSize - sizeof(EngineData);
    }
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
    std::atomic<s32> mProcResponCount;
    s32 mProcStatus;
    bool mMain;
    EngineConfig mConfig;
    net::TlsContext mTlsENG;
    TVector<Process> mChild;

    bool createProcess();
    bool createProcess(usz idx);
    bool runMainProcess();
    bool runChildProcess(net::Socket& readSock, net::Socket& writeSock);

    void initPath(const s8* fname);
    void initTask();
};


} //namespace app

#endif //APP_ENGINE_H
