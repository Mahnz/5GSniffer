# syntax=docker/dockerfile:1

ARG OS_VERSION=22.04

FROM ubuntu:${OS_VERSION}
ENV DEBIAN_FRONTEND=noninteractive

RUN apt -y update
RUN apt -y install git cmake make gcc g++ pkg-config libfftw3-dev \
    libmbedtls-dev libsctp-dev libyaml-cpp-dev libgtest-dev libliquid-dev \
    libconfig++-dev libzmq3-dev libspdlog-dev libfmt-dev libuhd-dev uhd-host clang

WORKDIR /5gsniffer/
COPY ./5gsniffer .

RUN mkdir -p build && rm -rf build/*

WORKDIR /5gsniffer/build

# ENV CXX=/usr/bin/clang++-14
# ENV CC=/usr/bin/clang-14
# RUN cmake -DCMAKE_C_COMPILER=/usr/bin/clang-14 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-14 ..
RUN cmake ..

RUN make srsRAN -j$(nproc)
RUN make -j$(nproc)

ENTRYPOINT ["/bin/bash"]