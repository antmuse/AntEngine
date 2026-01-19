#pragma once

#include "EngineConfig.h"

namespace app {


class ServerConfig {
public:
    ServerConfig();

    ~ServerConfig();

    bool load(const String& runPath, const String& cfg, bool mainProcess);

    TVector<WebsiteCfg> mWebsite;
    TVector<ProxyCfg> mProxy;
};


} // namespace app
