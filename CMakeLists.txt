cmake_minimum_required(VERSION 3.10)
set(CMAKE_GENERATOR_PLATFORM Win32)
project(StoneCamera VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  minhook
  GIT_REPOSITORY https://github.com/tsudakageyu/minhook
  GIT_TAG v1.3.4
)

FetchContent_MakeAvailable(minhook)

add_library(stone_camera SHARED stone_camera.cpp)

set_target_properties(stone_camera PROPERTIES
    SUFFIX ".asi"
)

target_link_libraries(stone_camera PRIVATE minhook)
