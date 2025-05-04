# Include catch2 directory (v2.2.1)
include_directories(catch2)

# Automatically collect all .cpp files under the test folder
file(GLOB_RECURSE FILENAMES 
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.cpp
)

# Print the list of collected test files
message(STATUS "Collected test files: ${FILENAMES}")

add_executable(run_tests ${FILENAMES})
target_link_libraries(run_tests PUBLIC cirsat_lib fmt lorina)

if (ENABLE_COVERAGE)
  target_link_libraries(run_tests PUBLIC gcov)
endif()
target_compile_definitions(run_tests PUBLIC CATCH_CONFIG_CONSOLE_WIDTH=300)
