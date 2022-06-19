//
// Created by liucxi on 2022/6/3.
//

#include "luwu/iomanager.h"
#include "luwu/socket.h"
#include "luwu/macro.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

void test_socket() {
    liucxi::IPAddress::ptr addr = liucxi::Address::LookupAnyIPAddress("www.baidu.com");

    if (addr) {
        LUWU_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LUWU_LOG_ERROR(g_logger) << "get addr filed";
        return;
    }

    liucxi::Socket::ptr sock = liucxi::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)) {
        LUWU_LOG_ERROR(g_logger) << "connect " << addr->toString() << " error";
        return;
    } else {
        LUWU_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buf[] = "GET / HTTP/1.0\r\n\r\n";
    ssize_t rt = sock->send(buf, sizeof(buf));
    if (rt <= 0) {
        LUWU_LOG_ERROR(g_logger) << "send filed, rt=" << rt;
        return;
    }

    std::string buffer;
    buffer.resize(4096);
    rt = sock->recv(&buffer[0], buffer.size());
    if (rt <= 0) {
        LUWU_LOG_ERROR(g_logger) << "recv filed, rt=" << rt;
        return;
    }

    buffer.resize(rt);
    std::cout << buffer;
}

int main() {
    liucxi::IOManager iom;
    iom.scheduler(test_socket);
    return 0;
}

