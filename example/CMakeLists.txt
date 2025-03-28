file(GLOB FILENAMES *.cpp)

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} ${filename} ../src/parser.cpp ../src/solver.cpp ../src/aig.cpp)
endforeach()

# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/build/example)