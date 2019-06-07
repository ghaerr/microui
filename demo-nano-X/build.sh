#!/bin/bash
gcc -o demo-nx main.c renderer.c ../src/microui.c -I../src -I../../microwindows/src/include \
    -O3 -Wall -std=c11 -pedantic -L../../microwindows/src/lib -lnano-X -lm
