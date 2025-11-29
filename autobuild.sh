rm -rf build
mkdir build
conan install . --build=missing -s build_type=Debug -of build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
make -j8