#include "ServerConfig.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "System.h"
#include "Timer.h"
#include "FileRWriter.h"
#include "json/json.h"
#include "Engine.h"

namespace app {

static u64 hashCallback(const void* key) {
    const String& str = *reinterpret_cast<const String*>(key);
    return AppHashSIP(str.c_str(), str.size());
}

static bool compareCallback(void* iUserData, const void* key1, const void* key2) {
    return (*reinterpret_cast<const String*>(key1)) == (*reinterpret_cast<const String*>(key2));
}

static void freeCallback(void* iUserData, void* val) {
    String* it = reinterpret_cast<String*>(val);
    delete it;
}

static void* keyValueDump(void* iUserData, const void* kval) {
    String* ret = new String(*reinterpret_cast<const String*>(kval));
    return ret;
}

static DictFunctions gDictCalls
    = {hashCallback, keyValueDump, keyValueDump, compareCallback, freeCallback, freeCallback};



ServerConfig::ServerConfig() {
}

ServerConfig::~ServerConfig() {
}


bool ServerConfig::load(const String& runPath, const String& cfg, bool mainProcess) {
    s8* tmp = cfg.data();
    usz len = cfg.size();
    if (cfg.isFileExtension("json")) {
        FileRWriter file;
        if (!file.openFile(cfg)) {
            return false;
        }
        len = file.getFileSize();
        tmp = new s8[len];
        if (file.getFileSize() != file.read(tmp, file.getFileSize())) {
            delete[] tmp;
            return false;
        }
    }
    Json::Value val;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    Json::CharReader* reder = builder.newCharReader();
    bool ret = reder->parse(tmp, tmp + len, &val, &errs);
    delete reder;
    if (tmp != cfg.data()) {
        delete[] tmp;
    }
    if (!ret) { // parse fail
        return false;
    }
    if (val.empty()) { // empty cfg
        return true;
    }

    // func for TlsConfig load
    auto func_loadtls = [](const Json::Value& val, TlsConfig& out) -> bool {
        if (val.isMember("TLS")) {
            const Json::Value& tls = val["TLS"];
            out.mTlsPathCA = tls["CA"].asCString();
            out.mTlsPathCert = tls["Cert"].asCString();
            out.mTlsPathKey = tls["Key"].asCString();
            out.mTlsVerify = tls["Verify"].asInt();
            out.mTlsVerifyDepth = tls["VerifyDepth"].asInt();
            out.mTlsVersionOff = tls["VersionOff"].asCString();
            out.mTlsCiphers = tls["Ciphers"].asCString();
            out.mTlsCiphersuites = tls["Ciphersuites"].asCString();
            if (tls.isMember("KeyPassword")) {
                out.mTlsPassword = tls["KeyPassword"].asCString();
            }
            if (tls.isMember("HttpALPN")) {
                out.mHttpALPN = AppClamp(tls["HttpALPN"].asInt(), 1, 3);
            }
            if (tls.isMember("PreferServerCiphers")) {
                out.mPreferServerCiphers = tls["PreferServerCiphers"].asBool();
            }
            if (tls.isMember("Debug")) {
                out.mDebug = tls["Debug"].asBool();
            }
            return true;
        }
        return false;
    };
    // TODO: try-catch below?
    if (val.isMember("Proxy")) {
        ProxyCfg nd;
        u32 mx = val["Proxy"].size();
        for (u32 i = 0; i < mx; ++i) {
            nd.mType = (u8)val["Proxy"][i]["Type"].asInt();
            nd.mSpeed = (u32)val["Proxy"][i]["MaxSpeed"].asInt();
            nd.mTimeout = 1000 * AppClamp<u32>(val["Proxy"][i]["Timeout"].asInt(), 0, 3600);
            nd.mLocal.setIPort(val["Proxy"][i]["Lisen"].asCString());
            nd.mRemote.setIPort(val["Proxy"][i]["Backend"].asCString());
            mProxy.pushBack(nd);
        }
    }
    if (val.isMember("Website")) {
        WebsiteCfg nd;
        u32 mx = val["Website"].size();
        for (u32 i = 0; i < mx; ++i) {
            nd.mType = (u8)val["Website"][i]["Type"].asInt();
            nd.mTimeout = 1000 * AppClamp<u32>(val["Website"][i]["Timeout"].asInt(), 0, 3600);
            nd.mLocal.setIPort(val["Website"][i]["Lisen"].asCString());
            nd.mRootPath = val["Website"][i]["Path"].asCString();
            nd.mRootPath.replace('\\', '/');
            nd.mHost = val["Website"][i]["Host"].asCString();
            if ('/' == nd.mRootPath.lastChar()) {
                nd.mRootPath.resize(nd.mRootPath.size() - 1);
            }
            if (1 == nd.mType) {
                if (!func_loadtls(val["Website"][i], nd.mTLS)) {
                    nd.mTLS = Engine::getInstance().getConfig().mEngTlsConfig;
                    DLOG(ELL_INFO, "ServerConfig::load, website[%u][%s] TLS use default", i, nd.mLocal.getStr());
                }
            }
            mWebsite.pushBack(nd);
            mWebsite.getLast().mDict = new HashDict(gDictCalls, nullptr);
        }
    }
    return ret;
}


} // namespace app
