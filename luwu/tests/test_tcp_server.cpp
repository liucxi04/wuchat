#include "luwu/tcp_server.h"
#include "luwu/macro.h"
#include "luwu/config.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

/**
 * @brief 自定义TcpServer类，重载handleClient方法
 */
class MyTcpServer : public liucxi::TCPServer {
protected:
    void handleClient(liucxi::Socket::ptr client) override;
};

void MyTcpServer::handleClient(liucxi::Socket::ptr client) {
    LUWU_LOG_INFO(g_logger) << "new client: " << *client;
    static std::string buf;
    buf.resize(4096);
    size_t len = client->recv(&buf[0], buf.length()); // 这里有读超时，由tcp_server.read_timeout配置项进行配置，默认120秒
    buf.resize(len);
    LUWU_LOG_INFO(g_logger) << "recv: " << buf;
    client->close();
}

void run() {
    liucxi::TCPServer::ptr server(new MyTcpServer); // 内部依赖shared_from_this()，所以必须以智能指针形式创建对象
//    liucxi::TCPServer::ptr server(new liucxi::TCPServer);
    auto addr = liucxi::Address::LookupAny("0.0.0.0:12344");
    LUWU_LOG_INFO(g_logger) << "addr " << *addr;
    LUWU_ASSERT(addr)

    if (!server->bind(addr)) {
        LUWU_LOG_ERROR(g_logger) << "bind error";
    }

    LUWU_LOG_INFO(g_logger) << "bind success, " << server->toString();

    if (server->start()) {
        std::cout << "server start" << std::endl;
    }
}

int main(int argc, char *argv[]) {

    liucxi::IOManager iom(1, true, "tcp");
    iom.scheduler(&run);

    return 0;
}