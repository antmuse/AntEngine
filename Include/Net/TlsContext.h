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

#include "Config.h"

namespace app {
namespace net {

// call once
void AppInitTlsLib();
// call once
void AppUninitTlsLib();

//不验证
//验证书,default
//验证书+域名
enum ETLSVerifyFlag {
    ETLS_VERIFY_NONE = 0x00,
    ETLS_VERIFY_CERT = 0x01,
    ETLS_VERIFY_HOST = 0x02,
    ETLS_VERIFY_CERT_HOST = ETLS_VERIFY_CERT | ETLS_VERIFY_HOST
};

class TlsContext {
public:
    TlsContext();

    ~TlsContext();

    s32 init(s32 vflag = ETLS_VERIFY_CERT, bool debug = false);

    void uninit();

    void setVerifyFlags(s32 iVerifyFlags);

    s32 addTrustedCerts(const s8* cert, usz length);

    s32 setCert(const s8* cert, usz length);

    s32 setPrivateKey(const s8* key, usz length);

    s32 getVerifyFlags()const {
        return mVerifyFlags;
    }

    void* getTlsContext()const {
        return mTlsContext;
    }

private:
    void* mTlsContext;  // SSL_CTX
    s32 mVerifyFlags;
};


}//namespace net
}//namespace app

#endif //APP_TLSCONTEXT_H
