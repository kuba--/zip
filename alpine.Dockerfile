FROM alpine

RUN apk add --no-cache gcc cmake make musl-dev

COPY . /zip

WORKDIR /zip

RUN cmake -DCMAKE_BUILD_TYPE='Debug' \
          -DCMAKE_DISABLE_TESTING=0 \
          -S . -B build && \
    cmake --build build

CMD cd build && ctest .
