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


#ifndef APP_HANDLETCP_H
#define	APP_HANDLETCP_H

#include "Handle.h"
#include "Net/Socket.h"


namespace app {
class RequestFD;
class RequestAccept;

namespace net {


class HandleTCP : public HandleTime {
public:
    HandleTCP() {
        //memset(this, 0, DOFFSETP(this, mSock));
        mType = EHT_TCP_CONNECT;
    }

    ~HandleTCP() { }

    const NetAddress& getLocal()const {
        return mLocal;
    }

    const NetAddress& getRemote()const {
        return mRemote;
    }

    void setRemote(const String& it) {
        mRemote.setIPort(it.c_str());
    }

    void setRemote(const NetAddress& it) {
        mRemote = it;
    }

    void setLocal(const String& it) {
        mLocal.setIPort(it.c_str());
    }

    void setLocal(const NetAddress& it) {
        mLocal = it;
    }

    Socket& getSock() {
        return mSock;
    }

    void setSocket(const Socket& it) {
        mSock = it;
    }

    s32 accept(RequestAccept* it);

    s32 connect(RequestFD* it);

    s32 write(RequestFD* it);

    s32 read(RequestFD* it);

    s32 close();

    /**
    * @brief launch connect
    * @param addr remote ip
    * @param it connect request
    * @return 0 if success, else failed.
    */
    s32 open(const String& addr, RequestFD* it);
    s32 open(const NetAddress& addr, RequestFD* it);

    /**
    * @brief launch linked sock
    * @param accp accpected request
    * @param it read request, don't start reading if null.
    * @return 0 if success, else failed and close the socket.
    */
    s32 open(const RequestAccept& accp, RequestFD* it);

protected:
    friend class app::Loop;

    NetAddress mLocal;
    NetAddress mRemote;
    Socket mSock;
};


} //namespace net
} //namespace app

#endif //APP_HANDLETCP_H
