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


#ifndef APP_ACCEPTOR_H
#define	APP_ACCEPTOR_H

#include "RefCount.h"
#include "Net/Socket.h"
#include "Net/HandleTCP.h"
#include "Net/NetAddress.h"
#include "Loop.h"
#if defined(DOS_WINDOWS)
#include "Windows/Request.h"
#elif defined(DOS_ANDROID) || defined(DOS_LINUX)
#include "Linux/Request.h"
#endif


namespace app {
namespace net {

class Acceptor :public RefCount {
public:
    Acceptor(Loop& loop, FuncReqCallback func, RefCount* iUser = nullptr);

    virtual ~Acceptor();

    RefCount* getUser() {
        return reinterpret_cast<RefCount*>(mTCP.getLink().mParent);
    }
    void setUser(RefCount* it) {
        RefCount* my = getUser();
        if (my) {
            my->drop();
        }
        if (it) {
            it->grab();
        }
        mTCP.getLink().mParent = (Node3*)it; //借用时间堆mLink上指针，存iUser
    }


    s32 postAccept();

    s32 open(const s8* addr) {
        String str = addr;
        return open(str);
    }

    s32 open(const String& addr);

    s32 open(const NetAddress& addr);

    s32 close();

    void setTimeout(s64 seconds) {
        mTCP.setTimeout(seconds);
        mTCP.setTimeGap(seconds);
    }

    void setBackend(const String& addr) {
        mTCP.setRemote(addr);
    }

    void setBackend(const NetAddress& addr) {
        mTCP.setRemote(addr);
    }


    net::HandleTCP& getHandleTCP() {
        return mTCP;
    }



private:
    void onClose(Handle* it);

    void onLink(RequestFD* it);

    static void funcOnClose(Handle* it) {
        Acceptor& nd = *(Acceptor*)it->getUser();
        nd.onClose(it);
    }

    static void funcOnLink(RequestFD* it) {
        Acceptor& nd = *(Acceptor*)it->mUser;
        nd.onLink(it);
    }

    const static s32 GMaxFly = 10;
    s32 mFlyCount;
    FuncReqCallback mOnLink;
    RequestAccept* mFlyRequests[GMaxFly];
    Loop& mLoop;
    net::HandleTCP mTCP;
};

} //namespace net
} //namespace app


#endif //APP_ACCEPTOR_H
