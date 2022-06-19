//
// Created by liucxi on 2022/6/8.
//

#ifndef LUWU_TCP_SERVER_H
#define LUWU_TCP_SERVER_H

#include <memory>
#include <utility>
#include "iomanager.h"
#include "address.h"
#include "socket.h"
#include "config.h"

namespace liucxi {

    struct TcpServerConf {
        typedef std::shared_ptr<TcpServerConf> ptr;

        std::vector<std::string> address;
        int keepalive = 0;
        int timeout = 1000 * 2 * 60;
        /// 服务器类型，http, ws, rock
        std::string type = "http";
        std::string name;

        bool isValid() const {
            return !address.empty();
        }

        bool operator==(const TcpServerConf& oth) const {
            return address == oth.address
                   && keepalive == oth.keepalive
                   && timeout == oth.timeout
                   && name == oth.name
                   && type == oth.type;
        }
    };

    template<>
    class LexicalCast<std::string, TcpServerConf> {
    public:
        TcpServerConf operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            TcpServerConf conf;
            conf.type = node["type"].as<std::string>(conf.type);
            conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
            conf.timeout = node["timeout"].as<int>(conf.timeout);
            conf.name = node["name"].as<std::string>(conf.name);
            if(node["address"].IsDefined()) {
                for(size_t i = 0; i < node["address"].size(); ++i) {
                    conf.address.push_back(node["address"][i].as<std::string>());
                }
            }
            return conf;
        }
    };

    template<>
    class LexicalCast<TcpServerConf, std::string> {
    public:
        std::string operator()(const TcpServerConf& conf) {
            YAML::Node node;
            node["type"] = conf.type;
            node["name"] = conf.name;
            node["keepalive"] = conf.keepalive;
            node["timeout"] = conf.timeout;
            for(auto& i : conf.address) {
                node["address"].push_back(i);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief TCP 服务器封装
     */
    class TCPServer : public std::enable_shared_from_this<TCPServer>, Noncopyable {
    public:
        typedef std::shared_ptr<TCPServer> ptr;

        explicit TCPServer(IOManager *worker = IOManager::GetThis(),
                           IOManager *accept = IOManager::GetThis());

        virtual ~TCPServer();

        virtual bool bind(Address::ptr address);

        virtual bool bind(const std::vector<Address::ptr> &addresses);

        virtual bool start();

        virtual void stop();

        uint64_t getRecvTimeout() const { return m_recvTimeout; }

        void setRecvTimeout(uint64_t timeout) { m_recvTimeout = timeout; }

        std::string getName() const { return m_name; }

        virtual void setName(std::string name) { m_name = std::move(name); }

        bool isStop() const { return m_stop; }

        virtual std::string toString();

    protected:
        virtual void handleClient(Socket::ptr client);

        virtual void startAccept(Socket::ptr sock);

    protected:
        std::vector<Socket::ptr> m_socks;   /// 监听 socket 数组
        IOManager *m_worker;                /// 新连接的 socket 的工作调度器
        IOManager *m_accept;                /// 接受 socket 连接的调度器
        uint64_t m_recvTimeout;             /// 接收超时时间
        std::string m_name;                 /// 服务器名称
        bool m_stop;                        /// 服务器是否停止
    };
}
#endif //LUWU_TCP_SERVER_H
