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


#include "Net/NetAddress.h"
#include "Converter.h"
//#include <cstring>

#if defined(DOS_WINDOWS)
#include <winsock2.h>
#include <WS2tcpip.h>
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif  //DOS_WINDOWS

#define DNET_ANY_IP "0.0.0.0"
#define DNET_ANY_IP6 "[::]"
#define DNET_ANY_PORT 0


namespace app {
namespace net {
constexpr static u8 GIPV6_SIZE = 28;
constexpr static u8 GIPV4_SIZE = 16;

NetAddress::NetAddress(bool ipv6) {
    clear();
    snprintf(mName, sizeof(mName), "%s:0", ipv6 ? DNET_ANY_IP6 : DNET_ANY_IP);
    init(ipv6);
    initIP();
    initPort();
}


NetAddress::NetAddress(const s8* ipPort) {
    setIPort(ipPort);
}


NetAddress::NetAddress(const String& ipPort) {
    setIPort(ipPort.c_str());
}


NetAddress::NetAddress(u16 port, bool ipv6) {
    clear();
    mPort = port;
    snprintf(mName, sizeof(mName), "%s:%d", ipv6 ? DNET_ANY_IP6 : DNET_ANY_IP, port);
    init(ipv6);
    initIP();
    initPort();
}


NetAddress::NetAddress(const s8* ip, u16 port, bool ipv6) {
    clear();
    if(ip) {
        snprintf(mName, sizeof(mName), "%s:%d", ip, port);
    } else {
        snprintf(mName, sizeof(mName), "%s:%d", ipv6 ? DNET_ANY_IP6 : DNET_ANY_IP, port);
    }
    mPort = port;
    init(ipv6);
    initIP();
    initPort();
}

NetAddress::NetAddress(const String& ip, u16 port, bool ipv6) :
    NetAddress(ip.c_str(), port, ipv6) {
}

NetAddress::NetAddress(const NetAddress& it) {
    memcpy(this, &it, sizeof(it));
}


NetAddress::~NetAddress() {
}


void NetAddress::clear() {
    memset(this, 0, sizeof(*this));
}


DINLINE void NetAddress::init(bool ipv6) {
    if(ipv6) {
        mSize = sizeof(sockaddr_in6);
        getAddress6()->sin6_family = AF_INET6;
    } else {
        mSize = sizeof(sockaddr_in);
        getAddress4()->sin_family = AF_INET;
    }
}


DINLINE void NetAddress::initIP() {
    if(isIPV6()) {
        inet_pton(getAddress6()->sin6_family, mName, &(getAddress6()->sin6_addr));
    } else {
        inet_pton(getAddress4()->sin_family, mName, &(getAddress4()->sin_addr));
    }
}


DINLINE void NetAddress::initPort() {
    if(isIPV6()) {
        getAddress6()->sin6_port = htons(mPort);
    } else {
        getAddress4()->sin_port = htons(mPort);
    }
}


NetAddress& NetAddress::operator=(const NetAddress& it) {
    if(this != &it) {
        memcpy(this, &it, sizeof(it));
    }
    return *this;
}


bool NetAddress::operator==(const NetAddress& it) const {
    if(mSize == it.mSize && mPort == it.mPort) {
        return 0 == memcmp(mAddress, it.mAddress, mSize);
    }
    return false;
}


bool NetAddress::operator!=(const NetAddress& it) const {
    if(mSize == it.mSize && mPort == it.mPort) {
        return 0 != memcmp(mAddress, it.mAddress, mSize);
    }
    return true;
}


u16 NetAddress::getFamily()const {
    return (*(u16*)mAddress);
}


void NetAddress::setIP(const s8* ip, bool ipv6) {
    if(ip && '[' != ip[0]) {
        if(ipv6) {
            mSize = sizeof(sockaddr_in6);
            getAddress6()->sin6_family = AF_INET6;
            inet_pton(AF_INET6, ip, &(getAddress6()->sin6_addr));
            snprintf(mName, sizeof(mName), "[%s]:%d", ip, mPort);
        } else {
            mSize = sizeof(sockaddr_in);
            getAddress4()->sin_family = AF_INET;
            inet_pton(AF_INET, ip, &(getAddress4()->sin_addr));
            snprintf(mName, sizeof(mName), "%s:%d", ip, mPort);
        }
    }
}


bool NetAddress::setIP(bool ipv6) {
    clear();
    s8 host_name[256];
    if(::gethostname(host_name, sizeof(host_name) - 1) < 0) {
        return false;
    }
    struct addrinfo* head = 0;
    struct addrinfo hints;
    hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;    // IPPROTO_TCP; //0，默认
    hints.ai_flags = AI_PASSIVE;        //flags的标志很多,常用的有AI_CANONNAME;
    hints.ai_addrlen = 0;
    hints.ai_canonname = 0;
    hints.ai_addr = 0;
    hints.ai_next = 0;
    s32 ret = ::getaddrinfo(host_name, 0, &hints, &head);
    if(ret) {
        return false;
    }

    usz len;
    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
        if(AF_INET6 == curr->ai_family) {
            mSize = sizeof(sockaddr_in6);
            memcpy(mAddress, curr->ai_addr, curr->ai_addrlen);
            mName[0] = '[';
            inet_ntop(getAddress6()->sin6_family, &getAddress6()->sin6_addr, mName + 1, sizeof(mName) - 5);
            mPort = ntohs(getAddress6()->sin6_port);
            len = strlen(mName);
            mName[len++] = ']';
            mName[len++] = ':';
            mName[len] = '0';
            break;
        }
        if(AF_INET == curr->ai_family) {
            mSize = sizeof(sockaddr_in);
            memcpy(mAddress, curr->ai_addr, curr->ai_addrlen);
            inet_ntop(getAddress4()->sin_family, &getAddress4()->sin_addr, mName, sizeof(mName));
            mPort = ntohs(getAddress4()->sin_port);
            len = strlen(mName);
            mName[len++] = ':';
            mName[len] = '0';
            break;
        }
    } //for

    ::freeaddrinfo(head);
    return mSize > 0;
}


void NetAddress::setPort(u16 port) {
    if(port != mPort) {
        if(isIPV6()) {
            getAddress6()->sin6_port = htons(port);
        } else {
            getAddress4()->sin_port = htons(port);
        }
        reverse();
    }
}


u16 NetAddress::getPort()const {
    return mPort;
}


void NetAddress::set(const s8* ip, u16 port, bool ipv6) {
    if(ip && '\0' != ip[0]) {
        clear();
        init(ipv6);
        mPort = port;
        initPort();
        setIP(ip, ipv6);
    }
}

void NetAddress::setAddrSize(usz sz) {
    mSize = (u8)sz;
    if (sizeof(sockaddr_in6) == mSize) {
        getAddress6()->sin6_family = AF_INET6;
    } else {
        getAddress4()->sin_family = AF_INET;
    }
}

void NetAddress::setIPort(const s8* ipAndPort) {
    if(nullptr == ipAndPort) {
        return;
    }
    clear();
    memcpy(mName, ipAndPort, AppMin(sizeof(mName) - 1, strlen(ipAndPort)));
    s8* curr = mName;
    if('[' == curr[0]) {//ipv6
        mSize = sizeof(sockaddr_in6);
        getAddress6()->sin6_family = AF_INET6;
        while(*curr != ']' && *curr != '\0') {
            ++curr;
        }
        s8 bk = curr[0];
        curr[0] = 0;
        inet_pton(AF_INET6, mName + 1, &(getAddress6()->sin6_addr));
        curr[0] = bk;
        if(curr[0] == ']' && curr[1] == ':') {
            mPort = (u16)App10StrToU32(curr + 2);
            initPort();
        } else {
            DASSERT(0);
            return;
        }
    } else {
        mSize = sizeof(sockaddr_in);
        getAddress4()->sin_family = AF_INET;
        while (*curr != ':' && *curr != '\0') {
            ++curr;
        }
        s8 bk = curr[0];
        curr[0] = 0;
        inet_pton(AF_INET, mName, &(getAddress4()->sin_addr));
        curr[0] = bk;
        if(curr[0] == ':') {
            mPort = (u16)App10StrToU32(++curr);
            initPort();
        } else {
            DASSERT(0);
            return;
        }
    }
}


void NetAddress::setDomain(const s8* iDNS, bool ipv6) {
    if(!iDNS) {
        return;
    }

    struct addrinfo* head = 0;
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(getaddrinfo(iDNS, 0, &hints, &head)) {
        return;
    }

    for(struct addrinfo* curr = head; curr; curr = curr->ai_next) {
        if(AF_INET6 == curr->ai_family) {
            mSize = sizeof(sockaddr_in6);
            memcpy(mAddress, curr->ai_addr, curr->ai_addrlen);
            getAddress6()->sin6_port = htons(mPort);
            reverse();
            break;
        }
        if(AF_INET == curr->ai_family) {
            mSize = sizeof(sockaddr_in);
            memcpy(mAddress, curr->ai_addr, curr->ai_addrlen);
            getAddress4()->sin_port = htons(mPort);
            reverse();
            break;
        }
    } //for

    ::freeaddrinfo(head);
}


NetAddress::IP NetAddress::toIP()const {
    IP id;
    if(isIPV6()) { //128bit
        DASSERT(sizeof(IP) == sizeof(getAddress6()->sin6_addr));
        id = &getAddress6()->sin6_addr;
    } else { //32bit
        DASSERT(sizeof(u32) == sizeof(getAddress4()->sin_addr));
        *(u32*)(&id) = (*(u32*)(&getAddress4()->sin_addr));
    }
    return id;
}


NetAddress::ID NetAddress::toID()const {
    ID id;
    if(isIPV6()) { //128bit
        DASSERT(sizeof(IP) == sizeof(getAddress6()->sin6_addr));
        id = &getAddress6()->sin6_addr;
    } else { //32bit
        DASSERT(sizeof(u32) == sizeof(getAddress4()->sin_addr));
        id.mHigh = (*(u32*)(&getAddress4()->sin_addr));
    }
    id.mTail = mPort;
    return id;
}


void NetAddress::setAddress6(const sockaddr_in6& it) {
    *getAddress6() = it;
    reverse();
}

void NetAddress::setAddress4(const sockaddr_in& it) {
    *getAddress4() = it;
    reverse();
}


void NetAddress::reverse() {
    memset(mName, 0, sizeof(mName));
    sockaddr& addr = *(sockaddr*)mAddress;
    usz len;
    if(AF_INET6 == addr.sa_family) {
        mPort = ntohs(getAddress6()->sin6_port);
        mSize = (u8)sizeof(sockaddr_in6);
        mName[0] = '[';
        inet_ntop(getAddress6()->sin6_family, &getAddress6()->sin6_addr, mName + 1, sizeof(mName) - 4);
        len = strlen(mName);
        mName[len++] = ']';
    } else {
        mPort = ntohs(getAddress4()->sin_port);
        mSize = (u8)sizeof(sockaddr_in);
        inet_ntop(getAddress4()->sin_family, &getAddress4()->sin_addr, mName, sizeof(mName) - 1);
        memset(mAddress + mSize, 0, sizeof(mAddress) - mSize);
        len = strlen(mName);
    }
    snprintf(mName + len, sizeof(mName) - len, ":%d", mPort);
}


s32 NetAddress::convertStr2IP(const s8* buffer, IP& result, bool ipv6) {
    DASSERT(buffer);
    return inet_pton(ipv6 ? AF_INET6 : AF_INET, buffer, &result);
}


void NetAddress::convertIP2Str(const IP& ip, String& result, bool ipv6) {
    result.reserve(DIP_STR_MAX_SIZE);
    ::inet_ntop(ipv6 ? AF_INET6 : AF_INET, &ip, (s8*)result.c_str(), DIP_STR_MAX_SIZE);
    result.setLen(strlen(result.c_str()));
}


}//namespace net
}//namespace app

