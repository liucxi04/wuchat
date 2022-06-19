cmake_minimum_required(VERSION 3.10)
project(luwu)

find_package(Threads)
find_package(Boost REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(luwu ${Boost_INCLUDE_DIRS} ${YAML_CPP_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIRS} ${YAML_CPP_LIBRARY_DIR})

set(SRC
        luwu/log.cpp
        luwu/utils.cpp
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
        luwu/http/http.cpp
        luwu/http/http-parser/http_parser.c
        luwu/http/http_parser.cpp
        luwu/tcp_server.cpp
        luwu/streams/stream.cpp
        luwu/streams/socket_stream.cpp
        luwu/http/http_session.cpp
        luwu/http/http_server.cpp
        luwu/http/servlet.cpp
        luwu/http/http_connection.cpp
        luwu/uri.cpp
        luwu/ext.cpp
        luwu/daemon.cpp
        luwu/env.cpp
        luwu/application.cpp)

add_library(luwu SHARED ${SRC})


add_executable(log tests/test_log.cpp)
add_dependencies(log luwu)
target_link_libraries(log luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp)

add_executable(config tests/test_config.cpp)
add_dependencies(config luwu)
target_link_libraries(config luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(thread tests/test_thread.cpp)
add_dependencies(thread luwu)
target_link_libraries(thread luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp)

add_executable(utils tests/test_utils.cpp)
add_dependencies(utils luwu)
target_link_libraries(utils luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp)

add_executable(fiber tests/test_fiber.cpp)
add_dependencies(fiber luwu)
target_link_libraries(fiber luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(scheduler tests/test_scheduler.cpp)
add_dependencies(scheduler luwu)
target_link_libraries(scheduler luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp)

add_executable(iomanager tests/test_iomanager.cpp)
add_dependencies(iomanager luwu)
target_link_libraries(iomanager luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_timer tests/test_timer.cpp)
add_dependencies(test_timer luwu)
target_link_libraries(test_timer luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp)

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook luwu)
target_link_libraries(test_hook luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address luwu)
target_link_libraries(test_address luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket luwu)
target_link_libraries(test_socket luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray luwu)
target_link_libraries(test_bytearray luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http luwu)
target_link_libraries(test_http luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser luwu)
target_link_libraries(test_http_parser luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server luwu)
target_link_libraries(test_tcp_server luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(echo_server tests/echo_server.cpp)
add_dependencies(echo_server luwu)
target_link_libraries(echo_server luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server luwu)
target_link_libraries(test_http_server luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_http_connection tests/test_http_connection.cpp)
add_dependencies(test_http_connection luwu)
target_link_libraries(test_http_connection luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_uri tests/test_uri.cpp)
add_dependencies(test_uri luwu)
target_link_libraries(test_uri luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(my_http_server tests/my_http_server.cpp)
add_dependencies(my_http_server luwu)
target_link_libraries(my_http_server luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(http_server tests/http_server.cpp)
add_dependencies(http_server luwu)
target_link_libraries(http_server luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test tests/test.cpp)
add_dependencies(test luwu)
target_link_libraries(test luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_daemon tests/test_daemon.cpp)
add_dependencies(test_daemon luwu)
target_link_libraries(test_daemon luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_env tests/test_env.cpp)
add_dependencies(test_env luwu)
target_link_libraries(test_env luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)

add_executable(test_application tests/test_application.cpp)
add_dependencies(test_application luwu)
target_link_libraries(test_application luwu ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARICES} yaml-cpp dl)