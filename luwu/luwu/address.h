//
// Created by liucxi on 2022/6/1.
//

#ifndef LUWU_ADDRESS_H
#define LUWU_ADDRESS_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <map>

namespace liucxi {

    class IPAddress;

    /**
     * @brief 所有地址的基类，抽象类
     */
    class Address {
    public:
        typedef std::shared_ptr<Address> ptr;

        /**
         * @brief 通过 sockaddr* 创建 Address
         */
        static Address::ptr Create(const sockaddr *addr);

        /**
         * @brief 通过 host 找到所有满足条件的 Address
         * @param results 保存满足条件的所有地址
         * @param host 域名，服务器名，如 www.liucxi.xyz[:80]，括号内为可选内容
         */
        static bool Lookup(std::vector<Address::ptr> &results,
                           const std::string &host, int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 通过 host 找到任意一个满足条件的 Address
         */
        static Address::ptr LookupAny(const std::string &host,
                                      int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 通过 host 找到任意一个满足条件的 IPAddress
         */
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
                                                             int family = AF_INET, int type = 0, int protocol = 0);

        /**
         * @brief 获取本机所有网卡的网卡名、地址、子网掩码位数
         */
        static bool GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &results,
                                        int family = AF_INET);

        /**
         * @brief 获取指定网卡的地址、子网掩码位数
         * @param face 网卡名称
         */
        static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &results,
                                        const std::string &face, int family = AF_INET);
        ~Address() = default;

        int getFamily() const;

        /**
         * @brief 返回 sockaddr 指针，只读
         */
        virtual const sockaddr *getAddr() const = 0;

        /**
         * @brief 返回 sockaddr 指针，读写
         */
        virtual sockaddr *getAddr() = 0;

        virtual socklen_t getAddrLen() const = 0;

        /**
         * @brief 将数据写入 ostream，配合 toString
         */
        virtual std::ostream &dump(std::ostream &os) const = 0;

        std::string toString() const;

        bool operator<(const Address &rhs) const;

        bool operator==(const Address &rhs) const;

        bool operator!=(const Address &rhs) const;
    };

    class IPAddress : public Address {
    public:
        typedef std::shared_ptr<IPAddress> ptr;

        /**
         * @brief 通过域名(服务器名)，端口创建 IPAddress
         */
        static IPAddress::ptr Create(const char *address, uint16_t port = 0);

        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

        virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

        virtual uint16_t getPort() const = 0;

        virtual void setPort(uint16_t p) = 0;
    };

    class IPv4Address : public IPAddress {
    public:
        typedef std::shared_ptr<IPv4Address> ptr;

        /**
         * @brief 通过点分十进制创建 IPv4Address
         */
        static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

        /**
         * @brief 通过 sockaddr_in 创建 IPv4Address
         */
        explicit IPv4Address(const sockaddr_in &addr);

        /**
         * @brief 通过 32 位二进制创建 IPv4Address
         */
        explicit IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &dump(std::ostream &os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

        IPAddress::ptr networkAddress(uint32_t prefix_len) override;

        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        uint16_t getPort() const override;

        void setPort(uint16_t p) override;

    private:
        sockaddr_in m_addr{};
    };

    class IPv6Address : public IPAddress {
    public:
        typedef std::shared_ptr<IPv6Address> ptr;

        /**
         * @brief 通过 IPv6 地址字符串和端口构造 IPv6Address
         */
        static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

        IPv6Address();

        /**
         * @brief 通过 sockaddr_in6 构造 IPv6Address
         */
        explicit IPv6Address(const sockaddr_in6 &addr);

        /**
         * @brief 通过 IPv6 二进制地址和端口构造 IPv6Address
         */
        explicit IPv6Address(const uint8_t address[16], uint16_t port = 0);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &dump(std::ostream &os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

        IPAddress::ptr networkAddress(uint32_t prefix_len) override;

        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        uint16_t getPort() const override;

        void setPort(uint16_t p) override;

    private:
        sockaddr_in6 m_addr{};
    };

    class UnixAddress : public Address {
    public:
        typedef std::shared_ptr<UnixAddress> ptr;

        UnixAddress();

        /**
         * @brief 通过路径构造 UnixAddress
         */
        explicit UnixAddress(const std::string &path);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        void setAddrLen(socklen_t v);

        std::string getPath() const;

        std::ostream &dump(std::ostream &os) const override;

    private:
        sockaddr_un m_addr{};
        socklen_t m_length{};
    };

    class UnknownAddress : public Address {
    public:
        typedef std::shared_ptr<UnknownAddress> ptr;

        explicit UnknownAddress(const sockaddr &addr);

        explicit UnknownAddress(int family);

        const sockaddr *getAddr() const override;

        sockaddr *getAddr() override;

        socklen_t getAddrLen() const override;

        std::ostream &dump(std::ostream &os) const override;

    private:
        sockaddr m_addr{};
    };

    std::ostream &operator<<(std::ostream &os, const Address &addr);
}
#endif //LUWU_ADDRESS_H
