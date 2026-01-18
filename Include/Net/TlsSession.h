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


#ifndef APP_TLSSESSION_H
#define APP_TLSSESSION_H

#include "RingBuffer.h"
#include "Net/TlsContext.h"


namespace app {
namespace net {

class TlsSession {
public:
    static s32 kUserDataIndex;
    static void showError();

    TlsSession() = delete;

    /**
     * @param ctx, TLS Context
     * @param inBuffers, must not be nullptr
     * @param outBuffers, must not be nullptr
     */
    TlsSession(const TlsContext& ctx, RingBuffer* inBuffers, RingBuffer* outBuffers, void* user = nullptr);

    ~TlsSession();

    bool setUserData(void* user);
    void* getUserData();

    void setAcceptState();

    void setConnectState();

    void setHost(const String& it);

    s32 write(const void* buf, s32 len);

    s32 read(void* buf, s32 len);

    s32 getError(s32 nread) const;

    s32 handshake();

    bool isInitFinished();

    s32 showPeerCert();

    void* getSSL() const {
        return mSSL;
    }

private:
    void* mSSL;
};

} // namespace net
} // namespace app

#endif // APP_TLSSESSION_H