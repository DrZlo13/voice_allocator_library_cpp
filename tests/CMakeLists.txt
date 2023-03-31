project(voice_allocator_library_cpp_tests LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    "tests.cpp"
    "tests_mono.cpp"
    "tests_poly.cpp"
)

add_executable(voice_allocator_library_cpp_tests ${SOURCES})

target_link_libraries(voice_allocator_library_cpp_tests "voice_allocator_library_cpp")

add_test(NAME voice_allocator_library_cpp_tests COMMAND "voice_allocator_library_cpp_tests")