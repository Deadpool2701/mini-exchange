cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure


cmake --build build-release --target exchange_benchmark
./build-release/exchange_benchmark