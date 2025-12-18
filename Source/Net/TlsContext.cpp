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


#include "Net/TlsContext.h"
#include <cstring>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include "Logger.h"
#include "RingBuffer.h"
#include "Net/Hostcheck.h"
#include "Certs.h"
#include "Spinlock.h"
#include "Net/TlsSession.h"
#include "Net/HandleTLS.h"
#include "FileRWriter.h"
#include "Packet.h"

#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
#error "openssl version is too low"
#endif



namespace app {
namespace net {

// in file Net/TlsSession.cpp
extern void AppInitRingBIO();
extern void AppUninitRingBIO();


static Spinlock G_TLS_LOCK;
static s32 GUSER_DATA_IDX = -1;
#define DHTTP2_ALPN_PROTO "\x02h2\x08http/1.1\x08http/1.0"
#define DHTTP_ALPN_PROTO "\x08http/1.1\x08http/1.0"

// call once
void AppUninitTlsLib() {
    if (!G_TLS_LOCK.tryUnlock()) {
        return;
    }
    AppUninitRingBIO();

    RAND_cleanup();
    ENGINE_cleanup();
    CONF_modules_unload(1);
    CONF_modules_free();
    EVP_cleanup();
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();
    CRYPTO_set_locking_callback(nullptr);
    CRYPTO_set_id_callback(nullptr);
}

// call once
void AppInitTlsLib() {
    if (!G_TLS_LOCK.tryLock()) {
        return;
    }
    // OPENSSL_no_config();
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    // atexit(AppUninitTlsLib);
    AppInitRingBIO();

    GUSER_DATA_IDX = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (GUSER_DATA_IDX == -1) {
        DLOG(ELL_ERROR, "SSL_get_ex_new_index() failed");
    } else {
        DLOG(ELL_INFO, "SSL_get_ex_new_index = %d", GUSER_DATA_IDX);
    }
}

bool AppSetTlsUserData(void* ssl, void* user) {
    DASSERT(ssl);
    if (SSL_set_ex_data(static_cast<SSL*>(ssl), GUSER_DATA_IDX, user) == 0) {
        DLOG(ELL_ERROR, "SSL_set_ex_data: fail, user = %p, ssl = %p", user, ssl);
        return false;
    }
    return true;
}

void* AppGetTlsUserData(const void* ssl) {
    DASSERT(ssl);
    return SSL_get_ex_data(static_cast<const SSL*>(ssl), GUSER_DATA_IDX);
}


static s32 FuncSelectALPN(SSL* ssl, const u8** out, u8* outlen, const u8* in, u32 inlen, void* arg) {
    net::HandleTLS* conn = reinterpret_cast<net::HandleTLS*>(AppGetTlsUserData(ssl));
    const usz cfg = reinterpret_cast<usz>(arg);
#ifdef DDEBUG
    for (u32 i = 0; i < inlen; i += in[i] + 1) {
        DLOG(ELL_INFO, "SSL ALPN supported by client: [%d] %.*s", (s32)in[i], static_cast<s32>(in[i]), in + i + 1);
    }
#endif
    const u8* proto = (const u8*)(2 == cfg ? DHTTP2_ALPN_PROTO : DHTTP_ALPN_PROTO);
    u32 protolen = (2 == cfg ? sizeof(DHTTP2_ALPN_PROTO) : sizeof(DHTTP_ALPN_PROTO)) - 1;
    /*  openssl will do this in what order?
    bool goon = true;
    while (proto[0]) {
        for (u32 i = 0; i < inlen; i += in[i] + 1) {
            if (in[i] == proto[0] && 0 == memcmp(proto + 1, in + i + 1, in[i])) {
                protolen = proto[0] + 1;
                goon = false;
                break;
            }
        }
        if (goon) {
            proto += proto[0] + 1;
            protolen -= proto[0] + 1;
        } else {
            DLOG(ELL_INFO, "SSL ALPN match: [%d/%u] %.*s", static_cast<s32>(proto[0]), protolen,
    static_cast<s32>(proto[0]), proto + 1); break;
        }
    }*/
    if (OPENSSL_NPN_NEGOTIATED != SSL_select_next_proto((u8**)out, outlen, proto, protolen, in, inlen)) {
        DLOG(ELL_ERROR, "fail SSL ALPN selected: %.*s, conn=%p", static_cast<s32>(*outlen), *out, arg, conn);
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    conn->setALPN(2 == *outlen ? 2 : 1);
    DLOG(ELL_INFO, "success SSL ALPN selected: %.*s, conn=%p", static_cast<s32>(*outlen), *out, conn);
    return SSL_TLSEXT_ERR_OK;
}



static inline void AppTlsLog(const SSL* ssl, s32 w, s32 flag, const s8* msg) {
    if (w & flag) {
        Logger::logInfo("AppTlsLog>>[%s]-[%s-%s]", msg, SSL_state_string(ssl), SSL_state_string_long(ssl));
    }
}


static void AppFunTlsDebug(const SSL* ssl, s32 where, s32 ret) {
    if (ret == 0) {
        fprintf(stderr, "info_callback, error occurred.\n");
        return;
    }
    AppTlsLog(ssl, where, SSL_CB_LOOP, "LOOP");
    AppTlsLog(ssl, where, SSL_CB_EXIT, "EXIT");
    AppTlsLog(ssl, where, SSL_CB_READ, "READ");
    AppTlsLog(ssl, where, SSL_CB_WRITE, "WRITE");
    AppTlsLog(ssl, where, SSL_CB_ALERT, "ALERT");
    AppTlsLog(ssl, where, SSL_CB_HANDSHAKE_START, "HSHAKE-START");
    AppTlsLog(ssl, where, SSL_CB_HANDSHAKE_DONE, "HSHAKE-DONE");
}



/* Callback for passing a keyfile password stored as an str to OpenSSL */
static s32 AppTlsPasswordCallback(s8* buf, s32 size, s32 rwflag, void* user) {
    if (!user) {
        return -1;
    }
    const String* pass = reinterpret_cast<String*>(user);
    if (pass->size() > (size_t)size) {
        return -1;
    }
    memcpy(buf, pass, pass->size());
    return (s32)pass->size();
}

TlsContext::TlsContext() : mTlsContext(nullptr), mVerifyFlags(ETLS_VERIFY_NONE) {
}

TlsContext::~TlsContext() {
}

s32 TlsContext::init(const EngineConfig::TlsConfig& cfg) {
    if (mTlsContext) {
        return 0;
    }
    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_method());
    if (!ssl_ctx) {
        TlsSession::showError();
        return EE_ERROR;
    }

    mTlsContext = ssl_ctx;
    if (cfg.mDebug) {
        SSL_CTX_set_info_callback(ssl_ctx, AppFunTlsDebug);
    }
    setVerifyFlags(cfg.mTlsVerify, cfg.mTlsVerifyDepth);
    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif
#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    SSL_CTX_set_options(ssl_ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_RENEGOTIATION);
    SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_DH_USE);
#if ((OPENSSL_VERSION_NUMBER < 0x30000000L) && defined(SSL_CTX_set_ecdh_auto))
    SSL_CTX_set_ecdh_auto(ssl_ctx, 1);
#endif
    if (cfg.mPreferServerCiphers) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
    }
    if (EE_OK != setVersion(cfg.mTlsVersionOff)) {
        DLOG(ELL_ERROR, "set version off err = %s", cfg.mTlsVersionOff.data());
    }

    s32 ret = addTrustedCerts(G_CA_CERT, strlen(G_CA_CERT));
    ret = addTrustedCerts(G_CA_CERT1, strlen(G_CA_CERT1));
    ret = addTrustedCerts(G_CA_ROOT_CERT, strlen(G_CA_ROOT_CERT));
    Packet buf(1024);
    FileRWriter keyfile;
    if (!cfg.mTlsPassword.empty()) {
        SSL_CTX_set_default_passwd_cb(static_cast<SSL_CTX*>(mTlsContext), AppTlsPasswordCallback);
        SSL_CTX_set_default_passwd_cb_userdata(static_cast<SSL_CTX*>(mTlsContext), (void*)(&cfg.mTlsPassword));
    }
    if (keyfile.openFile(cfg.mTlsPathCA)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != addTrustedCerts(buf.getPointer(), buf.size())) {
                DLOG(ELL_ERROR, "set CA err = %s", cfg.mTlsPathCA.data());
            }
        }
    } else {
        DLOG(ELL_ERROR, "fail to open CA file = %s", cfg.mTlsPathCA.data());
    }
    if (keyfile.openFile(cfg.mTlsPathCert)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != setCert(buf.getPointer(), buf.size())) {
                DLOG(ELL_ERROR, "set cert err = %s", cfg.mTlsPathCert.data());
            }
        }
    } else {
        ret = setCert(G_SERVER_CERT, strlen(G_SERVER_CERT));
        DLOG(ELL_ERROR, "fail to open cert file = %s", cfg.mTlsPathCert.data());
    }
    if (keyfile.openFile(cfg.mTlsPathKey)) {
        buf.resize(keyfile.getFileSize());
        if (buf.size() == keyfile.read(buf.getPointer(), buf.size())) {
            if (EE_OK != setPrivateKey(buf.getPointer(), buf.size())) {
                DLOG(ELL_ERROR, "set key err = %s", cfg.mTlsPathKey.data());
            }
        }
    } else {
        ret = setPrivateKey(G_SERVER_KEY, strlen(G_SERVER_KEY));
        DLOG(ELL_ERROR, "fail to open key file = %s", cfg.mTlsPathKey.data());
    }
    if (!cfg.mTlsCiphers.empty()) {
        if (!SSL_CTX_set_cipher_list(static_cast<SSL_CTX*>(mTlsContext), cfg.mTlsCiphers.data())) {
            DLOG(ELL_ERROR, "fail to set ciphers = %s", cfg.mTlsCiphers.data());
        } else {
            DLOG(ELL_INFO, "set ciphers = %s", cfg.mTlsCiphers.data());
        }
    }
    if (!cfg.mTlsCiphersuites.empty()) {
#ifdef TLS1_3_VERSION
        if (!SSL_CTX_set_ciphersuites(static_cast<SSL_CTX*>(mTlsContext), cfg.mTlsCiphersuites.data())) {
            DLOG(ELL_ERROR, "fail to set ciphersuites = %s", cfg.mTlsCiphersuites.data());
        } else {
            DLOG(ELL_INFO, "set ciphersuites = %s", cfg.mTlsCiphersuites.data());
        }
#else
        DLOG(ELL_WARN, "TLS v1.3 is not support, on ciphersuites set");
#endif
    }

    SSL_CTX_set_alpn_select_cb(static_cast<SSL_CTX*>(mTlsContext), FuncSelectALPN, (void*)(cfg.mHttpALPN));
    DLOG(ELL_INFO, "set ALPN %p = %d", mTlsContext, cfg.mHttpALPN);
    return EE_OK;
}

void TlsContext::uninit() {
    if (mTlsContext) {
        SSL_CTX_free(static_cast<SSL_CTX*>(mTlsContext));
        mTlsContext = nullptr;
    }
}


s32 TlsContext::setVersion(const String& it) {
    if (!mTlsContext) {
        DLOG(ELL_ERROR, "invalid TLS Context");
        return EE_ERROR;
    }
    String logs("disable TLS Version:");
    u64 flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    if (it.find("v1.0") >= 0) {
        logs += " v1.0";
        flags |= SSL_OP_NO_TLSv1;
    }
    if (it.find("v1.1") >= 0) {
        logs += " v1.1";
        flags |= SSL_OP_NO_TLSv1_1;
    }
    if (it.find("v1.2") >= 0) {
        logs += " v1.2";
        flags |= SSL_OP_NO_TLSv1_2;
    }
    if (it.find("v1.3") >= 0) {
#ifdef SSL_OP_NO_TLSv1_3
        logs += " v1.3";
        flags |= SSL_OP_NO_TLSv1_3;
#else
        DLOG(ELL_WARN, "TLS v1.3 is not support, on version set");
#endif
    }
    SSL_CTX_set_options(static_cast<SSL_CTX*>(mTlsContext), flags);
    DLOG(ELL_INFO, "%s", logs.data());
    return EE_OK;
}


s32 TlsContext::setVerifyFlags(s32 iflags, s32 depth) {
    if (!mTlsContext) {
        DLOG(ELL_ERROR, "invalid TLS Context");
        return EE_ERROR;
    }
    mVerifyFlags = iflags;
    DLOG(ELL_INFO, "TLS VerifyFlags = 0X%x,  depth = %d", iflags, depth);
    s32 val = SSL_VERIFY_NONE;
    if (ETLS_VERIFY_PEER & iflags) {
        val |= SSL_VERIFY_PEER;
    }
    if (ETLS_VERIFY_FAIL_IF_NO_PEER_CERT & iflags) {
        val |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    if (ETLS_VERIFY_CLIENT_ONCE & iflags) {
        val |= SSL_VERIFY_CLIENT_ONCE;
    }
    if (ETLS_VERIFY_POST_HANDSHAKE & iflags) {
        val |= SSL_VERIFY_POST_HANDSHAKE;
    }
    SSL_CTX_set_verify_depth(static_cast<SSL_CTX*>(mTlsContext), depth);
    SSL_CTX_set_verify(static_cast<SSL_CTX*>(mTlsContext), val, nullptr);
    return EE_OK;
}

s32 TlsContext::addTrustedCerts(const s8* cert, usz length) {
    X509* x509;
    X509_STORE* trusted_store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(mTlsContext));
    if (!trusted_store) {
        DLOG(ELL_ERROR, "fail to get root X509_STORE");
        TlsSession::showError();
        return EE_ERROR;
    }
    BIO* bio = BIO_new_mem_buf(cert, (s32)length);
    if (bio == nullptr) {
        TlsSession::showError();
        return EE_ERROR;
    }

    s32 ncerts = 0;
    while ((x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) != nullptr) {
        X509_STORE_add_cert(trusted_store, x509);
        X509_free(x509);
        ncerts++;
    }

    BIO_free_all(bio);
    DLOG(ELL_INFO, "LoadCA count = %d", ncerts);
    return ncerts == 0 ? EE_ERROR : 0;
}



s32 TlsContext::setCert(const s8* cert, usz length) {
    if (length > 0x7FFFFFFF) {
        return EE_INVALID_PARAM;
    }
    BIO* bio = BIO_new_mem_buf(cert, (s32)length);
    if (nullptr == bio) {
        TlsSession::showError();
        return EE_ERROR;
    }

    X509* x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    if (!x509) {
        BIO_free_all(bio);
        TlsSession::showError();
        return EE_ERROR;
    }
    if (!SSL_CTX_use_certificate(static_cast<SSL_CTX*>(mTlsContext), x509)) {
        DLOG(ELL_ERROR, "fail to use certificate");
        X509_free(x509);
        BIO_free_all(bio);
        TlsSession::showError();
        return EE_ERROR;
    }
    X509_free(x509);
    SSL_CTX_clear_extra_chain_certs(static_cast<SSL_CTX*>(mTlsContext)); // 清空额外证书
    s32 ret = 1;
    while ((x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) != nullptr) {
        if (!SSL_CTX_add_extra_chain_cert(static_cast<SSL_CTX*>(mTlsContext), x509)) {
            DLOG(ELL_ERROR, "fail to use sub certificate");
            X509_free(x509);
            break;
        }
        ++ret;
        X509_free(x509);
    }

    BIO_free_all(bio);
    DLOG(ELL_INFO, "LoadCert count = %d", ret);
    return EE_OK;
}


s32 TlsContext::setPrivateKey(const s8* key, usz length) {
    if (length > 0x7FFFFFFF) {
        return EE_INVALID_PARAM;
    }
    BIO* pkeybio = BIO_new_mem_buf(key, (s32)length);
    if (nullptr == pkeybio) {
        TlsSession::showError();
        return EE_ERROR;
    }
    EVP_PKEY* evpkey = PEM_read_bio_PrivateKey(pkeybio, nullptr, nullptr, nullptr);
    BIO_free_all(pkeybio);
    if (nullptr == evpkey) {
        TlsSession::showError();
        DLOG(ELL_ERROR, "fail to read pkey");
        return EE_ERROR;
    }
    if (!SSL_CTX_use_PrivateKey(static_cast<SSL_CTX*>(mTlsContext), evpkey)) {
        DLOG(ELL_ERROR, "fail to use pkey");
        TlsSession::showError();
        EVP_PKEY_free(evpkey);
        return EE_ERROR;
    }
    EVP_PKEY_free(evpkey);
    return EE_OK;
}

} // namespace net
} // namespace app
