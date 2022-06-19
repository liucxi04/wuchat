//
// Created by liucxi on 2022/6/1.
//
#include "address.h"
//#include "utils.h"
#include "macro.h"

#include <memory>
#include <sstream>
#include <netdb.h>
#include <errno.h>
#include <ifaddrs.h>

namespace liucxi {

    template<typename T>
    static T CreateMask(uint32_t bits) {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }

    template<typename T>
    static uint32_t CountBytes(T value) {
        uint32_t result = 0;
        for (; value; ++result) {
            value &= value - 1;
        }
        return result;
    }

    Address::ptr Address::Create(const sockaddr *addr) {
        if (addr == nullptr) {
            return nullptr;
        }

        Address::ptr rt;
        switch (addr->sa_family) {
            case AF_INET:
                rt.reset(new IPv4Address(*(const sockaddr_in *) addr));
                break;
            case AF_INET6:
                rt.reset(new IPv6Address(*(const sockaddr_in6 *) addr));
                break;
            default:
                rt.reset(new UnknownAddress(*addr));
                break;
        }
        return rt;
    }

    bool Address::Lookup(std::vector<Address::ptr> &results,
                         const std::string &host, int family, int type, int protocol) {
        addrinfo hints{}, *result, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = nullptr;
        hints.ai_addr = nullptr;
        hints.ai_next = nullptr;

        std::string node;   // IPv6 或 IPv4 的地址
        const char *service = nullptr; // 相当于端口号，IPv6 或 IPv4

        if (!host.empty() && host[0] == '[') {
            const char *endipv6 = strchr(host.c_str(), ']');
            if (endipv6) {
                if (*(endipv6 + 1) == ':') {
                    service = endipv6 + 2;
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        // 到这里 node 为空说明是 IPv4
        if (node.empty()) {
            service = (const char *) memchr(host.c_str(), ':', host.size());
            if (service) {
                if (!strchr(service + 1, ':')) {
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }

        // 到这里为空说明没有 service，即没有端口
        if (node.empty()) {
            node = host;
        }
        // hints 填写了希望返回的信息类型， result 保存了查询的结果
        int error = getaddrinfo(node.c_str(), service, &hints, &result);
        if (error) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "Address::Lookup getaddrinfo(" << host << ","
                                                    << family << "," << type << ") err=" << error << "errstr="
                                                    << gai_strerror(error);
            return false;
        }

        next = result;
        while (next) {
            results.push_back(Create(next->ai_addr));
            next = next->ai_next;
        }

        freeaddrinfo(result);
        return !results.empty();
    }

    Address::ptr Address::LookupAny(const std::string &host, int family, int type, int protocol) {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol)) {
            return result[0];
        }
        return nullptr;
    }

    IPAddress::ptr Address::LookupAnyIPAddress(const std::string &host, int family, int type, int protocol) {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol)) {
            for (auto &i: result) {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v) {
                    return v;
                }
            }
        }
        return nullptr;
    }

    bool Address::GetInterfaceAddress(std::multimap<std::string,
                                                    std::pair<Address::ptr, uint32_t>> &results, int family) {
        struct ifaddrs *result, *next;
        if (getifaddrs(&result) != 0) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "Address::GetInterfaceAddress getifaddrs "
                                                    << "err=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        try {
            for (next = result; next; next = next->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_length = 0u;
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                    continue;
                }
                switch (next->ifa_addr->sa_family) {
                    case AF_INET: {
                        addr = Create(next->ifa_addr);
                        uint32_t netmask = ((sockaddr_in *) next->ifa_netmask)->sin_addr.s_addr;
                        prefix_length = CountBytes(netmask);
                        break;
                    }
                    case AF_INET6: {
                        addr = Create(next->ifa_addr);
                        in6_addr &netmask = ((sockaddr_in6 *) next->ifa_netmask)->sin6_addr;
                        prefix_length = 0;
                        for (int i = 0; i < 16; ++i) {
                            prefix_length += CountBytes(netmask.s6_addr[i]);
                        }
                        break;
                    }
                    default:
                        break;
                }

                if (addr) {
                    results.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_length)));
                }
            }
        } catch (...) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "Address::GetInterfaceAddress exception";
            freeifaddrs(result);
            return false;
        }
        freeifaddrs(result);
        return true;
    }

    bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &results,
                                      const std::string &face, int family) {
        // 没有指定网卡名称
        if (face.empty() || face == "*") {
            if (family == AF_INET || family == AF_UNSPEC) {
                results.emplace_back(Address::ptr(new IPv4Address), 0u);
            }
            if (family == AF_INET6 || family == AF_UNSPEC) {
                results.emplace_back(Address::ptr(new IPv6Address), 0u);
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> result;
        if (!GetInterfaceAddress(result, family)) {
            return false;
        }
        auto it = result.equal_range(face);
        for (; it.first != it.second; ++it.first) {
            results.push_back(it.first->second);
        }
        return true;
    }

    int Address::getFamily() const {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool Address::operator<(const Address &rhs) const {
        socklen_t minLen = std::min(getAddrLen(), rhs.getAddrLen());
        int res = memcmp(getAddr(), rhs.getAddr(), minLen);
        if (res < 0) {
            return true;
        } else if (res > 0) {
            return false;
        } else {
            if (getAddrLen() < rhs.getAddrLen()) {
                return true;
            } else {
                return false;
            }
        }
    }

    bool Address::operator==(const Address &rhs) const {
        return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) != 0;
    }

    bool Address::operator!=(const Address &rhs) const {
        return !(*this == rhs);
    }

    IPAddress::ptr IPAddress::Create(const char *address, uint16_t port) {
        addrinfo hints{}, *results;
        memset(&hints, 0, sizeof(hints));

        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;

        int error = getaddrinfo(address, nullptr, &hints, &results);
        if (error) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "IPAddress::Create(" << address << ", " << port << ") error="
                                                    << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try {
            IPAddress::ptr rt = std::dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr));
            if (rt) {
                rt->setPort(port);
            }
            freeaddrinfo(results);
            return rt;
        } catch (...) {
            freeaddrinfo(results);
            return nullptr;
        }
    }

    IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port) {
        IPv4Address::ptr addr(new IPv4Address);
        addr->m_addr.sin_port = byteSwapOnLittleEndian(port);
        int rt = inet_pton(AF_INET, address, &addr->m_addr.sin_addr);
        if (rt <= 0) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "IPv4Address::Create(" << address << ","
                                                    << port << ") rt = " << rt << " errno=" << errno << " errstr="
                                                    << strerror(errno);
            return nullptr;
        }
        return addr;
    }

    IPv4Address::IPv4Address(const sockaddr_in &addr) {
        m_addr = addr;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteSwapOnLittleEndian(port);
        m_addr.sin_addr.s_addr = byteSwapOnLittleEndian(address);
    }

    const sockaddr *IPv4Address::getAddr() const {
        return (sockaddr *) &m_addr;
    }

    sockaddr *IPv4Address::getAddr() {
        return (sockaddr *) &m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream &IPv4Address::dump(std::ostream &os) const {
        uint32_t address = byteSwapOnLittleEndian(m_addr.sin_addr.s_addr);
        os << ((address >> 24) & 0xff) << "."
           << ((address >> 16) & 0xff) << "."
           << ((address >> 8) & 0xff) << "."
           << (address & 0xff);
        os << ":" << byteSwapOnLittleEndian(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }

        sockaddr_in addr(m_addr);
        addr.sin_addr.s_addr |= byteSwapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(addr);
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }

        sockaddr_in addr(m_addr);
        addr.sin_addr.s_addr &= byteSwapOnLittleEndian(~CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(addr);
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }

        sockaddr_in addr{};
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ~byteSwapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return std::make_shared<IPv4Address>(addr);
    }

    uint16_t IPv4Address::getPort() const {
        return byteSwapOnLittleEndian(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t p) {
        m_addr.sin_port = byteSwapOnLittleEndian(p);
    }

    IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port) {
        IPv6Address::ptr addr(new IPv6Address);
        addr->m_addr.sin6_port = byteSwapOnLittleEndian(port);
        int rt = inet_pton(AF_INET6, address, &addr->m_addr.sin6_addr);
        if (rt <= 0) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "IPv6Address::Create(" << address << ","
                                                    << port << ") rt = " << rt << " errno=" << errno << " errstr="
                                                    << strerror(errno);
            return nullptr;
        }
        return addr;
    }

    IPv6Address::IPv6Address() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6 &addr) {
        m_addr = addr;
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteSwapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
    }

    const sockaddr *IPv6Address::getAddr() const {
        return (sockaddr *) &m_addr;
    }

    sockaddr *IPv6Address::getAddr() {
        return (sockaddr *) &m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream &IPv6Address::dump(std::ostream &os) const {
        os << "[";
        auto addr = (uint16_t *) m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (size_t i = 0; i < 8; ++i) {
            if (addr[i] == 0 && !used_zeros) {
                continue;
            }
            if (i && addr[i - 1] == 0 && !used_zeros) {
                os << ":";
                used_zeros = true;
            }
            if (i) {
                os << ":";
            }
            os << std::hex << (int) byteSwapOnLittleEndian(addr[i]) << std::dec;
        }

        if (!used_zeros && addr[7] == 0) {
            os << "::";
        }
        os << "]:" << byteSwapOnLittleEndian(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
        sockaddr_in6 addr(m_addr);
        addr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
        for (unsigned int i = prefix_len / 8 + 1; i < 16; ++i) {
            addr.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(addr);
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
        sockaddr_in6 addr(m_addr);
        addr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
        for (unsigned int i = prefix_len / 8 + 1; i < 16; ++i) {
            addr.sin6_addr.s6_addr[i] = 0x00;
        }
        return std::make_shared<IPv6Address>(addr);
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in6 addr{};
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
        for (unsigned int i = 0; i < prefix_len / 8; ++i) {
            addr.sin6_addr.s6_addr[i] = 0xff;
        }
        return std::make_shared<IPv6Address>(addr);
    }

    uint16_t IPv6Address::getPort() const {
        return byteSwapOnLittleEndian(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t p) {
        m_addr.sin6_port = byteSwapOnLittleEndian(p);
    }

    static const size_t MAX_PATH_LENGTH = sizeof(((sockaddr_un *) nullptr)->sun_path) - 1;

    UnixAddress::UnixAddress() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LENGTH;
    }

    UnixAddress::UnixAddress(const std::string &path) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;

        if (!path.empty() && path[0] == '\0') {
            --m_length;
        }

        if (m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }

        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    const sockaddr *UnixAddress::getAddr() const {
        return (sockaddr *) &m_addr;
    }

    sockaddr *UnixAddress::getAddr() {
        return (sockaddr *) &m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const {
        return m_length;
    }

    void UnixAddress::setAddrLen(socklen_t v) {
        m_length = v;
    }

    std::string UnixAddress::getPath() const {
        std::stringstream ss;
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
            ss << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
        } else {
            ss << m_addr.sun_path;
        }
        return ss.str();
    }

    std::ostream &UnixAddress::dump(std::ostream &os) const {
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                                              m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }

    UnknownAddress::UnknownAddress(const sockaddr &addr) {
        m_addr = addr;
    }

    UnknownAddress::UnknownAddress(int family) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    const sockaddr *UnknownAddress::getAddr() const {
        return &m_addr;
    }

    sockaddr *UnknownAddress::getAddr() {
        return &m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream &UnknownAddress::dump(std::ostream &os) const {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    std::ostream &operator<<(std::ostream &os, const Address &addr) {
        return addr.dump(os);
    }
}
