#!/bin/bash

set -e

# 如果没有build目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -fr `pwd`/build/*
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/hummingbird       .so库拷贝到 /usr/lib
if [ ! -d /usr/include/hummingbird ]; then
    mkdir /usr/include/hummingbird
fi

for header in `ls ./src/*.h`
do
    cp $header /usr/include/hummingbird
done

cp `pwd`/lib/libhummingbird.so /usr/lib
