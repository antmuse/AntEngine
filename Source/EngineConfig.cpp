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


#include "EngineConfig.h"
#include "HashFunctions.h"
#include "Logger.h"
#include "System.h"
#include "Timer.h"
#include "FileWriter.h"
#include "FileReader.h"
#include "json/json.h"

namespace app {

static u64 hashCallback(const void* key) {
    const String& str = *reinterpret_cast<const String*>(key);
    return AppHashSIP(str.c_str(), str.getLen());
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

static DictFunctions gDictCalls = {
    hashCallback,
    keyValueDump,
    keyValueDump,
    compareCallback,
    freeCallback,
    freeCallback
};



EngineConfig::EngineConfig() :
    mDaemon(false),
    mPrint(0),
    mMaxPostAccept(10),
    mMaxThread(3),
    mMaxProcess(0),
    mMemSize(1024 * 1024 * 1),
    mLogPath("Log/"),
    mPidFile("Log/PID.txt"),
    mMemName("GMAP/MainMem.map") {
    //memset(this, 0, sizeof(*this));

    s8 sed[20];
    Timer::getTimeStr(sed, sizeof(sed));
    AppSetHashSeedSIP(sed);
}

EngineConfig::~EngineConfig() {

}

bool EngineConfig::save(const String& cfg) {
    System::createPath(cfg);
    FileWriter file;
    if (!file.openFile(cfg, false)) {
        return false;
    }

    Json::Value val;
    val["Daemon"] = mDaemon;
    val["Print"] = mPrint;
    val["LogPath"] = mLogPath.c_str();
    val["PidFile"] = mPidFile.c_str();
    val["ShareMem"] = mMemName.c_str();
    val["ShareMemSize"] = (Json::Value::Int64)mMemSize / (1024 * 1024);
    val["AcceptPost"] = mMaxPostAccept;
    val["ThreadPool"] = mMaxThread;
    val["Process"] = mMaxProcess;

    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true;
    builder["indentation"] = "  ";
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream ostr;
    if (0 != writer->write(val, &ostr)) {
        return false;
    }
    std::string buf(ostr.str());
    return buf.size() == file.write(buf.c_str(), buf.size());
}


bool EngineConfig::load(const String& runPath, const String& cfg, bool mainProcess) {
    mLogPath = runPath + mLogPath;
    mPidFile = runPath + mPidFile;
    mMemName = runPath + mMemName;

    FileReader file;
    if (!file.openFile(cfg)) {
        return false;
    }
    s8* tmp = new s8[file.getFileSize()];
    if (file.getFileSize() != file.read(tmp, file.getFileSize())) {
        delete[] tmp;
        return false;
    }

    Json::Value val;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    Json::CharReader* reder = builder.newCharReader();
    bool ret = reder->parse(tmp, tmp + file.getFileSize(), &val, &errs);
    delete[] tmp;
    delete reder;
    if (ret) {
        mDaemon = val["Daemon"].asBool();
        mPrint = val["Print"].asInt();
        mLogPath = val["LogPath"].asCString();
        mLogPath.replace('\\', '/');
        if ('/' != mLogPath.lastChar()) {
            mLogPath += '/';
        }
        mPidFile = val["PidFile"].asCString();
        mMemName = val["ShareMem"].asCString();
        mMemSize = 1024 * 1024 * AppClamp<s64>(val["ShareMemSize"].asInt64(), 1LL, 10LL * 1024);
        mMaxPostAccept = AppClamp<u8>(val["AcceptPost"].asInt(), 1, 255);
        mMaxThread = AppClamp<u8>(val["ThreadPool"].asInt(), 1, 255);
        mMaxProcess = AppClamp<s16>(val["Process"].asInt(), -1024, 1024);

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
                if ('/' == nd.mRootPath.lastChar()) {
                    nd.mRootPath.setLen(nd.mRootPath.getLen() - 1);
                }
                if (1 == nd.mType) {
                    if (!val["Website"][i].isMember("PathTLS")) {
                        Logger::logError("EngineConfig::load, website[%u][%s] PathTLS err",
                            i, nd.mLocal.getStr());
                        continue;
                    }
                    nd.mPathTLS = val["Website"][i]["PathTLS"].asCString();
                    nd.mPathTLS.replace('\\', '/');
                    if ('/' != mLogPath.lastChar()) {
                        mLogPath += '/';
                    }
                }
                mWebsite.pushBack(nd);
                mWebsite.getLast().mDict = new HashDict(gDictCalls, nullptr);
            }
        }
    }
    return ret;
}


} //namespace app
