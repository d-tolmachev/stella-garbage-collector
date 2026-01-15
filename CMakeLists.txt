cmake_minimum_required(VERSION 3.8)

project(stella_garbage_collector)

set(CMAKE_CXX_STANDARD 20)

set(MAX_ALLOC_SIZE_VALUE "" CACHE STRING "Maximum size of heap region (REQUIRED)")

if(NOT MAX_ALLOC_SIZE_VALUE)
    message(FATAL_ERROR "MAX_ALLOC_SIZE_VALUE must be specified!")
endif()

add_library(runtime STATIC
        stella/runtime.c
)

target_compile_definitions(runtime PRIVATE $<$<CONFIG:Debug>:STELLA_GC_STATS;STELLA_RUNTIME_STATS>)

add_library(stella_garbage_collector STATIC
        src/garbage_collector.cpp
)

target_include_directories(stella_garbage_collector PRIVATE stella)
target_compile_definitions(stella_garbage_collector PRIVATE MAX_ALLOC_SIZE=${MAX_ALLOC_SIZE_VALUE})
target_link_libraries(stella_garbage_collector private runtime)
