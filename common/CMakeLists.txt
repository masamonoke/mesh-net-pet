set(NAME common)
project(${NAME})

file(GLOB SRC
     "src/*.c"
     "src/*.cpp"
	 "include/*.h"
)

include_directories(include)

add_library(${PROJECT_NAME} ${SRC})
