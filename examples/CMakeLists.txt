file(GLOB_RECURSE EXAMPLE_SOURCES "*.cpp")

foreach(example_source ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_source} NAME_WE)
    add_executable(${example_name} ${example_source})
    # 修改库名为 cirsat_lib
    target_link_libraries(${example_name} PRIVATE cirsat_lib)
endforeach()