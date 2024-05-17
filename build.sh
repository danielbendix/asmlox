#!/bin/zsh

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
-lprofiler \
-lreadline -O0 -g -o clox
