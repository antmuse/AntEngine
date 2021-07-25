#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include "Net/NetAddress.h"
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

namespace app {

void AppTestNetAddress() {
    net::NetAddress a, b;
    net::NetAddress::ID ida, idb;
    net::NetAddress::IP ipa, ipb;
    const usz sss0 = sizeof(a);
    const usz sss1 = sizeof(ida);
    const usz sss2 = sizeof(ipa);

    String str;
    net::NetAddress::convertStr2IP("15.3.2.1", ipa);
    net::NetAddress::convertStr2IP("F::3::2::1", ipb, true);
    net::NetAddress::convertIP2Str(ipb, str, true);
    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");

    net::NetAddress::convertIP2Str(ipa, str);
    net::NetAddress::convertStr2IP("CDCD:910A:2222:5498:8475:1111:3900:2020", ipa, true);
    net::NetAddress::convertIP2Str(ipa, str, true);

    a.setIP();
    b.setIP(true);
    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");

    u16 fami1 = a.getFamily();
    u16 fami2 = b.getFamily();

    ida = a.toID();
    idb = b.toID();
    ipa = a.toIP();
    ipb = b.toIP();
    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");

    a.set("127.0.0.1", 9988, false);
    b.setIP("127.0.0.1", false);
    ida = a.toID();
    idb = b.toID();
    ipa = a.toIP();
    ipb = b.toIP();
    DASSERT(ida != idb);
    DASSERT(ipa == ipb);
    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");

    b.setIPort("127.0.0.1:19988");
    b.setPort(9988);
    ida = a.toID();
    idb = b.toID();
    ipa = a.toIP();
    ipb = b.toIP();
    DASSERT(ida == idb);
    DASSERT(ipa == ipb);
    net::NetAddress::convertIP2Str(ipa, str);
    net::NetAddress::convertIP2Str(ipb, str);

    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");

    a.set("23::AD", 7766, true);
    b.setIP("23::AD", true);
    b.setPort(7766);
    net::NetAddress c("[23::AD]:7766");
    ida = a.toID();
    idb = b.toID();
    ipa = a.toIP();
    ipb = b.toIP();
    DASSERT(ida == idb);
    DASSERT(ipa == ipb);
    DASSERT(ida == c.toID());
    net::NetAddress::convertIP2Str(ipa, str);
    net::NetAddress::convertIP2Str(ipb, str);

    printf("a=%s, b=%s,id==%s, ip==%s\n", a.getStr(), b.getStr(),
        ida == idb ? "yes" : "no", ipa == ipb ? "yes" : "no");


    net::NetAddress e("127.0.0.1:60000");
    net::NetAddress f("127.0.0.1:60000");
    DASSERT(e == f);
    DASSERT(e.toID() == f.toID());
    u16 fami3 = e.getFamily();
    u16 fami4 = f.getFamily();


    sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(60000);
    inet_pton(addr4.sin_family, "127.0.0.1", &(addr4.sin_addr));
    u32 id4 = *(u32*)(&addr4.sin_addr);
    ida = e.toID();
    s32 cmp = memcmp(e.getAddress4(), &addr4, sizeof(addr4));
    printf("finished\n");
}


} //namespace app
