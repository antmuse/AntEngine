#pragma once
#include "Net/HTTP/WebSite.h"

namespace app {
namespace net {

class ServerWeb : public Website {
public:
    ServerWeb(WebsiteCfg& cfg);
    virtual ~ServerWeb();

    virtual s32 createMsgEvent(HttpMsg* msg);

private:
};

} // namespace net
} // namespace app
