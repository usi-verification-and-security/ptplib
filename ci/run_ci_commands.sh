#!/bin/bash

set -ev
if [ -d build ]; then rm -rf build; fi
mkdir -p build
cd build

if [ ! -z ${CMAKE_CXX_COMPILER} ]; then
    COMPILER_OPTION="-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
fi

cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
      -DCMAKE_CXX_FLAGS="${FLAGS}" \
      ${COMPILER_OPTION} \
      ..

make -j4

if [ x"${INSTALL_WITH_SUDO}" == x"yes" ]; then
    sudo make install
else
    make install
fi

#printer example
cd ../tests/printer_example/ && rm -rf build && mkdir -p build && cd build
cmake \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -DCMAKE_CXX_FLAGS="${FLAGS}" \
    ..

make -j4

./PTP-Example

#protocol example
cd ../../protocol_example/ && rm -rf build && mkdir -p build && cd build

if [ x"${PROTOCOL_EXAMPLE}" == x"yes" ]; then
    ASAN_OPTIONS=detect_leaks=1
    temp="${SFLAGS}"
else
    temp="${FLAGS}"
fi

cmake \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -DCMAKE_CXX_FLAGS="${temp}" \
    ..

make -j4

./Protocol-Example 2 10 0.1 5 