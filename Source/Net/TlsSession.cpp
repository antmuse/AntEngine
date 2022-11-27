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
#include "Net/TlsSession.h"
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
    //AppGetBufOfBIO(bio)->init();
    return 1;
}

static s32 AppDeleteBIOBuffer(BIO* bio) {
    if (bio == nullptr) {
        return 0;
    }
    //AppGetBufOfBIO(bio)->uninit();
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
        DASSERT(num <= INT_MAX && num >= INT_MIN &&
            "BIO ctrl value is too big or too small");
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
//call once
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

//call once
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


enum EHostMatch {
    EHM_MATCH = 0,
    EHM_NO_MATCH,
    EHM_BAD_CERT,
    EHM_NO_SAN_PRESENT
};

static EHostMatch AppScanHost(X509* peer_cert, const s8* mHostName) {
    EHostMatch result = EHM_NO_MATCH;
    STACK_OF(GENERAL_NAME)* names = (STACK_OF(GENERAL_NAME)*) (X509_get_ext_d2i(
        peer_cert, NID_subject_alt_name, nullptr, nullptr));
    if (names == nullptr) {
        return EHM_NO_SAN_PRESENT;
    }

    for (s32 i = 0; i < sk_GENERAL_NAME_num(names); ++i) {
        GENERAL_NAME* name = sk_GENERAL_NAME_value(names, i);

        if (name->type == GEN_DNS) {
            const s8* common_name;
            ASN1_STRING* str = name->d.dNSName;
            if (str == nullptr) {
                result = EHM_BAD_CERT;
                break;
            }

            common_name = (const s8*)(ASN1_STRING_get0_data(str));
            if (strlen(common_name) != (usz)(ASN1_STRING_length(str))) {
                result = EHM_BAD_CERT;
                break;
            }

            if (AppCheckHost(common_name, mHostName) == true) {
                result = EHM_MATCH;
                break;
            }
        }
    }
    sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);

    return result;
}

static EHostMatch AppMatchCommonName(X509* peer_cert, const s8* mHostName) {
    s32 i = -1;
    X509_NAME* name = X509_get_subject_name(peer_cert);
    if (name == nullptr) {
        return EHM_BAD_CERT;
    }

    while ((i = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0) {
        ASN1_STRING* str;
        const s8* common_name;
        X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(name, i);
        if (name_entry == nullptr) {
            return EHM_BAD_CERT;
        }

        str = X509_NAME_ENTRY_get_data(name_entry);
        if (str == nullptr) {
            return EHM_BAD_CERT;
        }

        common_name = (const s8*)(ASN1_STRING_get0_data(str));
        if (strlen(common_name) != (usz)(ASN1_STRING_length(str))) {
            return EHM_BAD_CERT;
        }

        if (AppCheckHost(common_name, mHostName) == true) {
            return EHM_MATCH;
        }
    }

    return EHM_NO_MATCH;
}

static EHostMatch AppMatchHost(X509* peer_cert, const s8* mHostName) {
    EHostMatch result = AppScanHost(peer_cert, mHostName);
    if (result == EHM_NO_SAN_PRESENT) {
        result = AppMatchCommonName(peer_cert, mHostName);
    }
    return result;
}




TlsSession::TlsSession(SSL_CTX* ssl_ctx, RingBuffer* inBuffers, RingBuffer* outBuffers) {
    DASSERT(ssl_ctx);
    SSL_CTX_up_ref(ssl_ctx);
    mSSL = SSL_new(ssl_ctx);
    mInBIO = AppCreateBIO(inBuffers);
    mOutBIO = AppCreateBIO(outBuffers);
    SSL_set_bio(mSSL, mInBIO, mOutBIO);
}


TlsSession::~TlsSession() {
    SSL_CTX_free(SSL_get_SSL_CTX(mSSL));
    SSL_free(mSSL);//here have freed mOutBIO and mInBIO already.
    //BIO_free(mOutBIO);
    //BIO_free(mInBIO);
}


void TlsSession::setAcceptState() {
    SSL_set_accept_state(mSSL);
}

void TlsSession::setConnectState() {
    SSL_set_connect_state(mSSL);
}

void TlsSession::setHost(const s8* hostName) {
    SSL_set_tlsext_host_name(mSSL, hostName);
}

s32 TlsSession::write(const void* buf, s32 len) {
    return SSL_write(mSSL, buf, len);
}

s32 TlsSession::read(void* buf, s32 len) {
    return SSL_read(mSSL, buf, len);
}

s32 TlsSession::getError(s32 nread)const {
    return SSL_get_error(mSSL, nread);
}

s32 TlsSession::handshake() {
    return SSL_do_handshake(mSSL);
}

bool TlsSession::isInitFinished() {
    return 0 != SSL_is_init_finished(mSSL);
}

s32 TlsSession::verify(s32 verify_flags, const s8* hostname) {
    if (!verify_flags) {
        return EE_OK;
    }

    X509* peer_cert = SSL_get_peer_certificate(mSSL);
    if (peer_cert == nullptr) {
        return EE_ERROR;
    }

    if (verify_flags & ETLS_VERIFY_CERT) {
        long rc = SSL_get_verify_result(mSSL);
        if (rc != X509_V_OK) {
            X509_free(peer_cert);
            return ETLS_VERIFY_CERT;
        }
    }

    s32 ret = EE_OK;
    if (verify_flags & ETLS_VERIFY_HOST) {
        ret = ETLS_VERIFY_HOST;
        switch (AppMatchHost(peer_cert, hostname)) {
        case EHM_MATCH:
            ret = EE_OK;
            break;
        case EHM_NO_MATCH:
        case EHM_BAD_CERT:
        default: break;
        }
    }

    X509_free(peer_cert);
    return ret;
}


void TlsSession::showError() {
    const s8* data;
    s32 flags;
    unsigned long err;
    s8 buf[256];
#if (OPENSSL_VERSION_MAJOR >= 3)
    while ((err = ERR_get_error_all(nullptr, nullptr, nullptr, &data, &flags)) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        Logger::logError("TlsSession::showError, %s, %s", buf, flags & ERR_TXT_STRING ? data : "-");
    }
#else
    while ((err = ERR_get_error_line_data(nullptr, nullptr, &data, &flags)) != 0) {
        ERR_error_string_n(err, buf, sizeof(buf));
        Logger::logError("TlsSession::showError, %s, %s", buf, flags & ERR_TXT_STRING ? data : "-");
    }
#endif
    ERR_clear_error();
}

}//namespace net
}//namespace app
