include_directories(catch2) # v2.2.1

file(GLOB_RECURSE FILENAMES *.cpp)

add_executable(run_tests ${FILENAMES})
target_link_libraries(run_tests PUBLIC cirsat)
if (ENABLE_COVERAGE)
  target_link_libraries(run_tests PUBLIC gcov)
endif()
target_compile_definitions(run_tests PUBLIC CATCH_CONFIG_CONSOLE_WIDTH=300)
