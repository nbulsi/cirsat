include_directories(catch2) # v2.2.1

file(GLOB_RECURSE FILENAMES *.cpp)

# foreach(filename ${FILENAMES})
#   get_filename_component(basename ${filename} NAME_WE)
#   add_executable(${basename} ${filename} ../src/parser.cpp ../src/solver.cpp ../src/aig.cpp)
# endforeach()

add_executable(run_tests ${FILENAMES} ../src/parser.cpp ../src/solver.cpp ../src/aig.cpp)
# add_executable(run_tests ${FILENAMES})
if (ENABLE_COVERAGE)
  target_link_libraries(run_tests PUBLIC gcov)
endif()
target_compile_definitions(run_tests PUBLIC CATCH_CONFIG_CONSOLE_WIDTH=300)
