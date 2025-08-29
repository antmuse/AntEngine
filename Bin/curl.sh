#!/bin/bash

main(){
    local cmd=${1}
    echo "--------------------------------main: cmd=${cmd}"
    case "${cmd}" in
        "post")
            shift 1
            echo "post: upload $2 -> $1"
            if [ 1 == $# ]; then
                curl -k -X POST http://127.0.0.1:8000/fs/$1
            else
                curl -k -X POST http://127.0.0.1:8000/fs/$2 -T $1
            fi
            ;;
        "put")
            shift 1
            echo "put: upload $2 -> $1"
            curl -k -X PUT http://127.0.0.1:8000/fs/$2 -T $1
            ;;
        "get")
            shift 1
            echo "get: $1"
            curl -k -X GET http://127.0.0.1:8000/fs/$1
            ;;
        "strip")
            shift 1
            StripSymbol $@
            ;;
        "help")
            echo "get: index.html"
            echo "put: local.txt remote.txt"
            echo "post: local.txt remote.txt"
            ;;
        *)
            ;;
    esac
}

main $@
