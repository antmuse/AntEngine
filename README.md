AntEngine
====
跨平台，多进程，网络服务，Redis Client, TCP/TLS/HTTP/HTTPS

# 依赖
----
Openssl, zlib, jsoncpp, http_parser


# Usage
----

```cpp
//demo 1: TLS-server
int main(int argc, char** argv) {
    Engine& eng = Engine::getInstance();
    eng.init(argv[0]);
    Loop& loop = eng.getLoop();
    net::Acceptor* conn= new net::Acceptor(loop, net::Linker::funcOnLink, true?"TLS":nullptr);
    succ = conn->open(argv[2]);
    while (loop.run()) {    }
    eng.uninit();
    return 0;
}
```
