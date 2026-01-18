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


#include "Net/TlsSession.h"
#include <openssl/conf.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Engine.h"
#include "Net/Hostcheck.h"
#include "Logger.h"


namespace app {
namespace net {

static RingBuffer* AppGetBufOfBIO(BIO* bio) {
    void* data = BIO_get_data(bio);
    DASSERT(data && "BIO data field should not be nullptr");
    return (RingBuffer*)data;
}

static s32 AppCreateBIOBuffer(BIO* bio) {
    BIO_set_shutdown(bio, 1);
    BIO_set_init(bio, 1);
    // AppGetBufOfBIO(bio)->init();
    return 1;
}

static s32 AppDeleteBIOBuffer(BIO* bio) {
    if (bio == nullptr) {
        return 0;
    }
    // AppGetBufOfBIO(bio)->uninit();
    return 1;
}

static s32 AppBIORead(BIO* bio, s8* out, s32 len) {
    s32 bytes;
    RingBuffer* rb;
    BIO_clear_retry_flags(bio);

    rb = AppGetBufOfBIO(bio);
    bytes = rb->read(out, len);

    if (bytes == 0) {
        DASSERT(rb->getRet() <= INT_MAX && "Value is too big for ring buffer BIO read");
        bytes = (s32)rb->getRet();
        if (bytes != 0) {
            BIO_set_retry_read(bio);
        }
    }

    return bytes;
}

static s32 AppBIOWrite(BIO* bio, const s8* data, s32 len) {
    BIO_clear_retry_flags(bio);
    AppGetBufOfBIO(bio)->write(data, len);
    return len;
}

static s32 AppBIOPuts(BIO* bio, const s8* str) {
    abort();
}

static s32 AppBIOGets(BIO* bio, s8* out, s32 size) {
    abort();
}

static long AppBIOCtrl(BIO* bio, s32 cmd, long num, void* ptr) {
    long ret = 1;

    RingBuffer* rb = AppGetBufOfBIO(bio);

    switch (cmd) {
    case BIO_CTRL_RESET:
        rb->reset();
        break;
    case BIO_CTRL_EOF:
        ret = (rb->getSize() == 0);
        break;
    case BIO_C_SET_BUF_MEM_EOF_RETURN:
        rb->setRet(num);
        break;
    case BIO_CTRL_INFO:
        ret = rb->getSize();
        if (ptr != nullptr) {
            *(void**)ptr = nullptr;
        }
        break;
    case BIO_C_SET_BUF_MEM:
        DASSERT(0 && "Can't use SET_BUF_MEM with ring buf BIO");
        abort();
        break;
    case BIO_C_GET_BUF_MEM_PTR:
        DASSERT(0 && "Can't use GET_BUF_MEM_PTR with ring buf BIO");
        ret = 0;
        break;
    case BIO_CTRL_GET_CLOSE:
        ret = BIO_get_shutdown(bio);
        break;
    case BIO_CTRL_SET_CLOSE:
        DASSERT(num <= INT_MAX && num >= INT_MIN && "BIO ctrl value is too big or too small");
        BIO_set_shutdown(bio, (s32)num);
        break;
    case BIO_CTRL_WPENDING:
        ret = 0;
        break;
    case BIO_CTRL_PENDING:
        ret = rb->getSize();
        break;
    case BIO_CTRL_DUP:
    case BIO_CTRL_FLUSH:
        ret = 1;
        break;
    case BIO_CTRL_PUSH:
    case BIO_CTRL_POP:
    default:
        ret = 0;
        break;
    }
    return ret;
}

static BIO_METHOD* G_TLS_FUNC = nullptr;
// call once
void AppInitRingBIO() {
    G_TLS_FUNC = BIO_meth_new(BIO_TYPE_MEM, "RingBuffer");
    if (G_TLS_FUNC) {
        BIO_meth_set_write(G_TLS_FUNC, AppBIOWrite);
        BIO_meth_set_read(G_TLS_FUNC, AppBIORead);
        BIO_meth_set_puts(G_TLS_FUNC, AppBIOPuts);
        BIO_meth_set_gets(G_TLS_FUNC, AppBIOGets);
        BIO_meth_set_ctrl(G_TLS_FUNC, AppBIOCtrl);
        BIO_meth_set_create(G_TLS_FUNC, AppCreateBIOBuffer);
        BIO_meth_set_destroy(G_TLS_FUNC, AppDeleteBIOBuffer);
    }
}

// call once
void AppUninitRingBIO() {
    if (G_TLS_FUNC) {
        BIO_meth_free(G_TLS_FUNC);
        G_TLS_FUNC = nullptr;
    }
}

static BIO* AppCreateBIO(RingBuffer* rb) {
    BIO* bio = BIO_new(G_TLS_FUNC);
    BIO_set_data(bio, rb);
    return bio;
}


s32 TlsSession::kUserDataIndex = -1;


TlsSession::TlsSession(const TlsContext& ctx, RingBuffer* inBuffers, RingBuffer* outBuffers, void* user) {
    SSL_CTX* ssl_ctx = static_cast<SSL_CTX*>(ctx.getTlsContext());
    SSL_CTX_up_ref(ssl_ctx);  // grab for SSL_new 
    mSSL = SSL_new(ssl_ctx);
    setUserData(user);
    BIO* in = AppCreateBIO(inBuffers);
    BIO* out = AppCreateBIO(outBuffers);
    SSL_set_bio(static_cast<SSL*>(mSSL), in, out);
}


TlsSession::~TlsSession() {
    SSL_CTX_free(SSL_get_SSL_CTX(static_cast<SSL*>(mSSL)));
    SSL_free(static_cast<SSL*>(mSSL)); // here have freed in/out BIOs already.
    // BIO_free(mOutBIO);
    // BIO_free(mInBIO);
}

bool TlsSession::setUserData(void* user) {
    DASSERT(static_cast<SSL*>(mSSL));
    if (SSL_set_ex_data(static_cast<SSL*>(mSSL), TlsSession::kUserDataIndex, user) == 0) {
        DLOG(ELL_ERROR, "SSL_set_ex_data: fail, user = %p, ssl = %p", user, static_cast<SSL*>(mSSL));
        return false;
    }
    return true;
}

void* TlsSession::getUserData() {
    DASSERT(static_cast<SSL*>(mSSL));
    return SSL_get_ex_data(static_cast<SSL*>(mSSL), TlsSession::kUserDataIndex);
}


void TlsSession::setAcceptState() {
    SSL_set_accept_state(static_cast<SSL*>(mSSL));
}

void TlsSession::setConnectState() {
    SSL_set_connect_state(static_cast<SSL*>(mSSL));
}

void TlsSession::setHost(const String& it) {
    SSL_set_tlsext_host_name(static_cast<SSL*>(mSSL), it.data());
}

s32 TlsSession::write(const void* buf, s32 len) {
    return SSL_write(static_cast<SSL*>(mSSL), buf, len);
}

s32 TlsSession::read(void* buf, s32 len) {
    return SSL_read(static_cast<SSL*>(mSSL), buf, len);
}

s32 TlsSession::getError(s32 nread) const {
    return SSL_get_error(static_cast<SSL*>(mSSL), nread);
}

s32 TlsSession::handshake() {
    return SSL_do_handshake(static_cast<SSL*>(mSSL));
}

bool TlsSession::isInitFinished() {
    return 0 != SSL_is_init_finished(static_cast<SSL*>(mSSL));
}


s32 TlsSession::showPeerCert() {
    DASSERT(static_cast<SSL*>(mSSL));
    // SSL_get0_peer_certificate 不增加引用
    // SSL_get1_peer_certificate 增加引用, 需要X509_free
    X509* cert = SSL_get_peer_certificate(static_cast<SSL*>(static_cast<SSL*>(mSSL)));
    if (!cert) {
        DLOG(ELL_ERROR, "PeerCert: fail to read peer_cert, %p", static_cast<SSL*>(mSSL));
    }

    std::string tmpstr;
    s64 ver = X509_get_version(cert);
    DLOG(ELL_INFO, "PeerCert: ---Version: %lld", ver);
    {
        X509_NAME* nd = X509_get_subject_name(cert);
        char* subj = nd ? X509_NAME_oneline(nd, nullptr, 0) : nullptr;
        tmpstr = subj ? subj : "";
        DLOG(ELL_INFO, "PeerCert: ---Subject: %s", tmpstr.data());
    }
    {
        X509_NAME* nd = X509_get_issuer_name(cert);
        char* subj = nd ? X509_NAME_oneline(nd, nullptr, 0) : nullptr;
        tmpstr = subj ? subj : "";
        DLOG(ELL_INFO, "PeerCert: ---Issuer: %s", tmpstr.data());
    }
    {
        std::string serialNam;
        ASN1_INTEGER* asniData = X509_get_serialNumber(cert);
        BIGNUM* bigNum = asniData ? ASN1_INTEGER_to_BN(asniData, nullptr) : nullptr;
        if (bigNum) {
            char* hexData = BN_bn2hex(bigNum);
            if (hexData) {
                serialNam = std::string(hexData);
                OPENSSL_free(hexData);
            }
            BN_free(bigNum);
        }
        DLOG(ELL_INFO, "PeerCert: ---SN: %s", serialNam.data());
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        DLOG(ELL_ERROR, "PeerCert: fail to create BIO, %p", static_cast<SSL*>(mSSL));
        X509_free(cert);
        return EE_INTR;
    }
    if (!PEM_write_bio_X509(bio, cert)) {
        DLOG(ELL_ERROR, "PeerCert: fail to dump x509, %p", static_cast<SSL*>(mSSL));
        X509_free(cert);
        BIO_free(bio);
        return EE_INTR;
    }
    const s8* bio_ptr;
    s64 bio_len = BIO_get_mem_data(bio, &bio_ptr);
    std::string cert_pem(bio_ptr, bio_len);
    DLOG(ELL_INFO, "PeerCert: %s", cert_pem.data());

    BIO_free(bio);
    X509_free(cert);
    return EE_OK;
}


void TlsSession::showError() {
    const s8* data;
    s32 flags;
    unsigned long err;
    s8 buf[256];
#if (OPENSSL_VERSION_MAJOR >= 3)
    while ((err = ERR_get_error_all(nullptr, nullptr, nullptr, &data, &flags)) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        DLOG(ELL_ERROR, "OpenSSL: %s, %s", buf, flags & ERR_TXT_STRING ? data : "-");
    }
#else
    while ((err = ERR_get_error_line_data(nullptr, nullptr, &data, &flags)) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        DLOG(ELL_ERROR, "OpenSSL: %s, %s", buf, flags & ERR_TXT_STRING ? data : "-");
    }
#endif
    ERR_clear_error();
}


} // namespace net
} // namespace app
