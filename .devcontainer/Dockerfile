# FROM ubuntu

# RUN apt update \
# 	&& apt install -y gcc g++ make ninja-build cmake gdb lldb git clang libc++-dev libc++abi-dev libllvmlibc-*-dev zlib1g-dev libzstd-dev libedit-dev libcurl4-openssl-dev lld \
# 	&& apt clean

# FROM alpine:edge
FROM --platform=linux/amd64 alpine:edge
# FROM --platform=linux/arm64/v8 alpine:edge

RUN sed -i 's#https\?://dl-cdn.alpinelinux.org/alpine#https://mirrors.tuna.tsinghua.edu.cn/alpine#g' /etc/apk/repositories

RUN apk add --no-cache gcompat git cmake ninja gcc g++ clang musl-dev llvm-dev libc++-dev llvm-libunwind-dev lld libc-dev compiler-rt libc++-static llvm-libunwind-static linux-headers lldb clang-extra-tools

