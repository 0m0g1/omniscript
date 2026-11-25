#!/usr/bin/env bash
# Build dependencies/webp/bin/webp.dll.  Run from dependencies/webp in an MSYS2 MINGW64 shell:
#   cd /c/dev/omniscript/omniscript/dependencies/webp && ./build_webp.sh
# Needs: gcc (pacman -S mingw-w64-x86_64-gcc) and either git or curl+tar. src/webp_impl.c must be in ./src/.
set -e
if [ ! -d libwebp ]; then
    if command -v git >/dev/null 2>&1; then
        git clone --depth 1 --branch v1.4.0 https://github.com/webmproject/libwebp.git
    else
        curl -L -o libwebp.tar.gz https://github.com/webmproject/libwebp/archive/refs/tags/v1.4.0.tar.gz
        tar xzf libwebp.tar.gz
        mv libwebp-1.4.0 libwebp
        rm libwebp.tar.gz
    fi
fi
mkdir -p bin
SRCS=$(ls libwebp/src/dec/*.c libwebp/src/dsp/*.c libwebp/src/utils/*.c | grep -v -E 'enc|ssim|msa|neon|mips')
gcc -O2 -shared -static -static-libgcc -I libwebp -I libwebp/src -o bin/webp.dll $SRCS src/webp_impl.c
echo "built bin/webp.dll"