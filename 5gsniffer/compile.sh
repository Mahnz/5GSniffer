#!/bin/bash
mkdir -p build && rm -rf ./build/* && cd build

CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..

# echo "[INFO] Compiling srsRAN..."
# CC=/usr/bin/gcc CXX=/usr/bin/g++ make srsRAN -j$(nproc) -Wno-error=template-id-cdtor

echo "[INFO] Compiling 5Gsniffer..."
CC=/usr/bin/gcc CXX=/usr/bin/g++ make -j$(nproc) -Wno-error=template-id-cdtor

# Optional
sudo make install
sudo ldconfig
