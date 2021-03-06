#!/bin/bash

# Linux compile
#gcc -o demo-sdl2 main.c renderer.c ../src/microui.c -I../src -O3 -Wall -std=c11 -pedantic -lSDL2 -lGL -lm -g

# OSX compile
gcc -o demo-sdl2 main.c renderer.c ../src/microui.c -I../src -O3 -Wall -std=c11 -pedantic -lSDL2 -framework OpenGL -lm
