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


#include "Engine.h"
#include "System.h"
#include "Timer.h"
#include "FileWriter.h"
#include "Net/TcpProxy.h"
#include "Net/Acceptor.h"
#include "Net/HTTP/Website.h"


namespace app {

#if defined(DOS_WINDOWS)
const s8* G_CFGFILE = "Config/config_win.json";
#else
const s8* G_CFGFILE = "Config/config.json";
#endif
u32 MsgHeader::gSharedSN = 0;

Engine::Engine() : mPPID(0), mPID(0), mChild(32), mProcStatus(EPS_INIT), mProcResponCount(0), mMain(true) {
}

Engine::~Engine() {
}

void Engine::clear() {
    // Logger::clear();
    System::removeFile(mConfig.mPidFile.c_str());
}


void Engine::postCommand(s32 val) {
    MsgHeader cmd;
    switch (val) {
    case ECT_EXIT: {
        cmd.finish(ECT_EXIT, ++cmd.gSharedSN, ECT_VERSION);
        break;
    }
    case ECT_ACTIVE: {
        cmd.finish(ECT_ACTIVE, ++cmd.gSharedSN, ECT_VERSION);
        break;
    }
    case ECT_RESPAWN: {
        if (mMain && mConfig.mMaxProcess > 0) {
            ++mProcResponCount;
        }
        return;
    }
    default:
        Logger::log(ELL_INFO, "Engine::postCommand>>invalid cmd = %d", val);
        return;
    }
    if (mMain) {
        for (usz i = 0; i < mChild.size(); ++i) {
            /** TODO: keep send is thread/process safe
             *  @see Loop::postTask
             */

            if (sizeof(cmd) != mChild[i].mSocket.sendAll(&cmd, sizeof(cmd))) { // block send
                Logger::log(ELL_INFO, "Engine::postCommand>>fail post cmd = %d, pid=%d", val, mChild[i].mID);
            }
        }
    }
    if (ECT_EXIT == val) {
        mProcStatus = EPS_EXITING;
        if (!mMain || (mMain && 0 == mConfig.mMaxProcess)) {
            Logger::log(ELL_INFO, "Engine::postCommand>> %s process exit...", mMain ? "main" : "child");
            mLoop.postTask(cmd);
        }
    }
}

void Engine::initPath(const s8* fname) {
    mAppPath.reserve(260);
#if defined(DOS_WINDOWS)
    usz len = AppGbkToUTF8(fname, const_cast<s8*>(mAppPath.c_str()), mAppPath.getAllocated());
    mAppPath.setLen(len);
    mAppPath.deleteFilename();
    mAppPath.replace('\\', '/');
    mAppName = fname + mAppPath.getLen();
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    if ('/' == fname[0]) {
        mAppPath = fname;
        mAppPath.deleteFilename();
        mAppPath.replace('\\', '/');
        mAppName = fname + mAppPath.getLen();
    } else {
        mAppPath = System::getWorkingPath(); // pwd
        mAppName = fname;
        mAppName.deletePathFromFilename();
    }
#endif
}

bool Engine::init(const s8* fname, bool child) {
    if (!fname || 0 == fname[0]) {
        return false;
    }
    printf("fnm = %s\n", fname);
    mMain = !child;
    AppStrConverterInit();
    initPath(fname);
    if (!mConfig.load(mAppPath, G_CFGFILE, mMain)) {
        mConfig.save(mAppPath + G_CFGFILE + ".gen.json");
        return false;
    }
    if (mMain && mConfig.mDaemon) {
        if (EE_OK != System::daemon()) {
            return false;
        }
    }
    mPID = System::getPID();
    Logger::getInstance().setPID(mPID);
    MsgHeader::gSharedSN = 0;
    Timer::getTimeZone();
    System::getCoreCount();
    System::getPageSize();
    System::getDiskSectorSize();
    System::createPath(mConfig.mLogPath);
    const ILogReceiver* loger = nullptr;
    if ((mMain && 1 == mConfig.mPrint) || 2 == mConfig.mPrint) {
        loger = Logger::addPrintReceiver();
    }
    Logger::addFileReceiver();

    script::ScriptManager::getInstance();

    bool ret = 0 == System::loadNetLib();
    if (!ret) {
        Logger::log(ELL_ERROR, "Engine::init>>loadNetLib fail");
        return false;
    }

    net::AppInitTlsLib();
    mTlsENG.init();

    if (App4Char2S32("GMEM") == App4Char2S32(mConfig.mMemName.c_str())) {
        if (mMain) {
            if (!mMapfile.createMem(mConfig.mMemSize, mConfig.mMemName.c_str(), false, true)) {
                Logger::log(ELL_INFO, "Engine::init>>createMem fail = %s", mConfig.mMemName.c_str());
                return false;
            }
        } else {
            if (!mMapfile.openMem(mConfig.mMemName.c_str(), false)) {
                Logger::log(ELL_INFO, "Engine::init>>child openMem GMEM fail = %s", mConfig.mMemName.c_str());
                return false;
            }
        }
    } else {
        if (mMain) {
            if (!mMapfile.createMapfile(mConfig.mMemSize, mConfig.mMemName.c_str(), false, false, true)) {
                Logger::log(ELL_INFO, "Engine::init>>createMapfile fail = %s", mConfig.mMemName.c_str());
                return false;
            }
        } else {
            if (!mMapfile.openMem(mConfig.mMemName.c_str(), false)) {
                Logger::log(ELL_INFO, "Engine::init>>child openMem GMAP fail = %s", mConfig.mMemName.c_str());
                return false;
            }
        }
    }

    if (mMain) {
        MemSlabPool& mpool = getMemSlabPool();
        new (&mpool) MemSlabPool(getMemSlabPoolSize()); // mpool.initSlabSize();
        // mpool.mLock.tryUnlock();  TODO clear lock when ...
        getEngineStats().clear();

        System::removeFile(mConfig.mPidFile.c_str());
        FileWriter file;
        if (file.openFile(mConfig.mPidFile, false)) {
            file.writeParams("%d", mPID);
            file.close();
        } else {
            return false;
        }
        Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');
        ret = createProcess();
    }

    if (mMain) { // check again after fork()
        if (0 == mConfig.mPrint) {
            Logger::remove(loger);
        }
        ret = runMainProcess();
    } else {
        if (2 != mConfig.mPrint) {
            Logger::remove(loger);
        }
#if defined(DOS_WINDOWS)
        net::Socket cmdsock = (net::netsocket)GetStdHandle(STD_INPUT_HANDLE);
        Logger::log(ELL_INFO, "Engine::init>>pid = %d, cmdsock = %llu", mPID, cmdsock.getValue());
        if (!cmdsock.isOpen()) {
            Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');
            return false;
        }
        net::Socket tmp;
        Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');
        ret = runChildProcess(cmdsock, tmp);
#endif
    }

    if (ret) {
        script::ScriptManager::getInstance().loadFirstScript();
    }
    return ret;
}

bool Engine::uninit() {
    if (mMain) {
        MemSlabPool& mpool = getMemSlabPool();
        EngineStats& engStats = getEngineStats();

        Logger::log(ELL_INFO, "Engine::uninit>>share stats[total=%ld, closed=%ld, in=%lu, out=%lu]",
            engStats.mTotalHandles.load(), engStats.mClosedHandles.load(), engStats.mInBytes.load(),
            engStats.mOutBytes.load());

        u32 cnt = mpool.getStateCount();
        for (u32 i = 0; i < cnt; i++) {
            MemStat& mstat = *(mpool.mStats + i);
            Logger::log(ELL_INFO, "Engine::uninit>>share mem[%u][used/total=%lu/%lu, req=%lu, fail=%lu]", i,
                mstat.mUsed, mstat.mTotal, mstat.mRequests, mstat.mFails);
        }
    }
    Logger::log(ELL_INFO, "Engine::uninit>>pid = %d, main = %c, script=%llu", mPID, mMain ? 'Y' : 'N',
        script::ScriptManager::getInstance().getMemory());
    script::ScriptManager::getInstance().removeAll();
    Logger::flush();
    mMapfile.flush();
    mThreadPool.stop();
    clear();
    mTlsENG.uninit();
    mMapfile.closeAll();
    net::AppUninitTlsLib();
    return 0 == System::unloadNetLib();
}

void Engine::run() {
    if (mMain && mConfig.mMaxProcess > 0) {
        static std::chrono::milliseconds gap(100);
        while (EPS_RUNNING == mProcStatus) {
            if (mProcResponCount > 0) {
                --mProcResponCount;
                for (usz i = 0; i < mChild.size(); i++) {
                    if (!mChild[i].mAlive && EPS_RESPAWN == mChild[i].mStatus) {
                        if (createProcess(i)) {
                            if (!mMain) { // fork on linux
                                mProcResponCount = 0;
                                goto GT_PROC_CHILD;
                            }
                            Logger::log(
                                ELL_INFO, "Engine::run>> success respawn process[%lu] , pid=%d", i, mChild[i].mID);
                        }
                    }
                }
            } else {
                std::this_thread::sleep_for(gap); // TODO: wait...
            }
        }
        std::this_thread::sleep_for(gap);
        for (usz i = 0; i < mChild.size(); ++i) {
            if (mChild[i].mHandle) {
                System::waitProcess(mChild[i].mHandle);
                mChild[i].mHandle = nullptr;
                Logger::log(ELL_INFO, "Engine::run>> process[%d] exit, pid=%d", i, mChild[i].mID);
            }
        }
        return;
    }

GT_PROC_CHILD:
    // if have no child processes, the main process is the only worker.
    while (mLoop.run()) {
    }
}

bool Engine::step() {
    DASSERT(mMain && 0 == mConfig.mMaxProcess); // engine was used on client side.
    return mLoop.run();
}

bool Engine::runMainProcess() {
    if (mConfig.mMaxProcess > 0) {
        mProcStatus = EPS_RUNNING;
        return true;
    }
    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";

    net::SocketPair pair;
    if (!pair.open(unpath.c_str())) {
        Logger::log(ELL_ERROR, "Engine::runMainProcess>> fail to open SocketPair");
        return false;
    }
    mThreadPool.start(mConfig.mMaxThread);
    bool ret = mLoop.start(pair.getSocketB(), pair.getSocketA());
    if (ret) {
        mProcStatus = EPS_RUNNING;
        initTask();
    } else {
        Logger::log(ELL_ERROR, "Engine::runMainProcess>> start loop fail");
    }

    return ret;
}

bool Engine::runChildProcess(net::Socket& cmdsock, net::Socket& write) {
    mThreadPool.start(mConfig.mMaxThread);
    bool ret = mLoop.start(cmdsock, write);
    if (ret) {
        mProcStatus = EPS_RUNNING;
        initTask();
    } else {
        Logger::log(ELL_ERROR, "Engine::runChildProcess>> start loop fail");
    }
    return ret;
}


bool Engine::createProcess() {
    bool ret = true;
    mChild.resize(mConfig.mMaxProcess);
    for (s16 i = 0; i < mConfig.mMaxProcess; ++i) {
        mChild[i].mID = 0;
        mChild[i].mAlive = false;
        mChild[i].mHandle = nullptr;
        mChild[i].mStatus = EPS_INIT;
    }
    for (s16 i = 0; mMain && i < mConfig.mMaxProcess; ++i) {
        if (!createProcess(i)) {
            ret = false;
        }
    }
    return ret;
}


bool Engine::createProcess(usz idx) {
    if (idx >= mChild.size()) {
        return false;
    }

    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";

    Process& nd = mChild[idx];
    net::SocketPair pair;
    if (pair.open(unpath.c_str())) {
        nd.mStatus = EPS_RUNNING;
        nd.mAlive = true;
        nd.mSocket = pair.getSocketA();
        nd.mID = System::createProcess(pair.getSocketB().getValue(), nd.mHandle);
#if defined(DOS_LINUX)
        if (0 == nd.mID) { // child
            mMain = false;
            // pair.getSocketA().close();
            mPID = System::getPID();
            Logger::getInstance().setPID(mPID);
            runChildProcess(pair.getSocketB(), pair.getSocketA());
            return true;
        } else {
            if (nd.mID < 0) { // error
                goto GT_PROC_FAIL;
            }
            pair.getSocketB().close();
        }
#endif
        Logger::log(ELL_INFO, "Engine::createProcess>> success start pid = %d, ppid=%d", nd.mID, mPID);
        return true;
    }

GT_PROC_FAIL:
    Logger::log(ELL_ERROR, "Engine::createProcess>> fail to create Process[%u] fail", idx);
    nd.mID = 0;
    nd.mHandle = nullptr;
    nd.mStatus = EPS_EXITED;
    nd.mAlive = false;
    pair.close();
    return false;
}


void Engine::initTask() {
    for (usz i = 0; i < mConfig.mProxy.size(); ++i) {
        net::TcpProxyHub* pxhub = new net::TcpProxyHub(mConfig.mProxy[i]);
        net::Acceptor* nd = new net::Acceptor(mLoop, net::TcpProxyHub::funcOnLink, pxhub);
        pxhub->drop();
        nd->setTimeout(mConfig.mProxy[i].mTimeout);
        nd->setBackend(mConfig.mProxy[i].mRemote);
        if (0 == nd->open(mConfig.mProxy[i].mLocal)) {
            Logger::log(ELL_INFO, "Engine::init>>start TcpProxy=[%s->%s]", mConfig.mProxy[i].mLocal.getStr(),
                mConfig.mProxy[i].mRemote.getStr());
        } else {
            Logger::log(ELL_ERROR, "Engine::init>>fail TcpProxy=[%s->%s]", mConfig.mProxy[i].mLocal.getStr(),
                mConfig.mProxy[i].mRemote.getStr());
            nd->drop();
        }
    }


    for (usz i = 0; i < mConfig.mWebsite.size(); ++i) {
        switch (mConfig.mWebsite[i].mType) {
        case 0: // http
        case 1: // https
        {
            net::Website* website = new net::Website(mConfig.mWebsite[i]);
            net::Acceptor* nd = new net::Acceptor(mLoop, net::Website::funcOnLink, website);
            website->drop();
            nd->getHandleTCP().setTimeGap(mConfig.mWebsite[i].mTimeout);
            nd->getHandleTCP().setTimeout(mConfig.mWebsite[i].mTimeout);
            if (0 == nd->open(mConfig.mWebsite[i].mLocal)) {
                Logger::log(ELL_INFO, "Engine::init>>start website=%s,path=%s", mConfig.mWebsite[i].mLocal.getStr(),
                    mConfig.mWebsite[i].mRootPath.c_str());
            } else {
                Logger::log(ELL_ERROR, "Engine::init>>fail website=%s,path=%s", mConfig.mWebsite[i].mLocal.getStr(),
                    mConfig.mWebsite[i].mRootPath.c_str());
                // delete website & nd;
                nd->drop();
            }
            break;
        }
        default:
            Logger::log(ELL_ERROR, "Engine::init>>invalid Proxy=%lld, type=%d", i, mConfig.mWebsite[i].mType);
            break;
        }
    }

    // mConfig.mWebsite.clear();
}

} // namespace app
