//
// Created by liucxi on 2022/6/2.
//

#ifndef LUWU_SOCKET_H
#define LUWU_SOCKET_H

#include "noncopyable.h"
#include "address.h"
#include <memory>
#include <vector>

namespace liucxi {
    /**
     * @brief Socket 封装
     */
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
    public:
        typedef std::shared_ptr<Socket> ptr;

        static Socket::ptr CreateTCP();

        static Socket::ptr CreateUDP();

        static Socket::ptr CreateTCP6();

        static Socket::ptr CreateUDP6();

        static Socket::ptr CreateUnixTCP();

        static Socket::ptr CreateUnixUDP();

        /**
         * @brief 创建 TCP socket，与给定的 addr 的协议簇类型相同
         */
        static Socket::ptr CreateTCP(const Address::ptr &addr);

        /**
         * @brief 创建 UDP socket，与给定的 addr 的协议簇类型相同
         */
        static Socket::ptr CreateUDP(const Address::ptr &addr);

        Socket(int family, int type, int protocol = 0);

        ~Socket();

        uint64_t getSendTimeout() const;

        void setSendTimeout(uint64_t timeout);

        uint64_t getRecvTimeout() const;

        void setRecvTimeout(uint64_t timeout);

        bool getOption(int level, int option, void *result, size_t *len) const;

        template<typename T>
        bool getOption(int level, int option, T &value) {
            ssize_t len = sizeof(T);
            return getOption(level, option, &value, &len);
        }

        bool setOption(int level, int option, const void *result, size_t len) const;

        template<typename T>
        bool setOption(int level, int option, const T &value) {
            return setOption(level, option, &value, sizeof(T));
        }

        Socket::ptr accept() const;

        bool bind(const Address::ptr &addr);

        bool connect(const Address::ptr &addr, uint64_t timeout = -1);

        bool reconnect(uint64_t timeout = -1);

        bool close();

        bool listen(int backlog = SOMAXCONN) const;

        size_t send(const void *buffer, size_t length, int flags = 0) const;

        size_t send(const iovec *buffer, size_t length, int flags = 0) const;

        size_t sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags = 0) const;

        size_t sendTo(const iovec *buffer, size_t length, const Address::ptr &to, int flags = 0) const;

        size_t recv(void *buffer, size_t length, int flags = 0);

        size_t recv(iovec *buffer, size_t length, int flags = 0);

        size_t recvFrom(void *buffer, size_t length, const Address::ptr &from, int flags = 0);

        size_t recvFrom(iovec *buffer, size_t length, const Address::ptr &from, int flags = 0);

        /**
         * @brief 获取远端地址，使用 getpeername
         */
        Address::ptr getRemoteAddress();

        /**
         * @brief 获取本地地址，使用 getsockname
         */
        Address::ptr getLocalAddress();

        int getSocket() const { return m_sock; }

        int getFamily() const { return m_family; }

        int getType() const { return m_type; }

        int getProtocol() const { return m_protocol; }

        bool isConnected() const { return m_isConnected; }

        bool isValid() const { return m_sock != -1; }

        int getError() const;

        std::ostream &dump(std::ostream &os) const;

        bool cancelRead() const;

        bool cancelWrite() const;

        bool cancelAccept() const;

        bool cancelAll() const;

    private:

        /**
         * @brief 初始化 socket
         * @details 设置非阻塞和重用，被下面两个函数调用
         */
        void initSock();

        /**
         * @brief 使用一个套接字描述符 sock 来初始化
         */
        bool init(int sock);

        /**
         * @brief 创建 socket
         * @details 调用系统函数初始化 m_sock
         */
        void newSock();

    private:
        int m_sock;
        int m_family;
        int m_type;
        int m_protocol;
        bool m_isConnected;

        Address::ptr m_localAddress;        /// 本地地址
        Address::ptr m_remoteAddress;       /// 远端地址
    };

    std::ostream &operator<<(std::ostream &os, const Socket &socket);
}
#endif //LUWU_SOCKET_H
