AntEngine
====
��ƽ̨������̣��������Redis Client, TCP/TLS/HTTP/HTTPS

# ����
----
openssl, zlib, jsoncpp, http_parser, mysql

# ����
----
1. debian10, 64bit, kernal-v4.19.0, gcc-v8.3
2. windows10, 64bit, VS2019
3. c++14

# Usage
----

```cpp
//demo 1: TLS-server
int main(int argc, char** argv) {
    Engine& eng = Engine::getInstance();
    eng.init(argv[0]);
    Loop& loop = eng.getLoop();
    net::Acceptor* conn= new net::Acceptor(loop, net::Linker::funcOnLink, true?"TLS":nullptr);
    succ = conn->open("0.0.0.0:443");
    while (loop.run()) {    }
    eng.uninit();
    return 0;
}
```


# TODO
----
1 Linux AIO
2 IPC
3 WebSite, ����HTTP���������ҵ�����