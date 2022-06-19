//
// Created by liucxi on 2022/6/16.
//

#ifndef LUWU_APPLICATION_H
#define LUWU_APPLICATION_H

#include "iomanager.h"
#include "tcp_server.h"

#include <map>
#include <vector>
namespace liucxi {
    class Application {
    public:
        Application();

        static Application *GetInstance() { return s_instance; }

        bool init(int argc, char **argv);

        bool run();

        bool getServer(const std::string& type, std::vector<TCPServer::ptr>& svrs);

    private:
        int main(int argc, char **argv);

        int run_fiber();
    private:
        int m_argc = 0;
        char **m_argv = nullptr;
        IOManager::ptr m_ioManager;
        static Application *s_instance;
        std::map<std::string, std::vector<TCPServer::ptr> > m_servers;
    };
}
#endif //LUWU_APPLICATION_H
