#ifndef APP_LINKERUDP2_H
#define APP_LINKERUDP2_H

#include "Engine.h"
#include "RefCount.h"
#include "TMap.h"
#include "Packet.h"
#include "BinaryHeap.h"
#include "Net/HandleUDP.h"
#include "Net/KCProtocal.h"

namespace app {
namespace net {

class NetServerUDP2;

class LinkerUDP2 : public RefCount {
public:
    LinkerUDP2();

    ~LinkerUDP2();

    s32 onLink(u32 id, NetServerUDP2* sev, RequestUDP* sit);
    
    void onWrite(RequestUDP* it);

    void onRead(RequestUDP* it);

    s32 onTimeout(u32 val);

    u32 getID() const {
        return mProto.getID();
    }

    const net::NetAddress& getRemote()const{
        return mRemote;
    }

    Node3& getTimeNode() {
        return mNode;
    }

    const net::KCProtocal& getProto() const {
        return mProto;
    }

    static bool lessTime(const Node3* va, const Node3* vb);
    static LinkerUDP2* convNode(const Node3* val);

private:
    s32 sendMsg(const void* buf, s32 len);

    void onClose(Handle* it);

    static s32 sendKcpRaw(const void* buf, s32 len, void* user);
    s32 sendRawMsg(const void* buf, s32 len);

    static s32 funcOnTime(HandleTime* it) {
        //LinkerUDP2& nd = *(LinkerUDP2*)it->getUser();
        //return nd.onTimeout(*it);
        return 0;
    }
    static void funcOnClose(Handle* it) {
        LinkerUDP2& nd = *(LinkerUDP2*)it->getUser();
        nd.onClose(it);
    }

    static u32 mSN;
    u32 mTimeExt;
    Node3 mNode;
    NetServerUDP2* mSev;
    net::NetAddress mRemote;
    Loop* mLoop;
    MemPool* mMemPool;
    net::KCProtocal mProto;
    Packet mPack;
};


class NetServerUDP2 : public RefCount {
public:
    NetServerUDP2(bool tls);
    virtual ~NetServerUDP2();

    s32 open(const String& addr);

    bool isTLS() const {
        return mTLS;
    }

    void bind(net::LinkerUDP2* it) const {
        grab();
        it->grab();
    }

    void unbind(net::LinkerUDP2* it) const {
        drop();
        it->drop();
    }

    const net::HandleUDP& getHandleUDP() const {
        return mUDP;
    }

    MemPool* getMemPool() const {
        return mMemPool;
    }

    void closeNode(LinkerUDP2* nd);

    s32 sendRawMsg(LinkerUDP2& nd, const void* buf, s32 len);

private:
    s32 onTimeout(HandleTime& it);
    void onClose(Handle* it);
    void onRead(RequestUDP* it);
    void onWrite(RequestUDP* it);

    static void funcOnRead(RequestFD* it) {
        DASSERT(it && it->mUser);
        NetServerUDP2& nd = *reinterpret_cast<NetServerUDP2*>(it->getUser());
        nd.onRead(reinterpret_cast<RequestUDP*>(it));
    }
    static void funcOnClose(Handle* it) {
        NetServerUDP2& nd = *reinterpret_cast<NetServerUDP2*>(it->getUser());
        nd.onClose(it);
    }
    static s32 funcOnTime(HandleTime* it) {
        NetServerUDP2& nd = *reinterpret_cast<NetServerUDP2*>(it->getUser());
        return nd.onTimeout(*it);
    }
    static void funcOnWrite(RequestFD* it) {
        NetServerUDP2& nd = *(NetServerUDP2*)it->mUser;
        nd.onWrite(reinterpret_cast<RequestUDP*>(it));
    }
    bool mTLS;
    u32 mSN;
    net::HandleUDP mUDP;
    MemPool* mMemPool;
    TMap<u32, LinkerUDP2*> mClients;
    BinaryHeap mTimeHub;    //最小堆用于管理超时事件
};


} // namespace net
} // namespace app


#endif // APP_LINKERUDP2_H
