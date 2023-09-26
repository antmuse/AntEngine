AntEngine
====
+ 1. This is a cross platform, multi process, asynchronous, network services.
+ 2. support TCP/TLS/HTTP/HTTPS/KCP
+ 3. support Redis Client, MySQL client
+ 4. use epoll, io_uring under Linux
+ 5. use IOCP under Windows

## Depends
----
openssl, zlib, jsoncpp, mysql

## Development environment
----
+ 1. debian12, 64bit, kernal-v6.1
+ 2. windows11, 64bit, VS2022
+ 3. c++11
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
    conn->drop();
    while (loop.run()) {    }
    eng.uninit();
    return 0;
}
```


## TODO
----
+ 1. FRP Server|Client
