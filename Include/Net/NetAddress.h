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


#ifndef APP_NETADDRESS_H
#define APP_NETADDRESS_H


#include "IntID.h"
#include "Strings.h"


struct sockaddr_in;
struct sockaddr_in6;

//ipv4 "127.122.122.122:65535"
//ipv6 "[CDCD:910A:2222:5498:8475:1111:3900:2020]:65535"
#define DIP_STR_MAX_SIZE 48


namespace app {
namespace net {

class NetAddress {
public:
    typedef ID144 ID;
    typedef ID128 IP;

    /**
    *@brief Construct a net address with any ip and any port.
    *eg: "0.0.0.0:0", or "[::]:0"
    */
    NetAddress(bool ipv6 = false);

    /**
    *@brief Construct a net address with ip and port.
    *@param ipPort ip:port, eg: "127.0.0.1:6000" or "[23::AD]:6000"
    */
    NetAddress(const s8* ipPort);

    /**
    *@brief Construct a net address with "user defined ip" and any port.
    *@param ip User defined ip.
    */
    NetAddress(const String& ip);

    /**
    *@brief Construct a net address with "user defined port" and any ip.
    *@param port User defined port.
    */
    NetAddress(u16 port, bool ipv6 = false);

    ~NetAddress();

    NetAddress(const s8* ip, u16 port, bool ipv6 = false);

    NetAddress(const String& ip, u16 port, bool ipv6 = false);

    NetAddress(const NetAddress& other);

    NetAddress& operator=(const NetAddress& other);

    bool operator==(const NetAddress& other) const;

    bool operator!=(const NetAddress& other) const;

    bool isIPV6()const {
        return 28 == mSize;
    }

    //@return IPV6 = sizeof(sockaddr_in6)=28, IPV4 = sizeof(sockaddr_in)=16
    s32 getAddrSize()const {
        return mSize;
    }

    void setAddrSize(usz sz);

    /**
    *@return Family, IPV4=AF_INET,IPV6=AF_INET6
    */
    u16 getFamily()const;

    /**
    *@brief Set IP.
    *@param ip IPV4 or IPV6 str, without port.
    */
    void setIP(const s8* ip, bool ipv6 = false);

    void setIP(const String& ip, bool ipv6 = false) {
        setIP(ip.c_str(), ipv6);
    }

    /**
    *@brief Use local IP.
    *@return True if success to got local IP, else false.
    */
    bool setIP(bool ipv6 = false);

    void setPort(u16 port);

    /**
    *@brief Get port.
    *@return The port, in OS endian.
    */
    u16 getPort()const;

    /**
    *@brief Set IP by a DNS, the real address will auto inited.
    *@param iDNS User defined domain, eg: "www.google.com".
    */
    void setDomain(const s8* iDNS, bool ipv6 = false);

    /**
    *@brief Set IP:Port, the real address will auto inited.
    *@param ip User defined ip.
    *@param port User defined port, in OS-endian.
    */
    void set(const s8* ip, u16 port, bool ipv6 = false);

    /**
    *@brief Set IP and Port
    *@param ipAndPort format is "127.0.0.1:8080" or "[AD::01]:8080"
    */
    void setIPort(const s8* ipAndPort);

    void set(const String& ip, u16 port, bool ipv6 = false) {
        set(ip.c_str(), port, ipv6);
    }

    const s8* getStr() const {
        return mName;
    }

    void setAddress6(const sockaddr_in6& it);

    DFINLINE const sockaddr_in6* getAddress6() const {
        return reinterpret_cast<const sockaddr_in6*>(mAddress);
    }

    DFINLINE sockaddr_in6* getAddress6() {
        return reinterpret_cast<sockaddr_in6*>(mAddress);
    }

    void setAddress4(const sockaddr_in& it);

    DFINLINE const sockaddr_in* getAddress4() const {
        return reinterpret_cast<const sockaddr_in*>(mAddress);
    }

    DFINLINE sockaddr_in* getAddress4() {
        return reinterpret_cast<sockaddr_in*>(mAddress);
    }

    void reverse();

    static s32 convertStr2IP(const s8* ip, IP& result, bool ipv6 = false);

    static void convertIP2Str(const IP& ip, String& result, bool ipv6 = false);

    ID toID()const;

    IP toIP()const;

private:
    void clear();

    DINLINE void init(bool ipv6);
    DINLINE void initIP();
    DINLINE void initPort();

    u16 mPort;  //OS endian
    u8 mSize;
    u8 mAddress[28]; //sizeof(sockaddr_in6)=28, sizeof(sockaddr_in)=16
    s8 mName[DIP_STR_MAX_SIZE];
};


}//namespace net
}//namespace app

#endif //APP_NETADDRESS_H