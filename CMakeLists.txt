cmake_minimum_required(VERSION 3.14)
project(FileSystem)

set(CMAKE_CXX_STANDARD 11)

include_directories(src/source)

aux_source_directory(src/source dirSource)

set(SOURCE ${dirSource} src/source/define.cpp src/source/define.h src/source/define.h src/source/Directory.cpp src/source/Directory.h src/source/INoder.h src/source/Disk.cpp src/source/Disk.h src/source/User.cpp src/source/User.h src/source/FSystem.cpp src/source/FSystem.h
        main.cpp)

set(CMAKE_CXX_STANDARD 11)

add_executable(FileSystem ${SOURCE})
