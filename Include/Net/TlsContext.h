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


#ifndef APP_TLSCONTEXT_H
#define APP_TLSCONTEXT_H

#include "EngineConfig.h"

namespace app {
namespace net {

// call once
void AppInitTlsLib();
// call once
void AppUninitTlsLib();

bool AppSetTlsUserData(void* ssl, void* user);
void* AppGetTlsUserData(const void* ssl);

enum ETLSVerifyFlag {
    ETLS_VERIFY_NONE = 0x00,
    ETLS_VERIFY_PEER = 0x01,
    ETLS_VERIFY_FAIL_IF_NO_PEER_CERT = 0x02,
    ETLS_VERIFY_CLIENT_ONCE = 0x04,
    ETLS_VERIFY_POST_HANDSHAKE = 0x08
};

class TlsContext {
public:
    TlsContext();

    ~TlsContext();

    s32 init(const EngineConfig::TlsConfig& cfg);

    void uninit();

    s32 setVerifyFlags(s32 iVerifyFlags, s32 depth = 4);

    /**
     * @brief suggest use TLS "v1.2, v1.3" for safe.
     * @param it eg: "v1.0, v1.1, v1.2, v1.3"
     */
    s32 setVersion(const String& it);

    s32 addTrustedCerts(const s8* cert, usz length);

    s32 setCert(const s8* cert, usz length);

    s32 setPrivateKey(const s8* key, usz length);

    s32 getVerifyFlags() const {
        return mVerifyFlags;
    }

    void* getTlsContext() const {
        return mTlsContext;
    }

private:
    void* mTlsContext; // SSL_CTX
    s32 mVerifyFlags;
};


} // namespace net
} // namespace app

#endif // APP_TLSCONTEXT_H
