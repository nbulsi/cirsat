cmake_minimum_required(VERSION 3.10)
project(cirsat)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/lib
)

add_subdirectory(lib)
add_subdirectory(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
add_subdirectory(examples)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
option(BUILD_TESTING "Build the testing tree." OFF)

if(BUILD_TESTING)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
    add_subdirectory(test)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(cirsat cirsat.cpp)
target_link_libraries(cirsat PUBLIC cirsat_lib lorina)
