#!/bin/bash

# Parse command line arguments for -j parameter
JOBS=$(nproc)
BUILD_SRSRAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -j)
            JOBS="$2"
            shift 2
            ;;
        -j*)
            JOBS="${1#-j}"
            shift
            ;;
        --srsran)
            BUILD_SRSRAN=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [-j <number_of_jobs>] [--srsran]"
            exit 1
            ;;
    esac
done

mkdir -p build && rm -rf ./build/* && cd build

CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..

if [ "$BUILD_SRSRAN" = true ]; then
    echo ""
    echo "[INFO] Compiling srsRAN with $JOBS parallel jobs..."
    CC=/usr/bin/gcc CXX=/usr/bin/g++ make srsRAN -j$JOBS -Wno-error=template-id-cdtor
fi

echo ""
echo "[INFO] Compiling 5Gsniffer with $JOBS parallel jobs..."
CC=/usr/bin/gcc CXX=/usr/bin/g++ make -j$JOBS -Wno-error=template-id-cdtor

# Optional
echo ""
echo "[INFO] Installing 5Gsniffer..."
sudo make install
sudo ldconfig
