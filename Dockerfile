# syntax=docker/dockerfile:1

ARG OS_VERSION=22.04
ARG PROC

FROM ubuntu:${OS_VERSION}
ENV DEBIAN_FRONTEND=noninteractive
ARG RELEASE_VER=2023.02
ARG BUILD_TYPE=Release  # BUILD_TYPE Alternatives: Debug, RelWithDebInfo
ARG BLADERF_MODEL=xa9


RUN apt update && apt install -y --no-install-recommends \
      # Essentials \
      git \
      cmake \
      make \
      gcc \
      g++ \
      pkg-config \
      # srsRAN dependencies \
      libvolk2-dev \
      libvolk2-bin \
      libfftw3-dev \
      libmbedtls-dev \
      libconfig++-dev \
      libboost-program-options-dev \
      libsctp-dev \
      libyaml-cpp-dev \
      libgtest-dev \
      libliquid-dev \
      libzmq3-dev \
      libspdlog-dev \
      libfmt-dev \
      libuhd-dev \
      uhd-host \
      clang \
      build-essential \
      gpg-agent \
      libncurses5-dev \
      libusb-1.0-0 \
      libusb-1.0-0-dev \
      libtecla-dev \
      libtecla1 \
      ninja-build \
      software-properties-common \
      wget \
      # LTESniffer dependencies \
      ca-certificates \
      libglib2.0-dev \
      libudev-dev \
      libcurl4-gnutls-dev \
      libboost-all-dev \
      qtdeclarative5-dev \
      libqt5charts5-dev \
    # Download latest official firmware for the FPGA \
    && add-apt-repository ppa:nuandllc/bladerf \
    && apt update \
    && apt install -y --no-install-recommends \
    bladerf-fpga-hosted${BLADERF_MODEL} \
    # Cleanup APT cache \
    && rm -rf /var/lib/apt/lists/*

# ——————————————————————————————————————————————————————————————

RUN git clone --branch=${RELEASE_VER} \
  --depth=1 \
  --recursive \
  https://github.com/Nuand/bladeRF.git \
  /usr/local/src/bladeRF

WORKDIR /usr/local/src/bladeRF/build
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DINSTALL_UDEV_RULES=ON \
  -Wno-dev \
  -G Ninja \
  ../ \
  && JOBS=${PROC:-$(nproc)} && ninja -j"$JOBS" \
  && ninja install \
  && ldconfig

# ——————————————————————————————————————————————————————————————

WORKDIR /5gsniffer/
COPY ./5gsniffer .

RUN mkdir -p build && rm -rf build/*

WORKDIR /5gsniffer/build

ENV CXX=/usr/bin/clang++-14
ENV CC=/usr/bin/clang-14
RUN cmake \
      -DCMAKE_C_COMPILER=/usr/bin/clang-14 \
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++-14 ..

RUN JOBS=${PROC:-$(nproc)} && make srsRAN -j"$JOBS"
RUN JOBS=${PROC:-$(nproc)} && make -j"$JOBS"

RUN make install && ldconfig

# ——————————————————————————————————————————————————————————————

ENTRYPOINT ["/bin/bash"]
