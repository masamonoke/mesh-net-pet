include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)

cmake_minimum_required(VERSION 3.23.2)

set(NAME mesh-topology)
project(${NAME})

set(CMAKE_C_STANDARD 17)

# external libraries
include_directories(deps/log.c/src)

# common sources and headers library
add_subdirectory(common)
add_subdirectory(deps)

# modules
add_subdirectory(node)
add_subdirectory(server)
add_subdirectory(client)

# linkage
target_link_libraries(common logc)
target_link_libraries(mesh_node common)
target_link_libraries(server common)
target_link_libraries(client common)

# if iwyu not installed it's ok
# https://github.com/include-what-you-use/include-what-you-use
find_program(iwyu_path NAMES include-what-you-use iwyu)
if (iwyu_path)
	set_property(TARGET mesh_node PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
	set_property(TARGET server PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
	set_property(TARGET client PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
	set_property(TARGET common PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
