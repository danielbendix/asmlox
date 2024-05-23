#!/bin/bash

symbols_flag=""
optimization_flag="-O3"
profiler_flag=""

for arg in "$@"
do
    case $arg in
        --debug)
            symbols_flag="-g"
            optimization_flag="-O0"
        ;;
        --profile)
            symbols_flag="-g"
            profiler_flag="-lprofiler"
        ;;
        -h|--help)
        echo "Usage: $0 [options]"
        echo "Options:"
        echo "  -h, --help      Display this help message"
        echo "  --debug         Create a debug build"
        echo "  --profile       Link with the gperftools profiler library"
        exit 0
        ;;
        *)
        echo "Unknown option: $arg"
        exit 1
        ;;
    esac
done

clang *.c \
native/arm64.c \
native/arm64/print.o \
native/arm64/add.o \
native/arm64/subtract.o \
native/arm64/multiply.o \
native/arm64/divide.o \
native/arm64/increment.o \
native/arm64/less.o \
native/arm64/equal.o \
native/arm64/not.o \
native/arm64/global.o \
native/arm64/closure.o \
native/arm64/call.o \
native/arm64/return.o \
$symbols_flag \
$optimization_flag \
$profiler_flag \
-lreadline -o asmlox
