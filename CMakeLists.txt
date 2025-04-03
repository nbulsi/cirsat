cmake_minimum_required(VERSION 3.10)
project(cirsat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(examples)

option(BUILD_TESTING "Build the testing tree." OFF)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_executable(cirsat cirsat.cpp)
target_link_libraries(cirsat PRIVATE cirsat_lib)

install(TARGETS cirsat
    RUNTIME DESTINATION bin
)
