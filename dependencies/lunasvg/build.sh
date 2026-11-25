#!/bin/bash
set -e
mkdir -p bin obj
L=src/lunasvg
P=$L/plutovg
DEFS="-DLUNASVG_BUILD_STATIC -DPLUTOVG_BUILD_STATIC -O2"
INC="-I$L/include -I$L/source -I$P/include -I$P/source"

for f in $P/source/*.c; do
  gcc $DEFS $INC -c $f -o obj/pv_$(basename $f .c).o
done
for f in $L/source/*.cpp src/svg_impl.cpp; do
  g++ -std=c++17 $DEFS $INC -c $f -o obj/$(basename $f .cpp).o
done

g++ -shared -o bin/lunasvg.dll obj/*.o -static -static-libgcc -static-libstdc++ -lm
echo built bin/lunasvg.dll