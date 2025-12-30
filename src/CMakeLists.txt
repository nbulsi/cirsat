file(GLOB_RECURSE SOURCES "*.cpp")

add_library(cirsat_lib ${SOURCES})

target_include_directories(cirsat_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cirsat_lib PUBLIC lorina)