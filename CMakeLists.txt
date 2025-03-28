# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和语言
project(cirsat LANGUAGES CXX)

# 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# 设置编译选项
set(CMAKE_CXX_STANDARD 17) # 使用 C++17 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3") # Release 模式下的优化选项
set(CMAKE_CXX_FLAGS_DEBUG "-g")    # Debug 模式下的调试选项

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 添加可执行文件
add_executable(cirsat ${SOURCES})

# target_link_libraries(cirsat PRIVATE <library_name>)