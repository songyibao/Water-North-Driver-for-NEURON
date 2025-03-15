clear
rm -rf build/
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/software/vcpkg/scripts/buildsystems/vcpkg.cmake ..
make

cp libplugin-water.so /root/tmp/neuron-2.10.2/build/plugins/