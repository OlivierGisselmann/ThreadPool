cmake_minimum_required(VERSION 3.12)

project(ThreadPool)

# Language features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create interface to link from
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Include public API
target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

# Tests and benchmarks
option(BUILD_TESTS "Build unit tests and benchmarks" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()