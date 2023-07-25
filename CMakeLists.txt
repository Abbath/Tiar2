cmake_minimum_required(VERSION 3.12)

project(Tiar2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt)

add_executable(Tiar2 main.cpp)
target_include_directories(Tiar2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/include")
if (UNIX)
target_link_libraries(Tiar2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/lib/libraylib.so" fmt::fmt)
endif (UNIX)
if (WIN32)
target_link_libraries(Tiar2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../raylib-4.5.0/lib/raylib.dll" fmt::fmt)
endif (WIN32)
