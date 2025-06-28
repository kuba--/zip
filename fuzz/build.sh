cd $SRC/zip

mkdir -p build
cmake -S . -B build -DCMAKE_C_COMPILER_WORKS=1 -DZIP_BUILD_FUZZ=ON && cmake --build build --target install
