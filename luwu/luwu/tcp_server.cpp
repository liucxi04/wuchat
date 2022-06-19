//
// Created by liucxi on 2022/6/8.
//
#include "tcp_server.h"
#include "macro.h"

namespace liucxi {
    static Logger::ptr g_logger = LUWU_LOG_NAME("system");

    TCPServer::TCPServer(IOManager *worker, IOManager *accept)
            : m_worker(worker), m_accept(accept), m_recvTimeout(60 * 1000 * 2), m_name("luwu/1.0.0"), m_stop(true) {
    }

    TCPServer::~TCPServer() {
        for (auto &sock: m_socks) {
            sock->close();
        }
        m_socks.clear();
    }

    bool TCPServer::bind(Address::ptr address) {
        Socket::ptr sock = Socket::CreateTCP(address);
        if (!sock->bind(address)) {
            LUWU_LOG_ERROR(g_logger) << "bind filed errno=" << errno
                                     << " errstr=" << strerror(errno)
                                     << "addr=[" << address->toString() << "";
            return false;
        }
        if (!sock->listen()) {
            LUWU_LOG_ERROR(g_logger) << "listen filed errno=" << errno
                                     << " errstr=" << strerror(errno)
                                     << "addr=[" << address->toString() << "";
            return false;
        }
        m_socks.push_back(sock);
        return true;
    }

    bool TCPServer::bind(const std::vector<Address::ptr> &addresses) {
        for (auto &addr: addresses) {
            if (!bind(addr)) {
                m_socks.clear();
                return false;
            }
        }
        return true;
    }

    void TCPServer::startAccept(Socket::ptr sock) {
        while (!m_stop) {
            Socket::ptr client = sock->accept();
            if (client) {
                client->setRecvTimeout(m_recvTimeout);
                m_worker->scheduler([server = shared_from_this(), client] {
                    server->handleClient(client);
                });
            } else {
                LUWU_LOG_ERROR(g_logger) << "accept error, errno=" << errno
                                         << " errstr=" << strerror(errno);
            }
        }
    }

    bool TCPServer::start() {
        if (!m_stop) {
            return true;
        }
        m_stop = false;
        for (auto &sock: m_socks) {
            m_accept->scheduler([server = shared_from_this(), sock] {
                server->startAccept(sock);
            });
        }
        return true;
    }

    void TCPServer::stop() {
        m_stop = true;
        auto self = shared_from_this();
        m_accept->scheduler([this, self]() {
            for (auto &sock: m_socks) {
                sock->cancelAll();
                sock->close();
            }
            m_socks.clear();
        });
    }

    void TCPServer::handleClient(Socket::ptr client) {
        LUWU_LOG_INFO(g_logger) << "handleClient" << *client;
    }

    std::string TCPServer::toString() {
        std::stringstream ss;
        ss << "[name = " << m_name
           << " worker = " << (m_worker ? m_worker->getName() : "")
           << " recv_timeout = " << m_recvTimeout
           << "]" << std::endl;
        for (auto &sock: m_socks) {
            ss << "sock " << *sock << std::endl;
        }
        return ss.str();
    }

}
