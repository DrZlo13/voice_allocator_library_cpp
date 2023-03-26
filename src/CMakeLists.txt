cmake_minimum_required(VERSION 3.9)

project("voice_allocator_cpp_library")

set(INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(
    ${INCLUDES}
)

file(GLOB_RECURSE SOURCES
    "voice_allocator.cpp"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${INCLUDES}
)