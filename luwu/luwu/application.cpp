//
// Created by liucxi on 2022/6/16.
//

#include <csignal>
#include <memory>

#include "application.h"
#include "env.h"
#include "macro.h"
#include "config.h"
#include "ext.h"
#include "daemon.h"
#include "http/http_server.h"
#include "http/ws_server.h"

namespace liucxi {

    static Logger::ptr g_logger = LUWU_LOG_NAME("system");

    static ConfigVar<std::string>::ptr g_server_work_path =
            Config::lookup("server.work_path",
                           std::string("/home/liucxi/Documents/chat_luwu/bin"),
                           "server work path");

    static ConfigVar<std::string>::ptr g_server_pid_file =
            Config::lookup("server.pid_file",
                           std::string("chat.pid"),
                           "server pid file");

    static ConfigVar<std::vector<TcpServerConf>>::ptr g_server_conf =
            Config::lookup("servers",
                           std::vector<TcpServerConf>(),
                           "server conf");

    Application *Application::s_instance = nullptr;

    Application::Application() {
        s_instance = this;
    }

    bool Application::init(int argc, char **argv) {
        m_argc = argc;
        m_argv = argv;

        EnvMgr::getInstance()->addHelp("s", "start with the terminal");
        EnvMgr::getInstance()->addHelp("d", "run as daemon");
        EnvMgr::getInstance()->addHelp("c", "conf path default: ./conf");
        EnvMgr::getInstance()->addHelp("p", "print help");

        if (!EnvMgr::getInstance()->init(argc, argv)) {
            EnvMgr::getInstance()->printHelp();
            return false;
        }

        if (EnvMgr::getInstance()->has("p")) {
            EnvMgr::getInstance()->printHelp();
            return false;
        }

        std::string conf_path = EnvMgr::getInstance()->getConfigPath();
        LUWU_LOG_INFO(LUWU_LOG_ROOT()) << "load conf path: " << conf_path;
        Config::LoadFromConfDir(conf_path);

        int run_type = 0;
        if (EnvMgr::getInstance()->has("s")) {
            run_type = 1;
        }
        if (EnvMgr::getInstance()->has("d")) {
            run_type = 2;
        }

        if (run_type == 0) {
            EnvMgr::getInstance()->printHelp();
            return false;
        }

        std::string pid_file = g_server_work_path->getValue() + "/" +
                g_server_pid_file->getValue();
        if (FSUtil::IsRunningPidfile(pid_file)) {
            LUWU_LOG_ERROR(g_logger) << "server is running:" << pid_file;
            return false;
        }

        if (!FSUtil::Mkdir(g_server_work_path->getValue())) {
            LUWU_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
                                     << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Application::run() {
        bool is_daemon = EnvMgr::getInstance()->has("d");
        return start_daemon(m_argc, m_argv,
                            std::bind(&Application::main, this,
                                              std::placeholders::_1, std::placeholders::_2)
                            , is_daemon);
    }

    int Application::main(int argc, char **argv) {
//        signal(SIGPIPE, SIG_IGN);
        LUWU_LOG_INFO(g_logger) << "Application main";
        std::string conf_path = EnvMgr::getInstance()->getConfigPath();
        Config::LoadFromConfDir(conf_path, true);
        {
            std::string pid_file = g_server_work_path->getValue() + "/" +
                                   g_server_pid_file->getValue();
            std::ofstream ofs(pid_file);
            if (!ofs) {
                LUWU_LOG_ERROR(g_logger) << "open pid file " << pid_file << " failed";
                return false;
            }
            ofs << getpid();
        }

//        m_ioManager.reset(new IOManager(1, true, "main"));
//        m_ioManager->scheduler([this] { run_fiber(); });
//        m_ioManager->addTimer(2000, [](){
//        }, true);
//        m_ioManager->stop();
        IOManager iom(1, true);
        iom.scheduler([this]{ run_fiber(); });
        iom.stop();
        return 0;
    }


    int Application::run_fiber() {

        auto http_conf = g_server_conf->getValue();
        for(auto& i : http_conf) {
//            LUWU_LOG_INFO(g_logger) << std::endl << LexicalCast<TcpServerConf, std::string>()(i);

            std::vector<Address::ptr> address;
            for(auto& a : i.address) {
                size_t pos = a.find(':');
                if(pos == std::string::npos) {
                    address.push_back(std::make_shared<UnixAddress>(a));
                    continue;
                }
                int16_t port = atoi(a.substr(pos + 1).c_str());
                auto addr = IPAddress::Create(a.substr(0, pos).c_str(), port);
                if(addr) {
                    address.push_back(addr);
                    continue;
                }
                std::vector<std::pair<Address::ptr, uint32_t> > result;
                if(Address::GetInterfaceAddress(result, a.substr(0, pos))) {
                    for(auto& x : result) {
                        auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                        if(ipaddr) {
                            ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                        }
                        address.push_back(ipaddr);
                    }
                    continue;
                }

                auto addr1 = Address::LookupAny(a);
                if(addr1) {
                    address.push_back(addr1);
                    continue;
                }
                LUWU_LOG_ERROR(g_logger) << "invalid address: " << a;
                _exit(0);
            }

            TCPServer::ptr server;
            if(i.type == "http") {
                server.reset(new http::HttpServer(i.keepalive));
            } else if(i.type == "ws") {
                server.reset(new http::WSServer());
            } else {
                LUWU_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                                          << LexicalCast<TcpServerConf, std::string>()(i);
                _exit(0);
            }

            if(!i.name.empty()) {
                server->setName(i.name);
            }

            if(!server->bind(address)) {
                _exit(0);
            }

            m_servers[i.type].push_back(server);
            server->start();
        }
        return 0;
    }

    bool Application::getServer(const std::string& type, std::vector<TCPServer::ptr>& svrs) {
        auto it = m_servers.find(type);
        if(it == m_servers.end()) {
            return false;
        }
        svrs = it->second;
        return true;
    }
}