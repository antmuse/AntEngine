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

function Temp(){
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
}


function ShowHelp(){
    export LD_DEBUG=help
    echo "--------------------------------rundir = ${RUNDIR}"
    echo "--------------------------------usage:"
    echo "--------------------------------1. start    // start Server.bin"
    echo "--------------------------------2. stop     // stop  Server.bin"
    echo "--------------------------------3. show     // show  running info of Server.bin"
    echo "--------------------------------4. strip    // strip files"
    echo "--------------------------------5. help     // show help"
    echo "--------------------------------bin:"
    ls -lh ${RUNDIR}/*.bin
}

function StripExit() {
    #usage:
    scriptname=`basename ${0}`
    echo "USAGE ${scriptname} <tostrip>"

    errorcode=${1}
    shift
    echo $@
    exit ${errorcode}
}


function StripSymbol() {
    to_strip_dir=`dirname "$1"`
    to_strip_file=`basename "$1"`
    symbol_dir="."
    echo "dir $1,$#"

    if [ -z ${to_strip_file} ] ; then
        StripExit 0 "tostrip must be specified"
    fi

    cd "${to_strip_dir}"

    if [ 2 == $# ]; then
        symbol_dir=$2
    fi

    symbol_file="${to_strip_file}.debug"

    if [ "." != ${symbol_dir} ] ; then
        if [ ! -d "${symbol_dir}" ] ; then
            echo "creating dir ${to_strip_dir}/${symbol_dir}"
            mkdir -p "${symbol_dir}"
        fi
    fi


    symbol_line="$(nm ${to_strip_file} | wc -l)"
    if [ ${symbol_line} != 0 ] ; then
        echo "stripping ${to_strip_file}, putting debug symbol into ${symbol_file}"

        objcopy --only-keep-debug "${to_strip_file}" "${symbol_dir}/${symbol_file}"
        strip --strip-debug --strip-unneeded "${to_strip_file}"
        objcopy --add-gnu-debuglink="${symbol_dir}/${symbol_file}" "${to_strip_file}"
        chmod -x "${symbol_dir}/${symbol_file}"
    else
        echo "no symbols in execute file ${to_strip_file}"
    fi
    cd -
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
        "strip")
            shift 1
            StripSymbol $@
            ;;
        "help")
            ShowHelp
            ;;
        *)
            ShowHelp
            ;;
    esac
}


main $@
