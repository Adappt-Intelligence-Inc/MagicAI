
rm -rf build

mkdir -p build

cd build

cmake -DCMAKE_BUILD_TYPE=Debug .. -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF

#cmake -DENABLE_TESTING=Off -DCMAKE_BUILD_TYPE=MinSizeRel ..  -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF  -DUSE_SHARED_MBEDTLS_LIBRARY=1

make -j32

cd ..
