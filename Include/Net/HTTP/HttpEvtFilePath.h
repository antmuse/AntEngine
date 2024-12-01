#pragma once

#include "System.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {

class HttpEvtFilePath : public net::HttpEventer {
public:
    HttpEvtFilePath();
    virtual ~HttpEvtFilePath();

    virtual s32 onSent(net::HttpMsg* req) override;
    virtual s32 onFinish(net::HttpMsg* resp) override;
    virtual s32 onBodyPart(net::HttpMsg* resp) override;
    virtual s32 onOpen(net::HttpMsg* msg) override;
    virtual s32 onClose() override;

private:
    void writeChunk();
    void writeStr(const s8* it);
    void postResp();
    SRingBufPos mChunkPos;
    RingBuffer* mBody;
    net::HttpMsg* mMsg;
    usz mOffset;
    TVector<FileInfo> mList; 
};

} // namespace app
