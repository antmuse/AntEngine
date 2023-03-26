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


#include "Net/Socket.h"

#if defined(DOS_WINDOWS)
#include <winsock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <afunix.h>
#include "Windows/Request.h"
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/sockios.h>
#endif  //DOS_WINDOWS
#include "System.h"


namespace app {
namespace net {

enum EShutFlag {
    ESHUT_RECEIVE,
    ESHUT_SEND,
    ESHUT_BOTH
};

Socket::Socket(netsocket sock) :
    mSocket(sock) {
    DASSERT(sizeof(socklen_t) == sizeof(u32));
}


Socket::Socket() :
    mSocket(DINVALID_SOCKET) {
#if defined(DOS_WINDOWS)
    DASSERT(INVALID_SOCKET == DINVALID_SOCKET);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    //DASSERT(-1 == DINVALID_SOCKET);
#endif
}


Socket::~Socket() {
}


bool Socket::close() {
    if (DINVALID_SOCKET == mSocket) {
        return false;
    }

#if defined(DOS_WINDOWS)
    if (::closesocket(mSocket)) {
        return false;
    }
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    if (::close(mSocket)) {
        return false;
    }
#endif  //DOS_WINDOWS

    mSocket = DINVALID_SOCKET;
    return true;
}


bool Socket::closeReceive() {
    return 0 == ::shutdown(mSocket, ESHUT_RECEIVE);
}

bool Socket::closeSend() {
    return 0 == ::shutdown(mSocket, ESHUT_SEND);
}

bool Socket::closeBoth() {
    return 0 == ::shutdown(mSocket, ESHUT_BOTH);
}


bool Socket::isOpen()const {
    return DINVALID_SOCKET != mSocket;
}

s32 Socket::getError()const {
    s32 err = 0;
    socklen_t len = sizeof(err);
    if (0 != getsockopt(mSocket, SOL_SOCKET, SO_ERROR, (s8*)&err, &len)) {
        err = -1;
    }
    return err;
}

void Socket::getLocalAddress(NetAddress& it) {
    socklen_t len = it.getAddrSize();
    ::getsockname(mSocket, (struct sockaddr*)it.getAddress6(), &len);
    it.reverse();
}


void Socket::getRemoteAddress(NetAddress& it) {
    socklen_t len = it.getAddrSize();
    ::getpeername(mSocket, (struct sockaddr*)it.getAddress6(), &len);
    it.reverse();
}


s32 Socket::setReceiveOvertime(u32 it) {
#if defined(DOS_WINDOWS)
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (s8*)&it, sizeof(it));
#else
    struct timeval time;
    time.tv_sec = it / 1000;                     //seconds
    time.tv_usec = 1000 * (it % 1000);         //microseconds
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (s8*)&time, sizeof(time));
#endif
}


s32 Socket::setSendOvertime(u32 it) {
#if defined(DOS_WINDOWS)
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (s8*)&it, sizeof(it));
#else
    struct timeval time;
    time.tv_sec = it / 1000;                     //seconds
    time.tv_usec = 1000 * (it % 1000);           //microseconds
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, (s8*)&time, sizeof(time));
#endif
}


s32 Socket::setCustomIPHead(bool on) {
    s32 opt = on ? 1 : 0;
    return ::setsockopt(mSocket, IPPROTO_IP, IP_HDRINCL, (s8*)&opt, sizeof(opt));
}


s32 Socket::setReceiveAll(bool on) {
#if defined(DOS_WINDOWS)
    u32 dwBufferLen[10];
    u32 dwBufferInLen = on ? RCVALL_ON : RCVALL_OFF;
    DWORD dwBytesReturned = 0;
    //::ioctlsocket(mScoketListener.getValue(), SIO_RCVALL, &dwBufferInLen);
    return (::WSAIoctl(mSocket, SIO_RCVALL,
        &dwBufferInLen, sizeof(dwBufferInLen),
        &dwBufferLen, sizeof(dwBufferLen),
        &dwBytesReturned, 0, 0));
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    ifreq iface;
    const char* device = "eth0";
    DASSERT(IFNAMSIZ > ::strlen(device));

    s32 setopt_result = ::setsockopt(mSocket, SOL_SOCKET,
        SO_BINDTODEVICE, device, ::strlen(device));

    ::strcpy(iface.ifr_name, device);
    if (::ioctl(mSocket, SIOCGIFFLAGS, &iface) < 0) {
        return -1;
    }
    if (!(iface.ifr_flags & IFF_RUNNING)) {
        //printf("eth link down\n");
        return -1;
    }
    if (on) {
        iface.ifr_flags |= IFF_PROMISC;
    } else {
        iface.ifr_flags &= ~IFF_PROMISC;
    }
    return ::ioctl(mSocket, SIOCSIFFLAGS, &iface);
#endif
}


s32 Socket::keepAlive(bool on, s32 idleTime, s32 timeInterval, s32 maxTick) {
#if defined(DOS_WINDOWS)
    //struct tcp_keepalive {
    //    ULONG onoff;   // 是否开启 keepalive
    //    ULONG keepalivetime;  // 多长时间（ ms ）没有数据就开始 send 心跳包
    //    ULONG keepaliveinterval; // 每隔多长时间（ ms ） send 一个心跳包，
    //   // 发 5 次 (2000 XP 2003 默认 ), 10 次 (Vista 后系统默认 )
    //};
    tcp_keepalive alive;
    DWORD rbytes;
    alive.onoff = on ? TRUE : FALSE;
    alive.keepalivetime = 1000 * idleTime;
    alive.keepaliveinterval = 1000 * timeInterval;
    return ::WSAIoctl(mSocket, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), 0, 0, &rbytes, 0, 0);

#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    s32 opt = on ? 1 : 0;
    s32 ret;
    //SOL_TCP
    ret = ::setsockopt(mSocket, SOL_SOCKET, SO_KEEPALIVE, (s8*)&opt, sizeof(opt));
    if (!on || ret) return ret;

    ret = ::setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idleTime, sizeof(idleTime));
    if (ret) return ret;

    ret = ::setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPINTVL, &timeInterval, sizeof(timeInterval));
    if (ret) return ret;

    ret = ::setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPCNT, &maxTick, sizeof(maxTick));

    return ret;
#endif
}


s32 Socket::setLinger(bool it, u16 iTime) {
    struct linger val;
    val.l_onoff = it ? 1 : 0;
    val.l_linger = iTime;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (s8*)&val, sizeof(val));
}


s32 Socket::setBroadcast(bool it) {
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (s8*)&opt, sizeof(opt));
}


s32 Socket::setBlock(bool it) {
#if defined(DOS_WINDOWS)
    u_long ul = it ? 0 : 1;
    return ::ioctlsocket(mSocket, FIONBIO, &ul);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    s32 opts = ::fcntl(mSocket, F_GETFL);
    if (opts < 0) {
        return -1;
    }
    opts = it ? (opts & (~O_NONBLOCK)) : (opts | O_NONBLOCK);
    opts = ::fcntl(mSocket, F_SETFL, opts);
    return -1 == opts ? opts : 0;
#endif
}


s32 Socket::setReuseIP(bool it) {
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (s8*)&opt, sizeof(opt));
}


s32 Socket::setReusePort(bool it) {
#if defined(DOS_WINDOWS)
    return 0;
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    s32 opt = it ? 1 : 0;
    return ::setsockopt(mSocket, SOL_SOCKET, SO_REUSEPORT, (s8*)&opt, sizeof(opt));
#endif
}


s32 Socket::setSendCache(s32 size) {
    return ::setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (s8*)&size, sizeof(size));
}


s32 Socket::setReceiveCache(s32 size) {
    return ::setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (s8*)&size, sizeof(size));
}


s32 Socket::bind(const NetAddress& it) {
    return ::bind(mSocket, (sockaddr*)it.getAddress6(), it.getAddrSize());
}


s32 Socket::bind(bool ipv6) {
    NetAddress it(ipv6);
    return ::bind(mSocket, (sockaddr*)it.getAddress6(), it.getAddrSize());
}

bool Socket::getTcpInfo(STCP_Info* info) const {
    //TODO>>
#if defined(DOS_WINDOWS)
    //https://docs.microsoft.com/zh-cn/windows/win32/api/iphlpapi/nf-iphlpapi-getpertcpconnectionestats?redirectedfrom=MSDN
    bool ret = false;
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    struct tcp_info raw;
    socklen_t len = sizeof(raw);
    ::memset(&raw, 0, len);
    bool ret = (0 == ::getsockopt(mSocket, SOL_SOCKET, TCP_INFO, &raw, &len));
#endif
    return ret;
}


s32 Socket::sendTo(const void* iBuffer, s32 iSize, const NetAddress& address) {
#if defined(DOS_WINDOWS)
    return ::sendto(mSocket, (const s8*)iBuffer, iSize, 0,
        (sockaddr*)address.getAddress6(), address.getAddrSize());
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return ::sendto(mSocket, iBuffer, iSize, 0,
        (sockaddr*)address.getAddress6(), address.getAddrSize());
#endif
}


s32 Socket::receiveFrom(void* iBuffer, s32 iSize, NetAddress& address) {
#if defined(DOS_WINDOWS)
    socklen_t size = address.getAddrSize();
    return ::recvfrom(mSocket, (s8*)iBuffer, iSize, 0, (sockaddr*)address.getAddress6(), &size);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    socklen_t size = address.getAddrSize();
    return ::recvfrom(mSocket, iBuffer, iSize, 0, (sockaddr*)address.getAddress6(), &size);
#endif
}


bool Socket::openRaw(s32 protocol, bool ipv6) {
    return open(ipv6 ? AF_INET6 : AF_INET, SOCK_RAW, protocol);
}


bool Socket::openUDP(bool ipv6) {
    return open(ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}


bool Socket::openTCP(bool ipv6) {
    return open(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
}


bool Socket::open(s32 domain, s32 type, s32 protocol) {
    if (DINVALID_SOCKET != mSocket) {
        return false;
    }

    mSocket = ::socket(domain, type, protocol);
    return DINVALID_SOCKET != mSocket;
}



s32 Socket::setDelay(bool it) {
    s32 opt = it ? 0 : 1;
    return ::setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (s8*)&opt, sizeof(opt));
}


s32 Socket::connect(const NetAddress& it) {
    return ::connect(mSocket, (sockaddr*)it.getAddress6(), it.getAddrSize());
}


s32 Socket::listen(s32 backlog) {
    backlog = AppClamp(backlog, 1, SOMAXCONN);
#if defined(DOS_WINDOWS)
    //SOMAXCONN_HINT for win only, range[200-65535]
    //https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
    return ::listen(mSocket, SOMAXCONN_HINT(backlog));
#else
    //SOMAXCONN, pls set /proc/sys/net/core/somaxconn
    return ::listen(mSocket, backlog);
#endif
}


bool Socket::isAlive() {
    return (0 == send("", 0));
}


s32 Socket::sendAll(const void* iBuffer, s32 iSize) {
    s32 ret = 0;
    s32 step;
    for (; ret < iSize;) {
        step = send(((const s8*)iBuffer) + ret, iSize - ret);
        if (step > 0) {
            ret += step;
        } else {
            return step;
        }
    }
    return ret;
}


s32 Socket::send(const void* iBuffer, s32 iSize) {
#if defined(DOS_WINDOWS)
    return ::send(mSocket, (const s8*)iBuffer, iSize, 0);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return ::send(mSocket, iBuffer, iSize, MSG_NOSIGNAL);
#endif
}


s32 Socket::receiveAll(void* iBuffer, s32 iSize) {
    s32 ret = 0;
    s32 step;
    for (; ret < iSize;) {
        step = ::recv(mSocket, ((s8*)iBuffer) + ret, iSize - ret, MSG_WAITALL);
        if (step > 0) {
            ret += step;
        } else {
            return step;
        }
    }
    return ret;
    //#if defined(DOS_WINDOWS)
    //    return ::recv(mSocket, (s8*) iBuffer, iSize, MSG_WAITALL);
    //#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    //    return ::recv(mSocket, iBuffer, iSize, MSG_WAITALL);
    //#endif
}


s32 Socket::receive(void* iBuffer, s32 iSize) {
#if defined(DOS_WINDOWS)
    return ::recv(mSocket, (s8*)iBuffer, iSize, 0);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return ::recv(mSocket, iBuffer, iSize, 0);
#endif
}


Socket Socket::accept() {
    return ::accept(mSocket, 0, 0);
}


Socket Socket::accept(NetAddress& it) {
    socklen_t size = it.getAddrSize();
    Socket ret = ::accept(mSocket, (sockaddr*)it.getAddress6(), &size);
    if (ret.isOpen()) {
        it.reverse();
    }
    return ret;
}


#if defined(DOS_WINDOWS)
void* Socket::mFunctionConnect = nullptr;
void* Socket::mFunctionDisconnect = nullptr;
void* Socket::mFunctionAccept = nullptr;
void* Socket::mFunctionAcceptSockAddress = nullptr;

static void* mFunctionAcceptSockAddress;
s32 Socket::updateByAccepter(const Socket& sockListen) {
    netsocket it = sockListen.getValue();
    return setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (s8*)&it, sizeof(it));
}

s32 Socket::updateByConnect() {
    return setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
}

bool Socket::openSeniorTCP(bool ipv6) {
    return openSenior(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
}

bool Socket::openSeniorUDP(bool ipv6) {
    return openSenior(ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0, WSA_FLAG_OVERLAPPED);
}

bool Socket::openSenior(s32 domain, s32 type, s32 protocol, void* info, u32 group, u32 flag) {
    if (DINVALID_SOCKET != mSocket) {
        return true;
    }
    mSocket = ::WSASocket(domain, type, protocol, (LPWSAPROTOCOL_INFOW)info, group, flag);
    if (!mFunctionConnect && isOpen()) {
        mFunctionConnect = this->getFunctionConnect();
        mFunctionDisconnect = this->getFunctionDisconnect();
        mFunctionAccept = this->getFunctionAccpet();
        mFunctionAcceptSockAddress = this->getFunctionAcceptSockAddress();
    }
    return DINVALID_SOCKET != mSocket;
}


bool Socket::receive(RequestFD* iAction)const {
    //DWORD bytes = 0;
    DWORD flags = 0;
    StringView vvv = iAction->getWriteBuf();
    WSABUF buf = {(ULONG)vvv.mLen, vvv.mData};
    iAction->clearOverlap();
    s32 ret = ::WSARecv(mSocket,
        &buf,
        1,
        nullptr, //&bytes,
        &flags,
        &(iAction->mOverlapped),
        0);

    return 0 == ret ? true : (WSA_IO_PENDING == System::getError());
}

bool Socket::receiveFrom(RequestFD* iAction, NetAddress& addr)const {
    DWORD flags = 0;
    StringView vvv = iAction->getWriteBuf();
    WSABUF buf = {(ULONG)vvv.mLen, vvv.mData};
    iAction->clearOverlap();
    s32 len = addr.getAddrSize();
    s32 ret = ::WSARecvFrom(mSocket,
        &buf,
        1,
        nullptr, //&bytes,
        &flags,
        reinterpret_cast<sockaddr*>(addr.getAddress6()),
        &len,
        &(iAction->mOverlapped),
        0);

    return 0 == ret ? true : (WSA_IO_PENDING == System::getError());
}

bool Socket::send(RequestFD* iAction)const {
    //DWORD bytes = 0;
    //DWORD flags = 0;
    StringView vvv = iAction->getReadBuf();
    WSABUF buf = {(ULONG)vvv.mLen, vvv.mData};
    iAction->clearOverlap();
    s32 ret = ::WSASend(mSocket,
        &buf,
        1,
        nullptr,    //&bytes,
        0,          //flags,
        &(iAction->mOverlapped),
        0);

    return 0 == ret ? true : (WSA_IO_PENDING == System::getError());
}

bool Socket::sendTo(RequestFD* iAction, const NetAddress& addr)const {
    //DWORD bytes = 0;
    //DWORD flags = 0;
    StringView vvv = iAction->getReadBuf();
    WSABUF buf = {(ULONG)vvv.mLen, vvv.mData};
    iAction->clearOverlap();
    s32 ret = ::WSASendTo(mSocket,
        &buf,
        1,
        nullptr,    //&bytes,
        0,          //flags,
        reinterpret_cast<const sockaddr*>(addr.getAddress6()),
        addr.getAddrSize(),
        &(iAction->mOverlapped),
        0);

    return 0 == ret ? true : (WSA_IO_PENDING == System::getError());
}

bool Socket::connect(const NetAddress& it, RequestFD* iAction)const {
    iAction->clearOverlap();
    void* function = mFunctionConnect;
    LPFN_CONNECTEX iConnect = (LPFN_CONNECTEX)(function ? function : getFunctionConnect());
    if (function) {
        if (FALSE == iConnect(mSocket,
            (sockaddr*)it.getAddress6(),
            it.getAddrSize(),
            0,
            0,
            0,
            &iAction->mOverlapped)) {
            return (WSA_IO_PENDING == System::getError());
        }
        return true;
    }

    return false;
}


bool Socket::disconnect(RequestFD* iAction)const {
    iAction->clearOverlap();
    void* function = mFunctionDisconnect;
    LPFN_DISCONNECTEX iDisconnect = (LPFN_DISCONNECTEX)(function ? function : getFunctionDisconnect());
    if (iDisconnect) {
        if (FALSE == iDisconnect(mSocket, &iAction->mOverlapped, 0, 0)) {
            return (WSA_IO_PENDING == System::getError());
        }
        return true;
    }

    return false;
}


bool Socket::accept(const Socket& sock, RequestFD* iAction, void* addressCache, u32 addsize)const {
    iAction->clearOverlap();
    void* function = mFunctionAccept;
    LPFN_ACCEPTEX func = (LPFN_ACCEPTEX)(function ? function : getFunctionAccpet());
    if (func) {
        addsize >>= 1;
        DWORD bytes = 0;
        if (FALSE == func(mSocket,
            sock.getValue(),
            addressCache,
            0,
            addsize,
            addsize,
            &bytes,
            &iAction->mOverlapped)) {
            return (WSA_IO_PENDING == System::getError());
        }
        return true;
    }

    return false;
}


bool Socket::getAddress(void* addressCache, u32 addsize, NetAddress& local, NetAddress& remote)const {
    void* function = mFunctionAcceptSockAddress;
    LPFN_GETACCEPTEXSOCKADDRS func = (LPFN_GETACCEPTEXSOCKADDRS)(function ? function : getFunctionAcceptSockAddress());
    if (func) {
        const u32 addsize = local.getAddrSize() + 16;
        sockaddr_in6* addrLocal = nullptr;
        sockaddr_in6* addrRemote = nullptr;
        s32 localLen = local.getAddrSize();
        s32 remoteLen = local.getAddrSize();
        func(addressCache,
            0, //2*addsize
            addsize,
            addsize,
            (sockaddr**)&addrLocal,
            &localLen,
            (sockaddr**)&addrRemote,
            &remoteLen);

        if (local.isIPV6()) {
            local.setAddress6(*addrLocal);
            remote.setAddress6(*addrRemote);
        } else {
            local.setAddress4(*(sockaddr_in*)addrLocal);
            remote.setAddress4(*(sockaddr_in*)addrRemote);
        }
        return true;
    }

    return false;
}


void* Socket::getFunctionConnect()const {
    LPFN_CONNECTEX func = 0;
    GUID iGID = WSAID_CONNECTEX;
    DWORD retBytes = 0;

    if (0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* Socket::getFunctionDisconnect() const {
    LPFN_DISCONNECTEX func = 0;
    GUID iGID = WSAID_DISCONNECTEX;
    DWORD retBytes = 0;

    if (0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* Socket::getFunctionAccpet()const {
    LPFN_ACCEPTEX func = 0;
    GUID iGID = WSAID_ACCEPTEX;
    DWORD retBytes = 0;

    if (0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


void* Socket::getFunctionAcceptSockAddress()const {
    LPFN_GETACCEPTEXSOCKADDRS func = 0;
    GUID iGID = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD retBytes = 0;

    if (0 == ::WSAIoctl(
        mSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &iGID,
        sizeof(iGID),
        &func,
        sizeof(func),
        &retBytes,
        0,
        0)) {
        return func;
    }

    return 0;
}


#else

bool Socket::openSeniorTCP(bool ipv6) {
    bool ret = openTCP(ipv6);
    if (ret) {
        ret = (0 == setBlock(false));
        if (!ret) {
            close();
        }
    }
    return ret;
}

bool Socket::openSeniorUDP(bool ipv6) {
    bool ret = openUDP(ipv6);
    if (ret) {
        ret = (0 == setBlock(false));
        if (!ret) {
            close();
        }
    }
    return ret;
}

#endif//DOS_WINDOWS







SocketPair::SocketPair() {
}

SocketPair::~SocketPair() {
    //close();
}

bool SocketPair::close() {
    return mSockB.close() && mSockA.close();
}


bool SocketPair::open(const s8* unpath) {
    return open(AF_UNIX, SOCK_STREAM, 0, unpath);
}


#if defined(DOS_WINDOWS)
bool SocketPair::open(s32 domain, s32 type, s32 protocol, const s8* unpath) {
    SOCKET lisock = socket(domain, type, 0);
    if (INVALID_SOCKET == lisock) {
        return false;
    }

    SOCKET consock = INVALID_SOCKET;
    SOCKET acpsock = INVALID_SOCKET;
    struct sockaddr_un listen_addr;

    s32 ecode = -1;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sun_family = domain;
    snprintf(listen_addr.sun_path, sizeof(listen_addr.sun_path),
        "%s", unpath ? unpath : "");

    System::removeFile(listen_addr.sun_path);

    if (bind(lisock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        goto GT_FAIL;
    }

    if (listen(lisock, 1) == -1) {
        goto GT_FAIL;
    }

    consock = socket(domain, type, 0);
    if (INVALID_SOCKET == consock) {
        goto GT_FAIL;
    }

    if (connect(consock, (struct sockaddr*) & listen_addr, sizeof(listen_addr)) == -1) {
        goto GT_FAIL;
    }

    acpsock = accept(lisock, nullptr, nullptr);
    if (INVALID_SOCKET == acpsock) {
        goto GT_FAIL;
    }

    closesocket(lisock);
    System::removeFile(listen_addr.sun_path);

    mSockA = consock;
    mSockB = acpsock;
    //mSockA.setBlock(false);
    //mSockB.setBlock(false);
    return true;



    GT_FAIL:
    ecode = WSAGetLastError();

    if (INVALID_SOCKET == lisock) {
        closesocket(lisock);
    }
    if (INVALID_SOCKET == consock) {
        closesocket(consock);
    }
    if (INVALID_SOCKET == acpsock) {
        closesocket(acpsock);
    }
    System::removeFile(listen_addr.sun_path);
    return false;
}

#elif defined(DOS_LINUX) || defined(DOS_ANDROID)

bool SocketPair::open(s32 domain, s32 type, s32 protocol, const s8* unpath) {
    netsocket sockpair[2];
    if (0 != socketpair(domain, type, protocol, sockpair)) {
        //printf("error %d on socketpair\n", errno);
        return false;
    }
    mSockA = sockpair[0];
    mSockB = sockpair[1];

    //mSockA.setBlock(false);
    //mSockB.setBlock(false);
    return true;
}

#else
#error "OS NOT SUPPORTED!"
#endif //DOS_WINDOWS


} //namespace net
} //namespace app
