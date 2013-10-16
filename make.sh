#!/bin/bash

g++ main.cpp -Wall -g -std=c++0x -I/ubuntu_ext/test/prefix/include -L/ubuntu_ext/test/prefix/lib -Wl,--rpath=/ubuntu_ext/test/prefix/lib -lboost_system -lpthread -o urlcounter