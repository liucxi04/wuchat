cmake_minimum_required(VERSION 3.5)
project(chat)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/luwu)

include (luwu/cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/luwu)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(LIB_SRC
        chat/my_module.cc
        chat/resource_servlet.cpp
        chat/protocol.cpp
        chat/chat_servlet.cpp
    )

add_library(chat SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(chat)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
