cmake_minimum_required(VERSION 3.10)
project(udp_forward)

include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)

# 执行 git describe 命令，获取最新的提交的版本信息
# find_program(GIT_EXECUTABLE git)
# if (GIT_EXECUTABLE)
# execute_process(
# COMMAND git rev-parse --short=8 HEAD
# WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
# OUTPUT_VARIABLE GIT_SHA1
# )

# # 去掉 GIT_SHA1 变量中的换行符和空格
# # 判断 GIT_SHA1 变量是否为空
# if (NOT "${GIT_SHA1}" STREQUAL "")
# string(STRIP ${GIT_SHA1} GIT_SHA1)
# # 输出 GIT_SHA1 变量
# message(STATUS "Git SHA1: ${GIT_SHA1}")
# endif()
# endif ()

# asio
# add_library(asio INTERFACE)
# target_include_directories(asio INTERFACE asio-1.28.0/include)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio
    GIT_TAG asio-1-28-2
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

if(WIN32)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0601 _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    target_link_libraries(asio INTERFACE ws2_32)
endif()

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
endif()

# FetchContent_MakeAvailable(spdlog)
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${spdlog_SOURCE_DIR}/include)

# ini
FetchContent_Declare(
    inipp
    GIT_REPOSITORY https://github.com/mcmtroffaes/inipp.git
    GIT_TAG master
)
FetchContent_GetProperties(inipp)
if(NOT inipp_POPULATED)
    FetchContent_Populate(inipp)
endif()

add_library(inipp INTERFACE)
target_include_directories(inipp INTERFACE ${inipp_SOURCE_DIR})

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if(DEFINED SPD_LOG_LEVEL)
    message(STATUS "SPD_LOG_LEVEL=${SPD_LOG_LEVEL}")
    target_compile_definitions(spdlog INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${SPD_LOG_LEVEL})
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(STATUS "SPD_LOG_LEVEL=TRACE")
        target_compile_definitions(spdlog INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
    else()
        message(STATUS "SPD_LOG_LEVEL=DEBUG")
        target_compile_definitions(spdlog INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    endif()
endif()

# inc
set(udp_forward_inc
    common.h
    log.h
    udp_forward_server.h
    udp_forward_session.h
)

source_group("include" FILES ${udp_forward_inc})

# src
set(udp_forward_src
    main.cpp
    log.cpp
    udp_forward_server.cpp
    udp_forward_session.cpp
)

source_group("source" FILES ${udp_forward_src})

add_executable(${PROJECT_NAME} ${udp_forward_inc} ${udp_forward_src})

if(GIT_SHA1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE __GIT_SHA1__="${GIT_SHA1}")
endif()

if(NOT MSVC)
# target_compile_options(${PROJECT_NAME} PUBLIC -fvisibility=hidden -fPIC -Wall -flto=auto)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC /MP /MT)
endif()

# if (WIN32)
# target_compile_options(${PROJECT_NAME} PUBLIC /std:c++20)
# endif()

# target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME} PUBLIC asio spdlog inipp)

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} PUBLIC log)
endif()

if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
    target_link_libraries(${PROJECT_NAME} PUBLIC -static-libgcc -static-libstdc++ -static)
endif()
