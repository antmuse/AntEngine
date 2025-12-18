#!/bin/bash

main(){
    local cmd=${1}
    echo "--------------------------------main: cmd=${cmd}"
    case "${cmd}" in
        "post")
            shift 1
            echo "post: upload $2 -> $1"
            if [ 1 == $# ]; then
                curl -k -X POST https://127.0.0.1:8443/fs/$1
            else
                curl -k -X POST https://127.0.0.1:8443/fs/$2 -T $1
            fi
            ;;
        "put")
            shift 1
            echo "put: upload $2 -> $1"
            curl -k -X PUT https://127.0.0.1:8443/fs/$2 -T $1 \
                --header "Connection: keep-alive" 
            ;;
        "get")
            shift 1
            echo "get: $1"
            curl -k -X GET https://127.0.0.1:8443/$1 \
                --header "Connection: close" \
                --header "User-Agent: cccl"
            ;;
        "strip")
            shift 1
            StripSymbol $@
            ;;
        "help")
            echo "get: index.html"
            echo "put: local.txt remote.txt"
            echo "post: local.txt remote.txt"
            echo "curl -k -X GET --http2 https://192.168.1.5:8443/index.html"
            ;;
        *)
            ;;
    esac
}

main $@
