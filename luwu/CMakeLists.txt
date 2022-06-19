cmake_minimum_required(VERSION 3.0)
project(luwu)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++14 -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)

option(BUILD_TEST "ON for compile test" ON)

find_package(Boost REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

set(LIB_SRC
        luwu/log.cpp
        luwu/config.cpp
        luwu/thread.cpp
        luwu/fiber.cpp
        luwu/scheduler.cpp
        luwu/iomanager.cpp
        luwu/timer.cpp
        luwu/hook.cpp
        luwu/fd_manager.cpp
        luwu/address.cpp
        luwu/socket.cpp
        luwu/bytearray.cpp
        luwu/tcp_server.cpp
        luwu/streams/stream.cpp
        luwu/streams/socket_stream.cpp
        luwu/utils.cpp
        luwu/util/hash_util.cpp
        luwu/util/json_util.cpp
        luwu/http/http.cpp
        luwu/http/http-parser/http_parser.c
        luwu/http/http_parser.cpp
        luwu/http/http_session.cpp
        luwu/http/http_server.cpp
        luwu/http/servlet.cpp
        luwu/http/http_connection.cpp
        luwu/http/ws_connection.cpp
        luwu/http/ws_server.cpp
        luwu/http/ws_servlet.cpp
        luwu/http/ws_session.cpp
        luwu/uri.cpp
        luwu/ext.cpp
        luwu/daemon.cpp
        luwu/env.cpp
        luwu/application.cpp
        luwu/library.cpp
        luwu/module.cpp
        luwu/main.cpp)

add_library(luwu SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(luwu)

set(LIBS
        luwu
        pthread
        dl
        yaml-cpp
        jsoncpp
        ${OPENSSL_LIBRARIES}
        )

if (BUILD_TEST)
    luwu_add_executable(test_log "tests/test_log.cpp" luwu "${LIBS}")
    luwu_add_executable(test_util "tests/test_utils.cpp" luwu "${LIBS}")
    luwu_add_executable(test_env "tests/test_env.cpp" luwu "${LIBS}")
    luwu_add_executable(test_config "tests/test_config.cpp" luwu "${LIBS}")
    luwu_add_executable(test_thread "tests/test_thread.cpp" luwu "${LIBS}")
    luwu_add_executable(test_fiber "tests/test_fiber.cpp" luwu "${LIBS}")
    luwu_add_executable(test_scheduler "tests/test_scheduler.cpp" luwu "${LIBS}")
    luwu_add_executable(test_iomanager "tests/test_iomanager.cpp" luwu "${LIBS}")
    luwu_add_executable(test_timer "tests/test_timer.cpp" luwu "${LIBS}")
    luwu_add_executable(test_hook "tests/test_hook.cpp" luwu "${LIBS}")
    luwu_add_executable(test_address "tests/test_address.cpp" luwu "${LIBS}")
    luwu_add_executable(test_bytearray "tests/test_bytearray.cpp" luwu "${LIBS}")
    luwu_add_executable(test_tcp_server "tests/test_tcp_server.cpp" luwu "${LIBS}")
    luwu_add_executable(test_http "tests/test_http.cpp" luwu "${LIBS}")
    luwu_add_executable(test_http_parser "tests/test_http_parser.cpp" luwu "${LIBS}")
    luwu_add_executable(test_http_server "tests/test_http_server.cpp" luwu "${LIBS}")
    luwu_add_executable(test_uri "tests/test_uri.cpp" luwu "${LIBS}")
    luwu_add_executable(test_http_connection "tests/test_http_connection.cpp" luwu "${LIBS}")
    luwu_add_executable(test_daemon "tests/test_daemon.cpp" luwu "${LIBS}")
    luwu_add_executable(my_http_server "tests/my_http_server.cpp" luwu "${LIBS}")
endif ()

luwu_add_executable(bin_luwu "luwu/main.cpp" luwu "${LIBS}")
set_target_properties(bin_luwu PROPERTIES OUTPUT_NAME "luwu")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)