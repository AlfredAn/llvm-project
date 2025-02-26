#!/bin/bash
../build/bin/clang -emit-llvm -S a.c -Xclang -disable-O0-optnone

