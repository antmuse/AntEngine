AntEngine
====
+ 1. This is a cross platform, multi process, asynchronous, network services.
+ 2. support TCP/TLS/HTTP/HTTPS/KCP
+ 3. support Redis Client, MySQL client
+ 4. use epoll, io_uring under Linux
+ 5. use IOCP under Windows

## Depends
----
+ 1. openssl-3.5.2, commit_id: 0893a62353583343eb712adef6debdfbe597c227
+ 2. zlib, jsoncpp, mysqlclient

## Development environment
----
+ 1. debian12, 64bit, kernal-v6.1
+ 2. windows11, 64bit, VS2022
+ 3. c++11
## Usage
----

```cpp
void once_task(void* data){
    DLOG(ELL_INFO, "once_task done: %p", data);
}
int main(int argc, char** argv) {
    Engine& eng = Engine::getInstance();
    if (!eng.init(argv[0], false, "{}")) {
        printf("main>> engine init fail\n");
        return -1;
    }
    eng.getLoop().postTask(once_task, (void*)nullptr);
    eng.run();
    DLOG(ELL_INFO, "main>>exit...");
    eng.uninit();
    printf("main>>stop\n");
    return 0;
}
```


## TODO
----
+ 1. FRP Server|Client
