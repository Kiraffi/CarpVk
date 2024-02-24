cmake_minimum_required(VERSION 3.27)
project(carpvk)

set(CMAKE_CXX_STANDARD 17)
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -m64")
#ADD_DEFINITIONS(-DCMAKE_GENERATOR_PLATFORM=x64)

if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

include_directories(${Vulkan_INCLUDE_DIRS})

add_library(carpvk OBJECT
        src/carpvk.cpp
        src/carpvk.h
)

add_executable(carpvktest src/main.cpp)

target_include_directories(carpvk PUBLIC src)

target_link_libraries(carpvktest PRIVATE
        vulkan
        carpvk
)