rm -rf build_temp_dir
mkdir build_temp_dir
cd build_temp_dir
cmake ..
make -j4
mv compile_test ../
