AntEngine
====
跨平台，多进程，网络服务
# Usage
----

```cpp
//demo 1: TLS-server
void AppTestNetServer() {
    Engine& eng = Engine::getInstance();
    eng.init();
    Loop& loop = eng.getLoop();
    net::Acceptor* conn= new net::Acceptor(loop, net::Linker::funcOnLink, true?"TLS":nullptr);
    succ = conn->open(argv[2]);
    while (loop.run()) {    }
    eng.uninit();
    return 0;
}
```
