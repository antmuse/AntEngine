#!/usr/bin/python3
#coding=utf-8

import socket

if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    sock.bind(("0.0.0.0", 55007))

    # 1.receive message and get one export ip:port (PC1)
    data, conn1 = sock.recvfrom(1024)
    addr1 = "%s:%d" % (conn1[0], conn1[1])
    print("1.get PC1 export ip:port = %s" % addr1)

    # 2.receive message and get another export ip:port (PC2)
    data, conn2 = sock.recvfrom(1024)
    addr2 = "%s:%d" % (conn2[0], conn2[1])
    print("2.get PC2 export ip:port = %s" % addr2)

    # 3.send export address of PC1 to PC2
    sock.sendto(addr1.encode("utf-8"), conn2)
    print("3.send export address of PC1(%s) to PC2(%s)" % (addr1, addr2))

    # 4.send export address of PC2 to PC1
    sock.sendto(addr2.encode("utf-8"), conn1)
    print("4.send export address of PC2(%s) to PC1(%s)" % (addr2, addr1))

    print("done")
    sock.close()
# end main()
