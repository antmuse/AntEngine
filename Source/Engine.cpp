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
#include "Net/HTTP/HttpLayer.h"


namespace app {

u32 MsgHeader::gSharedSN = 0;
const s8* G_CFGFILE = "Config/config.json";

Engine::Engine() :
    mPPID(0),
    mPID(0),
    mChild(32),
    mMain(true) {
}

Engine::~Engine() {
}

void Engine::clear() {
    //Logger::clear();
    System::removeFile(mConfig.mPidFile.c_str());
}


void Engine::postCommand(s32 val) {
    MsgHeader cmd;
    switch (val) {
    case ECT_EXIT:
    {
        cmd.finish(ECT_EXIT, ++cmd.gSharedSN, ECT_VERSION);
        break;
    }
    case ECT_ACTIVE:
    {
        cmd.finish(ECT_ACTIVE, ++cmd.gSharedSN, ECT_VERSION);
        break;
    }
    default:
        Logger::log(ELL_INFO, "Engine::postCommand>>invalid cmd = %d", val);
        return;
    }

    for (usz i = 0; i < mChild.size(); ++i) {
        if (sizeof(cmd) != mChild[i].mSocket.sendAll(&cmd, sizeof(cmd))) { //block send
            Logger::log(ELL_INFO, "Engine::postCommand>>fail post cmd = %d, pid=%d", val, mChild[i].mID);
        }
    }

    if (ECT_EXIT == val) {
        for (usz i = 0; i < mChild.size(); ++i) {
            if (mChild[i].mHandle) {
                System::waitProcess(mChild[i].mHandle);
                mChild[i].mHandle = nullptr;
                Logger::log(ELL_INFO, "Engine::postCommand>>process[%d] exit, pid=%d",
                    i, mChild[i].mID);
            }
        }
        //if (mMainProcess.mHandle) {
        mMainProcess.mSocket.sendAll(&cmd, sizeof(cmd));
    }
}


bool Engine::init(const s8* fname, bool child) {
    mMain = !child;
    AppStrConverterInit();
#if defined(DOS_WINDOWS)
    s8 tmp[260];
    AppGbkToUTF8(fname, tmp, sizeof(tmp));
    fname = tmp;
#endif

    MsgHeader::gSharedSN = 0;
    mAppPath = fname;
    mAppPath.deleteFilename();
    mAppPath.replace('\\', '/');
    mAppName = fname + mAppPath.getLen();

    if (!mConfig.load(mAppPath, G_CFGFILE)) {
        mConfig.save(mAppPath + G_CFGFILE + ".gen.json");
        return false;
    }

    Timer::getTimeZone();
    System::getCoreCount();
    System::getPageSize();
    System::getDiskSectorSize();

    System::createPath(mConfig.mLogPath);

    Logger::getInstance();

    mPID = System::getPID();
    Logger::addFileReceiver();

    bool ret = 0 == System::loadNetLib();
    if (!ret) {
        Logger::log(ELL_ERROR, "Engine::init>>loadNetLib fail");
        return false;
    }

    mTlsENG.init();

    if (mConfig.mMaxProcess > 0) {
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

        MemSlabPool& mpool = getMemSlabPool();
        if (mMain) {
            new (&mpool)MemSlabPool(mConfig.mMemSize);
        } else {
            mpool.initSlabSize();
        }
    }

    if (mMain) {
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

    if(mMain){
        Logger::addPrintReceiver();
        ret = runMainProcess();
    }else{
#if defined(DOS_WINDOWS)
        net::Socket cmdsock = (net::netsocket)GetStdHandle(STD_INPUT_HANDLE);
        Logger::log(ELL_INFO, "Engine::init>>pid = %d, cmdsock = %llu", mPID, cmdsock.getValue());
        if (!cmdsock.isOpen()) {
            Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');
            return false;
        }

        Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');
        ret = runChildProcess(cmdsock);
#endif
    }

    mPID = System::getPID();
    return ret;
}


bool Engine::uninit() {
    Logger::flush();
    mThreadPool.stop();
    clear();
    mTlsENG.uninit();
    return 0 == System::unloadNetLib();
}

void Engine::run() {
    while (mLoop.run()) {
    }
}

bool Engine::step() {
    return mLoop.run();
}

bool Engine::runMainProcess() {
    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";

    mMainProcess.mHandle = nullptr;
    net::SocketPair pair;
    if (pair.open(unpath.c_str())) {
        mMainProcess.mID = System::getPID();
        mMainProcess.mStatus = 0;
        mMainProcess.mSocket = pair.getSocketA();
    } else {
        Logger::log(ELL_ERROR, "Engine::runMainProcess>> fail");
        return false;
    }
    mThreadPool.start(mConfig.mMaxThread);
    bool ret = mLoop.start(pair.getSocketB());
    if (ret) { initTask(); }
    return ret;
}

bool Engine::runChildProcess(net::Socket& cmdsock) {
    mThreadPool.start(mConfig.mMaxThread);
    bool ret = mLoop.start(cmdsock);
    if (ret) {
        initTask();
    }
    return ret;
}

bool Engine::createProcess() {
    bool ret = true;
    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";

    Process nd;
    mChild.reallocate(mConfig.mMaxProcess);
    net::SocketPair pair;
    for (s16 i = 0; i < mConfig.mMaxProcess; ++i) {
        if (pair.open(unpath.c_str())) {
            nd.mSocket = pair.getSocketA();
            nd.mID = System::createProcess(pair.getSocketB().getValue(), nd.mHandle);
            nd.mStatus = 1;
#if defined(DOS_LINUX)
            if (0 == nd.mID) {//child
                pair.getSocketA().close();
                runChildProcess(pair.getSocketB());
                mMain=false;
                return true;
            } else {
                pair.getSocketB().close();
                if(nd.mID<0){ //error
                    pair.close();
                    continue;
                }
            }
#endif
            mChild.pushBack(nd);
            Logger::log(ELL_INFO, "Engine::createProcess>>pid = %d, cmdsock = %llu", nd.mID, nd.mSocket.getValue());
        } else {
            Logger::log(ELL_ERROR, "Engine::init>>start Process[%d] fail", (s32)i);
            ret = false;
            break;
        }
    }
    return ret;
}


void Engine::initTask() {
    for (usz i = 0; i < mConfig.mProxy.size(); ++i) {
        net::Acceptor* nd = new net::Acceptor(mLoop, net::TcpProxy::funcOnLink, &mConfig.mProxy[i]);
        nd->setTimeout(mConfig.mProxy[i].mTimeout);
        nd->setBackend(mConfig.mProxy[i].mRemote);
        if (0 == nd->open(mConfig.mProxy[i].mLocal)) {
            Logger::log(ELL_INFO, "Engine::init>>start TcpProxy=[%s->%s]",
                mConfig.mProxy[i].mLocal.getStr(), mConfig.mProxy[i].mRemote.getStr());
        } else {
            Logger::log(ELL_ERROR, "Engine::init>>fail TcpProxy=[%s->%s]",
                mConfig.mProxy[i].mLocal.getStr(), mConfig.mProxy[i].mRemote.getStr());
            delete nd;
        }
    }


    for (usz i = 0; i < mConfig.mWebsite.size(); ++i) {
        switch (mConfig.mWebsite[i].mType) {
        case 0: //http
        case 1: //https
        {
            net::Acceptor* nd = new net::Acceptor(mLoop, net::HttpLayer::funcOnLink, &mConfig.mWebsite[i]);
            nd->getHandleTCP().setTimeGap(mConfig.mWebsite[i].mTimeout);
            nd->getHandleTCP().setTimeout(mConfig.mWebsite[i].mTimeout);
            if (0 == nd->open(mConfig.mWebsite[i].mLocal)) {
                Logger::log(ELL_INFO, "Engine::init>>start website=%s,path=%s",
                    mConfig.mWebsite[i].mLocal.getStr(), mConfig.mWebsite[i].mRootPath.c_str());
            } else {
                Logger::log(ELL_ERROR, "Engine::init>>fail website=%s,path=%s",
                    mConfig.mWebsite[i].mLocal.getStr(), mConfig.mWebsite[i].mRootPath.c_str());
                delete nd;
            }
            break;
        }
        default:
            Logger::log(ELL_ERROR, "Engine::init>>invalid Proxy=%lld, type=%d", i, mConfig.mWebsite[i].mType);
            break;
        }
    }
}

} //namespace app
