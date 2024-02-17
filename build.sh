#!/bin/zsh

clang *.c \
native/arm64.c \
native/arm64/print.o \
native/arm64/add.o \
native/arm64/less.o \
native/arm64/global.o \
native/arm64/return.o \
-lprofiler \
-lreadline -O3 -g -o clox 
