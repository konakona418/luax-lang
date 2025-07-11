cmake_minimum_required(VERSION 3.10.0)
project(luaxc VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp src/*.c src/*.h src/*.hpp)
file(GLOB_RECURSE TESTS test/*.cpp test/*.h test/*.hpp test/*.c test/*.h test/*.hpp)

add_executable(luaxc main.cpp ${SOURCES} ${TESTS})
target_include_directories(luaxc PRIVATE src)
target_include_directories(luaxc PRIVATE test)

