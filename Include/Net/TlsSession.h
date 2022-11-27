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
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>

namespace app {
namespace net {

class TlsSession {
public:
    static void showError();

    TlsSession() = delete;

    /**
     * @param ssl_ctx, must not be nullptr
     * @param inBuffers, must not be nullptr
     * @param outBuffers, must not be nullptr
     */
    TlsSession(SSL_CTX* ssl_ctx, RingBuffer* inBuffers, RingBuffer* outBuffers);

    ~TlsSession();

    void setAcceptState();

    void setConnectState();

    void setHost(const s8* mHostName);

    s32 write(const void* buf, s32 len);

    s32 read(void* buf, s32 len);

    s32 getError(s32 nread)const;

    s32 handshake();

    bool isInitFinished();

    s32 verify(s32 verify_flags, const s8* hostname);

private:
    SSL* mSSL;
    BIO* mInBIO;
    BIO* mOutBIO;
};

}//namespace net
}//namespace app

#endif //APP_TLSSESSION_H