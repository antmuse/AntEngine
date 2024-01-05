#!/bin/bash


RUNDIR="$(dirname "$(readlink -f "${BASH_SOURCE:-$0}" )" )"

function ShowRuning(){
    echo "--------------------------------rundir = ${RUNDIR}"
    echo "--------------------------------ShowRuning: process of ${1}"
    ps -ef | grep ${1} | grep -v grep
    echo "--------------------------------ShowRuning: tcp of ${1}"
    sudo ss -tlnp | grep ${1}
}

function SetENV(){
    export LD_PRELOAD=$LD_PRELOAD:$(pwd)/../../../depend/jemalloc/lib/libjemalloc.so.2
    export LD_DEBUG=files
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)

    #不允许非root用户的指定程序绑定1024以下端口
    #sudo setcap -r ${RUNDIR}/Server.bin

    #允许非root用户的指定程序绑定1024以下端口
    sudo /usr/sbin/setcap cap_net_bind_service=+eip ${RUNDIR}/Server.bin


    if [ $? -eq 0 ]; then
        echo "====last cmd ok!===="
    else
        echo "====last cmd fail!===="
        exit 1
    fi

    ulimit -c unlimited
    ulimit -c

    # echo "/tmp/core/%e-%p-%t.core" > /proc/sys/kernel/core_pattern
    # echo "2" > /proc/sys/fs/suid_dumpable
    cat /proc/sys/kernel/core_pattern
}


function StopServer(){
    if test -f "${RUNDIR}/Log/pid.txt"; then
        local ID=`cat 'Log/pid.txt'`
        echo "--------------------------------kill all Server.bin, pid=${ID}"
        kill -2 $ID
        sleep 1
    else
        echo "--------------------------------pid.txt not exist."
    fi
    ShowRuning "Server.bin"
}


function ShowHelp(){
    export LD_DEBUG=help
    echo "--------------------------------rundir = ${RUNDIR}"
    echo "--------------------------------usage:"
    echo "--------------------------------1.     ./Server.bin"
    echo "--------------------------------2.     ./Server.bin"
    echo "--------------------------------3.     ./Server.bin"
    echo "--------------------------------bin:"
    ls -lh ${RUNDIR}/*.bin
}

main(){
    local cmd=${1}
    echo "--------------------------------main: cmd=${cmd}"
    case "${cmd}" in
        "start")
            echo "start Server.bin"
            SetENV
            ps -ef|grep Server.bin | grep -v grep
            # ./Server.bin $@ > /dev/null 2>&1 &
            ./Server.bin $@
            ;;
        "stop")
            echo "stop Server.bin"
            StopServer
            ;;
        "show")
            echo "show Server.bin"
            ShowRuning "Server.bin"
            ;;
        "help")
            ShowHelp
            ;;
        *)
            echo '--------------------------------输入 1 到 4 之间的数字:'
            echo '你输入的数字为:'
            read aNum
            case $aNum in
                1)  echo '你选择了 1'
                    ;;
                2)  echo '你选择了 2'
                    ;;
                3)  echo '你选择了 3'
                    ;;
                4)  echo '你选择了 4'
                    ;;
                *)  echo '你没有输入[1-4]之间的数字'
                    ;;
            esac
            ;;
    esac
}


main $@
