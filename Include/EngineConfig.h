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


#ifndef APP_ENGINECONFIG_H
#define	APP_ENGINECONFIG_H

#include "Strings.h"
#include "TVector.h"
#include "HashDict.h"
#include "Net/NetAddress.h"

namespace app {


class EngineConfig {
public:
    EngineConfig();

    ~EngineConfig();

    bool load(const String& runPath, const String& cfg, bool mainProcess);

    bool save(const String& cfg);

    bool mDaemon;
    u8 mPrint;
    u8 mMaxPostAccept;
    u8 mMaxThread;
    s16 mMaxProcess;
    u64 mMemSize;
    String mLogPath;
    String mPidFile;
    String mMemName;
    struct WebsiteCfg {
        u8 mType;               //0=http, 1=https
        u32 mTimeout;           //in milliseconds
        u32 mSpeed;             //in bytes per seconds
        String mRootPath;
        String mPathTLS;
        net::NetAddress mLocal;
        HashDict* mDict;         //cache of site
        WebsiteCfg() :
            mType(0),
            mTimeout(20*1000),
            mSpeed(1024 * 4),
            mDict(nullptr) {
        }
        ~WebsiteCfg() {
            if (mDict) {
                delete mDict;
                mDict = nullptr;
            }
        }
    };
    struct ProxyCfg {
        u8 mType;    //0=[tcp-tcp], 1=[tls-tcp], 2=[tcp-tls], 3=[tls-tls]
        u32 mTimeout; //in milliseconds
        u32 mSpeed;  //in bytes per seconds
        net::NetAddress mLocal;
        net::NetAddress mRemote;
    };
    TVector<WebsiteCfg> mWebsite;
    TVector<ProxyCfg> mProxy;
};


} //namespace app

#endif //APP_ENGINECONFIG_H
