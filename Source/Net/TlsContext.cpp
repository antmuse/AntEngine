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


static X509* AppLoadCert(const s8* cert, usz length) {
    if (length > INT_MAX) {
        return nullptr;
    }

    BIO* bio = BIO_new_mem_buf(cert, (s32)length);
    if (bio == nullptr) {
        return nullptr;
    }

    X509* x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free_all(bio);
    return x509;
}


static EVP_PKEY* AppLoadKey(const s8* key, usz length) {
    if (length > INT_MAX) {
        return nullptr;
    }
    BIO* bio = BIO_new_mem_buf(key, (s32)length);
    if (bio == nullptr) {
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free_all(bio);
    return pkey;
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
    SSL_CTX_set_ecdh_auto(ssl_ctx, 1);
    setVerifyFlags(cfg.mTlsVerify, cfg.mTlsVerifyDepth);
    if (EE_OK != setVersion(cfg.mTlsVersionOff)) {
        DLOG(ELL_ERROR, "set version off err = %s", cfg.mTlsVersionOff.data());
    }
    s32 ret = addTrustedCerts(G_CA_CERT, strlen(G_CA_CERT));
    ret = addTrustedCerts(G_CA_CERT1, strlen(G_CA_CERT1));
    ret = addTrustedCerts(G_CA_ROOT_CERT, strlen(G_CA_ROOT_CERT));
    Packet buf(1024);
    FileRWriter keyfile;
    if (!cfg.mTlsPassword.empty()) { // TODO make safe pass loader
        SSL_CTX_set_default_passwd_cb((SSL_CTX*)mTlsContext, AppTlsPasswordCallback);
        SSL_CTX_set_default_passwd_cb_userdata((SSL_CTX*)mTlsContext, (void*)(&cfg.mTlsPassword));
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
        if (!SSL_CTX_set_cipher_list((SSL_CTX*)mTlsContext, cfg.mTlsCiphers.data())) {
            DLOG(ELL_ERROR, "fail to set ciphers = %s", cfg.mTlsCiphers.data());
        } else {
            DLOG(ELL_INFO, "set ciphers = %s", cfg.mTlsCiphers.data());
        }
    }
    if (!cfg.mTlsCiphersuites.empty()) {
#ifdef TLS1_3_VERSION
        if (!SSL_CTX_set_ciphersuites((SSL_CTX*)mTlsContext, cfg.mTlsCiphersuites.data())) {
            DLOG(ELL_ERROR, "fail to set ciphersuites = %s", cfg.mTlsCiphersuites.data());
        } else {
            DLOG(ELL_INFO, "set ciphersuites = %s", cfg.mTlsCiphersuites.data());
        }
#else
        DLOG(ELL_WARN, "TLS v1.3 is not support, on ciphersuites set");
#endif
    }
    return EE_OK;
}

void TlsContext::uninit() {
    if (mTlsContext) {
        SSL_CTX_free((SSL_CTX*)mTlsContext);
        mTlsContext = nullptr;
    }
}


s32 TlsContext::setVersion(const String& it) {
    if (!mTlsContext) {
        DLOG(ELL_ERROR, "invalid TLS Context");
        return EE_ERROR;
    }
    String logs("disable TLS Version:");
    if (it.find("v1.0") >= 0) {
        logs += " v1.0";
        SSL_CTX_set_options((SSL_CTX*)mTlsContext, SSL_OP_NO_TLSv1);
    }
    if (it.find("v1.1") >= 0) {
        logs += " v1.1";
        SSL_CTX_set_options((SSL_CTX*)mTlsContext, SSL_OP_NO_TLSv1_1);
    }
    if (it.find("v1.2") >= 0) {
        logs += " v1.2";
        SSL_CTX_set_options((SSL_CTX*)mTlsContext, SSL_OP_NO_TLSv1_2);
    }
    if (it.find("v1.3") >= 0) {
#ifdef SSL_OP_NO_TLSv1_3
        logs += " v1.3";
        SSL_CTX_set_options((SSL_CTX*)mTlsContext, SSL_OP_NO_TLSv1_3);
#else
        DLOG(ELL_WARN, "TLS v1.3 is not support, on version set");
#endif
    }
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
    SSL_CTX_set_verify_depth((SSL_CTX*)mTlsContext, depth);
    SSL_CTX_set_verify((SSL_CTX*)mTlsContext, val, nullptr);
    return EE_OK;
}

s32 TlsContext::addTrustedCerts(const s8* cert, usz length) {
    s32 ncerts = 0;
    X509* x509;
    X509_STORE* trusted_store = SSL_CTX_get_cert_store((SSL_CTX*)mTlsContext);

    BIO* bio = BIO_new_mem_buf(cert, (s32)length);
    if (bio == nullptr) {
        TlsSession::showError();
        return EE_ERROR;
    }

    while ((x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) != nullptr) {
        X509_STORE_add_cert(trusted_store, x509);
        X509_free(x509);
        ncerts++;
    }

    BIO_free_all(bio);

    return ncerts == 0 ? EE_ERROR : 0;
}

s32 TlsContext::setCert(const s8* cert, usz length) {
    X509* x509 = AppLoadCert(cert, length);
    if (x509 == nullptr) {
        TlsSession::showError();
        return EE_ERROR;
    }

    SSL_CTX_use_certificate((SSL_CTX*)mTlsContext, x509);
    X509_free(x509);
    return 0;
}

s32 TlsContext::setPrivateKey(const s8* key, usz length) {
    EVP_PKEY* pkey = AppLoadKey(key, length);
    if (pkey == nullptr) {
        TlsSession::showError();
        return EE_ERROR;
    }

    SSL_CTX_use_PrivateKey((SSL_CTX*)mTlsContext, pkey);
    EVP_PKEY_free(pkey);
    return 0;
}

} // namespace net
} // namespace app
