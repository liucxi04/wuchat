//
// Created by liucxi on 2022/6/8.
//

#include <unistd.h>
#include "luwu/macro.h"
#include "luwu/tcp_server.h"
#include "luwu/socket.h"
#include "luwu/bytearray.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

class EchoServer : public liucxi::TCPServer {
    void handleClient(const liucxi::Socket::ptr &client) override;
};

void EchoServer::handleClient(const liucxi::Socket::ptr &client) {
    LUWU_LOG_INFO(g_logger) << "handleClient";
    liucxi::ByteArray::ptr ba(new liucxi::ByteArray);

    while (true) {
        ba->clear();
        std::vector<iovec> ios;
        ba->getWriteBuffers(ios, 1024);

        size_t rt = client->recv(&ios[0], ios.size());

        if (rt == 0) {
            LUWU_LOG_INFO(g_logger) << "client close";
            break;
        } else if (rt < 0) {
            LUWU_LOG_ERROR(g_logger) << "client error rt = " << rt;
            break;
        }

        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        LUWU_LOG_INFO(g_logger) << ba->toString();
        std::cout.flush();
    }
}

void run() {
    EchoServer::ptr  es(new EchoServer);
    auto addr = liucxi::Address::LookupAny("0.0.0.0:1234");
    while (!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main() {
    liucxi::IOManager iom;
    iom.scheduler(run);
    return 0;
}