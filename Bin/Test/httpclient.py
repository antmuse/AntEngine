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
    sock.connect(serv)
    sock.send("GET".encode("utf-8"))
    time.sleep(1)
    sock.send(" / HTTP/1.1\r\n\r\n".encode("utf-8"))
    sock.settimeout(5)
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
