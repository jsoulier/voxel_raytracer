project(stb)
add_library(stb stb.c)
target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})