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


#ifndef APP_SOCKET_H
#define APP_SOCKET_H


#include "NetAddress.h"

namespace app {
class RequestFD;

namespace net {


#if defined(DOS_WINDOWS)
using netsocket = usz;
#define DINVALID_SOCKET (~((usz)0))

#elif defined(DOS_LINUX) || defined(DOS_ANDROID)

#define DINVALID_SOCKET (-1)
typedef s32 netsocket;

#endif 



class Socket {
public:

    struct STCP_Info {
        //TODO
    };

    Socket(netsocket sock);

    Socket();

    ~Socket();

    Socket(const Socket& other) : mSocket(other.mSocket) {
    }

    Socket& operator=(const Socket& other) {
        //if(this == &other) {
        //    return *this;
        //}
        mSocket = other.mSocket;
        return *this;
    }

    Socket& operator=(const netsocket other) {
        mSocket = other;
        return *this;
    }

    bool operator==(const Socket& other) const {
        return mSocket == other.mSocket;
    }

    bool operator!=(const Socket& other) const {
        return mSocket != other.mSocket;
    }

    bool operator>(const Socket& other) const {
        return mSocket > other.mSocket;
    }

    bool operator<(const Socket& other) const {
        return mSocket < other.mSocket;
    }

    bool operator>=(const Socket& other) const {
        return mSocket >= other.mSocket;
    }

    bool operator<=(const Socket& other) const {
        return mSocket <= other.mSocket;
    }


    bool isOpen()const;

    s32 getError()const;

    netsocket getValue() const {
        return mSocket;
    }

    void setInvalid() {
        mSocket = DINVALID_SOCKET;
    }

    /**
    *@brief Set send overtime.
    *@param iTime The time in milliseconds.
    *@return 0 if successed, else failed.
    */
    s32 setSendOvertime(u32 iTime);

    /**
    *@brief Set receive overtime.
    *@param iTime The time in milliseconds.
    *@return 0 if successed, else failed.
    */
    s32 setReceiveOvertime(u32 iTime);

    /**
    *@brief Set keep alive.
    *@param on ture to turn on, else turn off.
    *@param idleTime How long to start tick when idle, in seconds.
    *@param timeInterval Time interval of tick, in seconds.
    *@param maxTick max ticks of heartbeat, ignored on windows OS.
    *@return 0 if successed, else failed.
    *@note On Windows Vista and later, the max tick is set to 10 and cannot be changed.
    */
    s32 keepAlive(bool on, s32 idleTime, s32 timeInterval, s32 maxTick);

    /**
    *@brief Set broadcast.
    *@return 0 if successed, else failed.
    */
    s32 setBroadcast(bool it);

    /**
    *@brief Set linger.
    *@param it true to turn on linger, else turn off.
    *@param iTime Time in seconds.
    *@return 0 if successed, else failed.
    */
    s32 setLinger(bool it, u16 iTime);

    /**
    *@brief Set block.
    *@return 0 if successed, else failed.
    */
    s32 setBlock(bool it);

    /**
    *@brief Set reuse IP.
    *@return 0 if successed, else failed.
    */
    s32 setReuseIP(bool it);

    /**
    *@brief Enbale or disable custom IP head, only raw socket.
    *@param on OS write IP head if true, else user write, default is true.
    *@return 0 if successed, else failed.
    */
    s32 setCustomIPHead(bool on);

    /**
    *@brief Set receive all data, promiscuous mode, only raw socket.
    *@return 0 if successed, else failed.
    */
    s32 setReceiveAll(bool on);

    /**
    *@brief Set reuse port.
    *@return 0 if successed, else failed.
    */
    s32 setReusePort(bool on);

    /**
    *@brief Set send cache size of socket.
    *@param size The socket cache size.
    *@return 0 if success, else if failed.
    */
    s32 setSendCache(s32 size);

    /**
    *@brief Set receive cache size of socket.
    *@param size The socket cache size.
    *@return 0 if success, else if failed.
    */
    s32 setReceiveCache(s32 size);

    /**
    *@brief Close socket.
    *@return true if success, else false.
    */
    bool close();
    bool closeSend();
    bool closeReceive();
    bool closeBoth();

    void getLocalAddress(NetAddress& it);

    void getRemoteAddress(NetAddress& it);

    //s32 getFamily()const;

    /**
    *@brief Bind socket with address.
    *@param it net address.
    *@return 0 if success, else if failed.
    */
    s32 bind(const NetAddress& it);

    /**
    *@brief Bind socket with any address["0.0.0.0:0"].
    *@return 0 if success, else if failed.
    */
    s32 bind(bool ipv6 = false);

    bool getTcpInfo(STCP_Info* info) const;

    /**
    *@brief Open a raw socket.
    *@param protocol Net protocol.
    *@return true if success, else if false.
    */
    bool openRaw(s32 protocol, bool ipv6 = false);

    bool openTCP(bool ipv6 = false);

    bool openUDP(bool ipv6 = false);

    /**
    *@brief Open a user defined socket.
    *@return true if success, else if false.
    */
    bool open(s32 domain, s32 type, s32 protocol);


    s32 sendTo(const void* iBuffer, s32 iSize, const NetAddress& address);

    s32 receiveFrom(void* iBuffer, s32 iSize, NetAddress& address);


    s32 send(const void* iBuffer, s32 iSize);

    /**
    *@brief Send in a loop, if using nonblock socket.
    */
    s32 sendAll(const void* iBuffer, s32 iSize);

    s32 receive(void* iBuffer, s32 iSize);

    s32 receiveAll(void* iBuffer, s32 iSize);

    /**
    *@brief Set delay.
    *@param it true to enable nagle, else disable nagle.
    *@return 0 if successed, else failed.
    */
    s32 setDelay(bool it);


    /**
    *@brief Connect with peer.
    *@return 0 if successed, else failed.
    */
    s32 connect(const NetAddress& it);

    /**
    *@brief Listen peer.
    *@param backlog range[1-65535] on windows, else min(net.core.somaxconn,backlog) on linux
    *@return 0 if successed, else failed.
    */
    s32 listen(s32 backlog);

    /**
    *@brief Accpet a new client.
    *@return A valid socket if successed, else invalid.
    */
    Socket accept();

    /**
    *@brief Accpet a new client.
    *@param it net address of remote peer.
    *@return A valid socket if successed, else invalid.
    */
    Socket accept(NetAddress& it);


    bool isAlive();

    bool openSeniorUDP(bool ipv6 = false);

    bool openSeniorTCP(bool ipv6 = false);

#if defined(DOS_WINDOWS)
    /**
    *@brief Update socket by accept socket.
    *@param sock accept socket.
    *@return 0 if success, else if failed.
    */
    s32 updateByAccepter(const Socket& sock);
    s32 updateByConnect();

    /**
    * @brief Windows only
    */
    bool openSenior(s32 domain, s32 type, s32 protocol, void* info, u32 group, u32 flag);

    /**
    *@brief Post a accept action.
    *@param sock The accpet socket.
    *@param iAction The accpet action.
    *@param addressCache The cache to store addresses of local and remote.
    *@note: In IPV4, addressCache size must not less than 64 = 2*(sizeof(sockaddr_in) + 16).
    */
    bool accept(const Socket& sock, RequestFD* iAction, void* addressCache, u32 addrsz)const;

    bool getAddress(void* addressCache, u32 addsize, NetAddress& local, NetAddress& remote)const;

    bool connect(const NetAddress& it, RequestFD* iAction)const;


    bool disconnect(RequestFD* iAction)const;


    bool receive(RequestFD* iAction)const;

    bool receiveFrom(RequestFD* iAction, NetAddress& addr)const;

    bool send(RequestFD* iAction)const;

    bool sendTo(RequestFD* iAction, const NetAddress& addr)const;


    void* getFunctionDisconnect()const;


    void* getFunctionConnect()const;


    void* getFunctionAccpet()const;


    void* getFunctionAcceptSockAddress()const;

private:
    static void* mFunctionConnect;
    static void* mFunctionDisconnect;
    static void* mFunctionAccept;
    static void* mFunctionAcceptSockAddress;
#endif//DOS_WINDOWS

protected:
    netsocket mSocket;
};




class SocketPair {
public:
    SocketPair();

    ~SocketPair();

    bool close();

    /**
    * @brief Open a local socket pair with AF_UNIX & SOCK_STREAM.
    * @param unpath path for sockaddr_un, windows only.
    * @return true if success to open, else false.
    */
    bool open(const s8* unpath = "");

    bool open(s32 domain, s32 type, s32 protocol, const s8* unpath = "");

    Socket& getSocketA() {
        return mSockA;
    }

    Socket& getSocketB() {
        return mSockB;
    }

private:
    Socket mSockA;
    Socket mSockB;
};


} //namespace net 
} //namespace app 


#endif //APP_SOCKET_H