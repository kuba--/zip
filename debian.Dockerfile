FROM debian

RUN apt-get update -qq && \
    apt-get install -qy cmake gcc make libc-dev && \
    rm -rf /var/lib/apt/lists/* /var/cache/apt

COPY . /zip

WORKDIR /zip

RUN cmake -DCMAKE_BUILD_TYPE='Debug' \
          -DCMAKE_DISABLE_TESTING=0 \
          -S . -B 'build' && \
    cmake --build 'build'

CMD cd build && ctest .
