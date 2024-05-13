set(NAME server)
project(${NAME})

file(GLOB SRC
     "src/*.c"
     "src/*.cpp"
	 "include/*.h"
)

include_directories(include)
include_directories(../common/include)

add_executable(${PROJECT_NAME} ${SRC})
