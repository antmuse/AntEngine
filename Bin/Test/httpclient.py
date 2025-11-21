#!/usr/bin/python3
#coding=utf-8
import random
import socket
import string
import time
if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv = ("127.0.0.1", 8000)
    print("------------connect server: ", serv)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sock.settimeout(10)
    sock.connect(serv)

    obuf = "GET / HTTP/1.1\r\nContent-Length: 0\r\nAccept: */*\r\nHost: loc.cn\r\n\r\n"
    if isinstance(obuf, str):
        obuf = obuf.encode("utf-8")
    
    for obt in obuf:
        print(f"sent byte: {obt}")
        sock.send(bytes([obt]))
        time.sleep(0.2)

    """
    sock.send("GET".encode("utf-8"))
    time.sleep(0.2)
    sock.send(" / HTTP/1.1\r\n".encode("utf-8"))
    sock.send("Content-Length: 7\r\n".encode("utf-8"))
    sock.send("Content-Length: 0\r\n".encode("utf-8"))
    sock.send("Accept: */*\r\n".encode("utf-8"))
    time.sleep(0.2)
    sock.send("Host: loc.cn\r\n\r\n".encode("utf-8"))
    """

    data = sock.recv(4096)
    print("------------receive: %s" % str(data.decode("utf-8")))
    count = 0
    while count<2: 
        try:  
            data = sock.recv(4096)
            print("------------receive: %s" % str(data.decode("utf-8")))
        except Exception as ee:
            print(ee)
        count += 1
        time.sleep(1)
    # end while()

    sock.close()
    print("------------cliend exit")
