cmake_minimum_required(VERSION 3.12)
project(MyEcs C)

set(CMAKE_C_STANDARD 11)

add_executable(MyEcs
        ecs.c
        example.c
)
