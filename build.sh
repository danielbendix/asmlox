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
native/arm64/print.s \
native/arm64/add.s \
native/arm64/subtract.s \
native/arm64/multiply.s \
native/arm64/divide.s \
native/arm64/increment.s \
native/arm64/equal.s \
native/arm64/not_equal.s \
native/arm64/less.s \
native/arm64/less_equal.s \
native/arm64/greater.s \
native/arm64/greater_equal.s \
native/arm64/not.s \
native/arm64/negate.s \
native/arm64/global.s \
native/arm64/closure.s \
native/arm64/call.s \
native/arm64/return.s \
$symbols_flag \
$optimization_flag \
$profiler_flag \
-lreadline -o asmlox
