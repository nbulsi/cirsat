add_library(cirsat_lib INTERFACE)
target_include_directories(cirsat_lib SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(cirsat_lib INTERFACE lorina fmt)