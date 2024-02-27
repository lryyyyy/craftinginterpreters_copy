mkdir -p build; cd build
cmake ..; make
cd -; cp build/clox clox