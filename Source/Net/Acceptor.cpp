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


#include "Net/Acceptor.h"
#include "System.h"

namespace app {
namespace net {


Acceptor::Acceptor(Loop& loop, FuncReqCallback func, RefCount* iUser) :
    mFlyCount(0),
    mOnLink(func),
    mLoop(loop) {
    mTCP.setClose(EHT_TCP_ACCEPT, Acceptor::funcOnClose, this);

    //accept无需加入时间堆
    mTCP.setTime(nullptr, 30 * 1000, 60 * 1000, -1);

    setUser(iUser);
    memset(mFlyRequests, 0, sizeof(mFlyRequests));
}


Acceptor::~Acceptor() {
    for (s32 i = 0; i < GMaxFly; ++i) {
        if (mFlyRequests[i]) {
            delete mFlyRequests[i];
            mFlyRequests[i] = nullptr;
        }
    }
}


s32 Acceptor::close() {
    return mTCP.launchClose();
}


s32 Acceptor::postAccept() {
    s32 ret = EE_OK;
    for (s32 i = 0; i < GMaxFly; ++i) {
        mFlyRequests[i] = new RequestAccept(Acceptor::funcOnLink, this, mTCP.getLocal().getAddrSize());
        ret = mTCP.accept(mFlyRequests[i]);
        if (0 != ret) {
            delete mFlyRequests[i];
            mFlyRequests[i] = nullptr;
            Logger::log(ELL_INFO, "Acceptor::postAccept>>accept ecode=%d, fly=%d",
                System::getAppError(), mFlyCount);
            break;
        }
        ++mFlyCount;
    }
    Logger::log(ELL_INFO, "Acceptor::postAccept>>accept fly=%d", mFlyCount);
    return ret;
}


s32 Acceptor::open(const String& addr) {
    mTCP.setLocal(addr);
    return open(mTCP.getLocal());
}


s32 Acceptor::open(const NetAddress& addr) {
    mFlyCount = 0;
    mTCP.setLocal(addr);
    const_cast<NetAddress&>(mTCP.getRemote()).setAddrSize(mTCP.getLocal().getAddrSize());
    s32 ret = mLoop.openHandle(&mTCP);
    if (EE_OK == ret) {
        postAccept();
        Logger::log(ELL_INFO, "Acceptor::open>>listen=%s, backend=%s",
            mTCP.getLocal().getStr(), mTCP.getRemote().getStr());
    }
    //setUser(nullptr);
    return ret;
}


void Acceptor::onClose(Handle* it) {
    Logger::log(ELL_INFO, "Acceptor::onClose>>listen=%s, backend=%s, fly=%d",
        mTCP.getLocal().getStr(), mTCP.getRemote().getStr(), mFlyCount);
    setUser(nullptr);
    drop(); // delete this;
}


void Acceptor::onLink(RequestFD* it) {
    RequestAccept* req = (RequestAccept*)it;
    if (0 != it->mError) {
        --mFlyCount;
        if (!mLoop.isStop()) {
            Logger::log(ELL_ERROR, "Acceptor::onLink>>onpost, ecode=%d, it=%p, fly=%d",
                it->mError, req, mFlyCount);
        }
        return;
    }
    if (mOnLink) {
        mOnLink(it);
    } else {
        s64 sock = req->mSocket.getValue();
        Logger::log(ELL_INFO, "Acceptor::onLink>>none accept for [%s->%s],sock=%lld",
            req->mRemote.getStr(), req->mLocal.getStr(), sock);
        req->mSocket.close();
    }
    s32 ret = mTCP.accept(req);
    if (0 != ret) {
        --mFlyCount;
        Logger::log(ELL_ERROR, "Acceptor::onLink>>repost ecode=%d, fly=%d",
            System::getAppError(), mFlyCount);
    }
}


} //namespace net
} //namespace app
