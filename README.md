# Thread Pool

**-- WORK IN PROGRESS --**

## Motivation
This project was born because I needed a way to automatically dispatch work across multiple threads, without worrying about thread management and race conditions.

## Architecture
TODO - Explain how this works
TODO - Add sequence and class diagram

## Build instructions
This project comes with a CMake integration option. If you wish to link this as an **INTERFACE** library, you just need to build with CMake and link **ThreadPool** against your executable.
```
# Build the interface only
cmake -S . -B out -G Ninja

# Build tests and benchmarks as well
cmake -S . -B out -G Ninja -DBUILD_TESTS=ON
```

## Credits
TODO - Add resources used for this project