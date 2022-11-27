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

#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
#error "openssl version is too low"
#endif



namespace app {
namespace net {

//in file Net/TlsSession.cpp
extern void AppInitRingBIO();
extern void AppUninitRingBIO();


static Spinlock G_TLS_LOCK;

//call once
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

//call once
void AppInitTlsLib() {
    if (!G_TLS_LOCK.tryLock()) {
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    //atexit(AppUninitTlsLib);
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


TlsContext::TlsContext() :
    mTlsContext(nullptr),
    mVerifyFlags(ETLS_VERIFY_NONE) {
}

TlsContext::~TlsContext() {
}

s32 TlsContext::init(s32 vflag, bool debug) {
    if (mTlsContext) {
        return 0;
    }
    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_method());
    if (!ssl_ctx) {
        return EE_ERROR;
    }

    mTlsContext = ssl_ctx;
    mVerifyFlags = vflag;

    if (debug) {
        SSL_CTX_set_info_callback(ssl_ctx, AppFunTlsDebug);
    }

    SSL_CTX_set_ecdh_auto(ssl_ctx, 1);
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);

    s32 ret = addTrustedCerts(G_CA_CERT, strlen(G_CA_CERT));
    ret = addTrustedCerts(G_CA_CERT1, strlen(G_CA_CERT1));
    ret = addTrustedCerts(G_CA_ROOT_CERT, strlen(G_CA_ROOT_CERT));
    ret = setCert(G_SERVER_CERT, strlen(G_SERVER_CERT));
    ret = setPrivateKey(G_SERVER_KEY, strlen(G_SERVER_KEY));
    return 0;
}

void TlsContext::uninit() {
    if (mTlsContext) {
        SSL_CTX_free((SSL_CTX*)mTlsContext);
        mTlsContext = nullptr;
    }
}

void TlsContext::setVerifyFlags(s32 iflags) {
    mVerifyFlags = iflags;
}

s32 TlsContext::addTrustedCerts(const s8* cert, usz length) {
    s32 ncerts = 0;
    X509* x509;
    X509_STORE* trusted_store = SSL_CTX_get_cert_store((SSL_CTX*)mTlsContext);

    BIO* bio = BIO_new_mem_buf(cert, (s32)length);
    if (bio == nullptr) {
        return EE_ERROR;
    }

    while ((x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr)) !=
        nullptr) {
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
        return EE_ERROR;
    }

    SSL_CTX_use_certificate((SSL_CTX*)mTlsContext, x509);
    X509_free(x509);
    return 0;
}

s32 TlsContext::setPrivateKey(const s8* key, usz length) {
    EVP_PKEY* pkey = AppLoadKey(key, length);
    if (pkey == nullptr) {
        return EE_ERROR;
    }

    SSL_CTX_use_PrivateKey((SSL_CTX*)mTlsContext, pkey);
    EVP_PKEY_free(pkey);
    return 0;
}


}//namespace net
}//namespace app
