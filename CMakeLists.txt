cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(template_sample
        LANGUAGES C CXX
        DESCRIPTION "test coding"
        HOMEPAGE_URL "blog.csdn.net/wanghualin033")

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)

file(GLOB SRCS *.cpp *.hpp)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} pthread)