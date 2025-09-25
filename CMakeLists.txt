cmake_minimum_required(VERSION 3.12)
project(MyEcs C)

set(CMAKE_C_STANDARD 11)

# Find the OpenMP package
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")

# Add your executable
add_executable(MyEcs
        ecs.c
        example.c
)

# Link OpenMP and the math library to your executable
target_link_libraries(MyEcs PRIVATE m)