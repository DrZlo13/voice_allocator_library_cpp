project(voice_allocator_cpp_library_tests LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_executable(voice_allocator_cpp_library_tests "tests.cpp")

target_link_libraries(voice_allocator_cpp_library_tests PRIVATE "voice_allocator_cpp_library")

add_test(NAME voice_allocator_cpp_library_tests COMMAND "voice_allocator_cpp_library_tests")