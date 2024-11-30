#include "Net/HTTP/StationError.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {
namespace net {


s32 StationError::onMsg(HttpMsg* msg) {
    DASSERT(msg);
    HttpHead& hed = msg->getHeadOut();
    StringView key("Transfer-Encoding", sizeof("Transfer-Encoding") - 1);
    hed.remove(key);

    // msg->getHeadOut().clear();
    msg->getCacheOut().reset();
    static const s8* ebody = "<html>\n"
                             "<head>\n"
                             u8"<title>ERROR</title>\n"
                             "</head>\n"
                             "<body>\n"
                             u8"<hr><br>*_*<br><hr>"
                             "</body>\n"
                             "</html>";
    static const usz esz = strlen(ebody);

    s8 tmp[128];
    msg->writeStatus(msg->getStatus(), "ERR");
    msg->dumpHeadOut();
    msg->writeOutBody(tmp, snprintf(tmp, sizeof(tmp), "Content-Length:%llu\r\n\r\n", esz));
    msg->writeOutBody(ebody, esz);
    msg->setStationID(ES_RESP_BODY_DONE);

    return msg->getHttpLayer()->sendResp(msg);
}

} // namespace net
} // namespace app
