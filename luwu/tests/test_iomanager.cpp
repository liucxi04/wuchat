//
// Created by liucxi on 2022/4/26.
//
#include "luwu/log.h"
#include "luwu/iomanager.h"
#include "luwu/macro.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

int sockfd;
void watch_io_read();

// 写事件回调，只执行一次，用于判断非阻塞套接字connect成功
void do_io_write() {
    LUWU_LOG_INFO(g_logger) << "write callback";
    int so_err;
    socklen_t len = size_t(so_err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if(so_err) {
        LUWU_LOG_INFO(g_logger) << "connect fail";
        return;
    }
    LUWU_LOG_INFO(g_logger) << "connect success";
}

// 读事件回调，每次读取之后如果套接字未关闭，需要重新添加
void do_io_read() {
    LUWU_LOG_INFO(g_logger) << "read callback";
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd, buf, sizeof(buf));
    if(readlen > 0) {
        buf[readlen] = '\0';
        LUWU_LOG_INFO(g_logger) << "read " << readlen << " bytes, read: " << buf;
    } else if(readlen == 0) {
        LUWU_LOG_INFO(g_logger) << "peer closed";
        close(sockfd);
        return;
    } else {
        LUWU_LOG_ERROR(g_logger) << "err, errno=" << errno << ", errstr=" << strerror(errno);
        close(sockfd);
        return;
    }
    // read之后重新添加读事件回调，这里不能直接调用addEvent，因为在当前位置fd的读事件上下文还是有效的，直接调用addEvent相当于重复添加相同事件
    liucxi::IOManager::GetThis()->scheduler(watch_io_read);
}

void watch_io_read() {
    LUWU_LOG_INFO(g_logger) << "watch_io_read";
    liucxi::IOManager::GetThis()->addEvent(sockfd, liucxi::IOManager::READ, do_io_read);
}

void test_io() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    LUWU_ASSERT(sockfd > 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1234);
    inet_pton(AF_INET, "10.10.19.159", &servaddr.sin_addr.s_addr);

    int rt = connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr));
    if(rt != 0) {
        if(errno == EINPROGRESS) {
            LUWU_LOG_INFO(g_logger) << "EINPROGRESS";
            // 注册写事件回调，只用于判断connect是否成功
            // 非阻塞的TCP套接字connect一般无法立即建立连接，要通过套接字可写来判断connect是否已经成功
            liucxi::IOManager::GetThis()->addEvent(sockfd, liucxi::IOManager::WRITE, do_io_write);
            // 注册读事件回调，注意事件是一次性的
            liucxi::IOManager::GetThis()->addEvent(sockfd, liucxi::IOManager::READ, do_io_read);
        } else {
            LUWU_LOG_ERROR(g_logger) << "connect error, errno:" << errno << ", errstr:" << strerror(errno);
        }
    } else {
        LUWU_LOG_ERROR(g_logger) << "else, errno:" << errno << ", errstr:" << strerror(errno);
    }
}

int main() {
    liucxi::IOManager iom;
//     liucxi::IOManager iom(3); // 演示多线程下IO协程在不同线程之间切换
    iom.scheduler(test_io);
    return 0;
}

