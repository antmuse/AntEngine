AntEngine
====
跨平台，多进程，网络服务，Redis Client, TCP/TLS/HTTP/HTTPS

## 依赖
----
openssl, zlib, jsoncpp, http_parser, mysql

## 环境
----
+ 1 debian10, 64bit, kernal-v4.19.0, gcc-v8.3
+ 2 windows10, 64bit, VS2019
+ 3 c++11

## Usage
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


## TODO
----
+ 1 Linux AIO
+ 2 IPC
+ 3 WebSite, 分离HTTP基础代码和业务代码
