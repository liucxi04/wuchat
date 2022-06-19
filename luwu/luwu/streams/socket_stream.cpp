//
// Created by liucxi on 2022/6/10.
//

#include "socket_stream.h"

#include <utility>

namespace liucxi {

    SocketStream::SocketStream(Socket::ptr socket, bool owner)
        : m_socket(std::move(socket))
        , m_owner(owner){
    }

    SocketStream::~SocketStream() {
        if (m_owner && m_socket) {
            m_socket->close();
        }
    }

    bool SocketStream::isConnected() const {
        return m_socket && m_socket->isConnected();
    }

    size_t SocketStream::read(void *buffer, size_t length) {
        if (!isConnected()) {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

    size_t SocketStream::read(ByteArray::ptr ba, size_t length) {
        if (!isConnected()) {
            return -1;
        }
        std::vector<iovec> ios;
        ba->getWriteBuffers(ios, length);
        size_t rt = m_socket->recv(&ios[0], ios.size());
        if (rt > 0) {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    size_t SocketStream::write(const void *buffer, size_t length) {
        if (!isConnected()) {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

    size_t SocketStream::write(ByteArray::ptr ba, size_t length) {
        if (!isConnected()) {
            return -1;
        }
        std::vector<iovec> ios;
        ba->getReadBuffers(ios, length);
        size_t rt = m_socket->send(&ios[0], length);
        if (rt > 0) {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    void SocketStream::close() {
        if (m_socket) {
            m_socket->close();
        }
    }
}