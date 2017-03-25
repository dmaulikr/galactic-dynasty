#!/bin/bash

if [ ! -e odoors ]
then
    git clone https://github.com/apamment/odoors
fi

cd odoors
make

cd ..

gcc -c main.c -o main.o -I./odoors/ -mno-ms-bitfields
gcc -c interbbs2.c -o interbbs2.o  -mno-ms-bitfields
gcc -c inih/ini.c -o inih/ini.o -mno-ms-bitfields
gcc -o GalacticDynasty main.o interbbs2.o inih/ini.o odoors/libs-`uname -s`/libODoors.a -lsqlite3
