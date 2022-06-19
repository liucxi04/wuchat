//
// Created by liucxi on 2022/6/2.
//
#include "socket.h"
#include "macro.h"
#include "hook.h"
#include "iomanager.h"
#include "fd_manager.h"

#include <netinet/tcp.h>

namespace liucxi {

    Socket::ptr Socket::CreateTCP() {
        Socket::ptr sock(new Socket(AF_INET, SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP() {
        Socket::ptr sock(new Socket(AF_INET, SOCK_DGRAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCP6() {
        Socket::ptr sock(new Socket(AF_INET6, SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP6() {
        Socket::ptr sock(new Socket(AF_INET6, SOCK_DGRAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCP(const Address::ptr& addr) {
        Socket::ptr sock(new Socket(addr->getFamily(), SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(const Address::ptr& addr) {
        Socket::ptr sock(new Socket(addr->getFamily(), SOCK_DGRAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixTCP() {
        Socket::ptr sock(new Socket(AF_UNIX, SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixUDP() {
        Socket::ptr sock(new Socket(AF_UNIX, SOCK_DGRAM, 0));
        return sock;
    }

    Socket::Socket(int family, int type, int protocol)
            : m_sock(-1), m_family(family), m_type(type), m_protocol(protocol), m_isConnected(false) {
    }

    Socket::~Socket() {
        close();
    }

    uint64_t Socket::getSendTimeout() const {
        FdContext::ptr ctx = FdMgr::getInstance()->get(m_sock);
        if (ctx) {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void Socket::setSendTimeout(uint64_t timeout) {
        struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    uint64_t Socket::getRecvTimeout() const {
        FdContext::ptr ctx = FdMgr::getInstance()->get(m_sock);
        if (ctx) {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    void Socket::setRecvTimeout(uint64_t timeout) {
        struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(int level, int option, void *result, size_t *len) const {
        int rt = getsockopt(m_sock, level, option, result, (socklen_t *) len);
        if (rt) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "Socket::getOption sock=" << m_sock
                                                    << " level=" << level << " option=" << option
                                                    << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void *result, size_t len) const {
        int rt = setsockopt(m_sock, level, option, result, (socklen_t) len);
        if (rt) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "Socket::setOption sock=" << m_sock
                                                    << " level=" << level << " option=" << option
                                                    << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept() const {
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        int newFd = ::accept(m_sock, nullptr, nullptr);
        if (newFd == -1) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "accept(" << m_sock << ") errno="
                                                    << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        if (sock->init(newFd)) {
            return sock;
        }
        return nullptr;
    }

    bool Socket::bind(const Address::ptr &addr) {
        if (!isValid()) {
            newSock();
        }

        if (addr->getFamily() != m_family) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "bind sock.family("
                                                    << m_family << ") addr.family(" << addr->getFamily()
                                                    << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "bind error"
                                                    << ", errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return false;
        }
        getLocalAddress();
        return true;
    }

    bool Socket::connect(const Address::ptr &addr, uint64_t timeout) {
        m_remoteAddress = addr;
        if (!isValid()) {
            newSock();
        }

        if (addr->getFamily() != m_family) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "connect sock.family("
                                                    << m_family << ") addr.family(" << addr->getFamily()
                                                    << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (timeout == (uint64_t)-1) {
            if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
                LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "sock=" << m_sock << " connect(" << addr->toString()
                                                        << ") error errno=" << errno
                                                        << " errstr=" << strerror(errno);
                close();
                return false;
            }
        } else {
            if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout)) {
                LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "sock=" << m_sock << " connect(" << addr->toString()
                                                        << ") timeout=" << timeout
                                                        << " error errno=" << errno
                                                        << " errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::reconnect(uint64_t timeout) {
        if (!m_remoteAddress) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "reconnect remote address is null";
            return false;
        }
        m_localAddress.reset();
        return connect(m_remoteAddress, timeout);
    }

    bool Socket::listen(int backlog) const {
        if (!isValid()) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "listen error sock=-1";
            return false;
        }
        if (::listen(m_sock, backlog)) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "listen error errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close() {
        if (!m_isConnected && m_sock == -1) {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1) {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    size_t Socket::send(const void *buffer, size_t length, int flags) const {
        if (isConnected()) {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }

    size_t Socket::send(const iovec *buffer, size_t length, int flags) const {
        if(isConnected()) {
            msghdr msg{};
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    size_t Socket::sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags) const {
        if (isConnected()) {
            return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
        }
        return -1;
    }

    size_t Socket::sendTo(const iovec *buffer, size_t length, const Address::ptr &to, int flags) const {
        if(isConnected()) {
            msghdr msg{};
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            msg.msg_name = to->getAddr();
            msg.msg_namelen = to->getAddrLen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    size_t Socket::recv(void *buffer, size_t length, int flags) {
        if (isConnected()) {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }

    size_t Socket::recv(iovec *buffer, size_t length, int flags) {
        if(isConnected()) {
            msghdr msg{};
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    size_t Socket::recvFrom(void *buffer, size_t length, const Address::ptr &from, int flags) {
        if (isConnected()) {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }

    size_t Socket::recvFrom(iovec *buffer, size_t length, const Address::ptr &from, int flags) {
        if(isConnected()) {
            msghdr msg{};
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrLen();
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    Address::ptr Socket::getRemoteAddress() {
        if (m_remoteAddress) {
            return m_remoteAddress;
        }

        Address::ptr result;
        switch (m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addlen = result->getAddrLen();
        if (getpeername(m_sock, result->getAddr(), &addlen)) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "getpeername error sock=" << m_sock
                                                    << " errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    Address::ptr Socket::getLocalAddress() {
        if (m_localAddress) {
            return m_localAddress;
        }

        Address::ptr result;
        switch (m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addlen = result->getAddrLen();
        if (getsockname(m_sock, result->getAddr(), &addlen)) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "getsockname error sock=" << m_sock
                                                    << " errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    int Socket::getError() const {
        int error = 0;
        size_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
            return -1;
        }
        return error;
    }

    std::ostream &Socket::dump(std::ostream &os) const {
        os << "[Socket sock=" << m_sock
           << " is_connected=" << m_isConnected
           << " family=" << m_family
           << " type=" << m_type
           << " protocol=" << m_protocol;
        if (m_localAddress) {
            os << " local_address=" << m_localAddress->toString();
        }
        if (m_remoteAddress) {
            os << " remote_address=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    bool Socket::cancelRead() const {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
    }

    bool Socket::cancelWrite() const {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
    }

    bool Socket::cancelAccept() const {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
    }

    bool Socket::cancelAll() const {
        return IOManager::GetThis()->cancelAll(m_sock);
    }

    bool Socket::init(int sock) {
        FdContext::ptr ctx = FdMgr::getInstance()->get(sock);
        if (ctx && ctx->isSocket() && !ctx->isClose()) {
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getRemoteAddress();
            getLocalAddress();
            return true;
        }
        return false;
    }

    void Socket::initSock() {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM) {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }

    void Socket::newSock() {
        m_sock = socket(m_family, m_type, m_protocol);
        if (m_sock != -1) {
            initSock();
        } else {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "socket(" << m_family
                                                    << " ," << m_type << " ," << m_protocol << ") errno="
                                                    << errno << " errstr=" << strerror(errno);
        }
    }

    std::ostream &operator<<(std::ostream &os, const Socket &socket) {
        return socket.dump(os);
    }

}
