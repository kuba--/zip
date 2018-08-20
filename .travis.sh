#!/bin/bash
#
# Build script for travis-ci.org builds.
#
mkdir build
cd build

if [ $ANALYZE = "true" ] && [ "$CC" = "clang" ]; then
    # scan-build -h
    scan-build cmake -G "Unix Makefiles" ..
    scan-build -enable-checker security.FloatLoopCounter \
        -enable-checker security.insecureAPI.UncheckedReturn \
        --status-bugs -v \
        make -j 8 \
        make -j 8 test
else
    cmake -DCMAKE_BUILD_TYPE=Debug -DSANITIZE_ADDRESS=On ..
    make -j 8
    ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=verbosity=1:log_threads=1 ctest -V
fi