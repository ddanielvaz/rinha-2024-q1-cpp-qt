cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(rinha_cpp VERSION 2.8.0 LANGUAGES CXX)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86|x86_64)$")
    find_package(Qt6 REQUIRED COMPONENTS HttpServer Sql Core Network Concurrent)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT "${CMAKE_CXX_COMPILER_TARGET}" STREQUAL "")
    set(arch ${CMAKE_CXX_COMPILER_TARGET})
else()
    set(arch ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(target ${PROJECT_NAME})

set(headers
    include/MutexedPool.h
    include/ThreadedDatabase.h
)

set(sources
    src/main.cpp
    src/ThreadedDatabase.cpp
)

source_group("Headers" FILES ${headers})
source_group("Sources" FILES ${sources})

add_executable(${target} ${sources} ${headers})

target_compile_definitions(${target} PRIVATE
    BUILD_SYSTEM_CMAKE
)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^(x86|x86_64)$")
    add_compile_definitions(ARCH_X86_64)
    set(name "rinha_cpp")
endif()

set_target_properties(${target} PROPERTIES
    OUTPUT_NAME ${name}
)

target_link_libraries(${target} PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::HttpServer
    Qt6::Network
    Qt6::Sql
)

target_include_directories(${target} PRIVATE
    src
    include
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message("Compilando em modo RELEASE")
    target_compile_definitions(${target} PRIVATE
        QT_NO_DEBUG_OUTPUT
        QT_NO_WARNING_OUTPUT
    )
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_definitions(${target} PRIVATE
        QT_NO_DEBUG_OUTPUT
        QT_NO_WARNING_OUTPUT
    )
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(${target} PUBLIC
        DEBUG_MODE
    )
endif()
