cmake_minimum_required(VERSION 3.1.0)
project(SOIDIERFIRE)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# set(CMAKE_BUILD_TYPE "Debug")

# set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Options
option(CIRSAT_EXAMPLES "Build example" OFF)
option(CIRSAT_TEST "Build test" ON)

#绝对路径写法：include_directories(${CMAKE_SOURCE_DIR/include}),#CMAKE_SOURCE_DIR等价于根目录
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(cirsat main.cpp src/parser.cpp src/solver.cpp src/aig.cpp)

if(CIRSAT_EXAMPLES)
  add_subdirectory(example)
endif()

if(CIRSAT_TEST)
  add_subdirectory(test)
endif()

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build)
