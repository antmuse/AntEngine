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
        Logger::log(ELL_INFO, "Engine::postCommand>>post cmd = %d, pid=%d", val, mChild[i].mID);
        mChild[i].mSocket.sendAll(&cmd, sizeof(cmd)); //block send
    }
}


bool Engine::init(const s8* fname) {
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
    Logger::addPrintReceiver();
    Logger::addFileReceiver();
    //AppInitTlsLib();
    mPID = System::getPID();
    Logger::log(ELL_INFO, "Engine::init>>pid = %d, main = %c", mPID, mMain ? 'Y' : 'N');

    FileWriter file;
    if (file.openFile(mConfig.mPidFile, false)) {
        file.writeParams("%d", mPID);
        file.close();
    } else {
        return false;
    }

    mTlsENG.init();

    if (App4Char2S32("GMEM") == App4Char2S32(mConfig.mMemName.c_str())) {
        if (!mMapfile.createMem(mConfig.mMemSize, mConfig.mMemName.c_str(), false, true)) {
            Logger::log(ELL_INFO, "Engine::init>>createMem fail = %s", mConfig.mMemName.c_str());
            return false;
        }
    } else {
        if (!mMapfile.createMapfile(mConfig.mMemSize, mConfig.mMemName.c_str(), false, false, true)) {
            Logger::log(ELL_INFO, "Engine::init>>createMapfile fail = %s", mConfig.mMemName.c_str());
            return false;
        }
    }

    //snprintf((s8*)mMapfile.getMem(), mMapfile.getMemSize(), "map=%s, size=%llu/%llu", mConfig.mMemName.c_str(), mConfig.mMemSize, mConfig.mMemSize / 1024 / 1024);
    createProcess();
    return mChild.size() > 0;
}


bool Engine::uninit() {
    mThreadPool.stop();
    clear();
    mTlsENG.uninit();
    return 0 == System::unloadNetLib();
}

bool Engine::run(bool step) {
    bool ret;
    if (mConfig.mMaxProcess < 1) {
        //std::chrono::milliseconds waitms(10);
        ret = mLoop.run();
        if (!step) {
            while (ret) {
                //std::this_thread::sleep_for(waitms);
                ret = mLoop.run();
            }
        }
        return ret;
    }

    runChildProcess();
    return true;
}

void Engine::runMainProcess() {

}

void Engine::runChildProcess() {
    //std::chrono::milliseconds waitms(10);
    while (mLoop.run()) {
        //std::this_thread::sleep_for(waitms);
    }
}

void Engine::createProcess() {
    bool ret = 0 == System::loadNetLib();

    String unpath = Engine::getInstance().getConfig().mLogPath;
    unpath += System::getPID();
    unpath += ".unpath";

    Process nd;
    mChild.reallocate(mConfig.mMaxProcess);
    net::SocketPair pair;
    if (mConfig.mMaxProcess < 1) {
        if (pair.open(unpath.c_str())) {
            nd.mID = System::getPID();
            nd.mStatus = 0;
            nd.mSocket = pair.getSocketA();
            mChild.pushBack(nd);
        } else {
            Logger::log(ELL_ERROR, "Engine::init>>start main Process fail");
        }
    } else {
        for (s16 i = 0; i < mConfig.mMaxProcess; ++i) {
            if (pair.open(unpath.c_str())) {
                nd.mID = System::getPID();
                nd.mStatus = 0;
                nd.mSocket = pair.getSocketA();
                mChild.pushBack(nd);
            } else {
                Logger::log(ELL_ERROR, "Engine::init>>start Process[%d] fail", (s32)i);
                break;
            }
        }
    }

    mThreadPool.start(mConfig.mMaxThread);
    mLoop.start(pair.getSocketB());

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
    //return ret;
}

} //namespace app
