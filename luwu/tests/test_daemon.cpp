#include "luwu/daemon.h"
#include "luwu/macro.h"
#include "luwu/iomanager.h"
#include "luwu/config.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

liucxi::Timer::ptr timer;
int server_main(int argc, char **argv) {
    LUWU_LOG_INFO(g_logger) << liucxi::ProcessInfoMgr::getInstance()->toString();
    liucxi::IOManager iom(1);
    timer = iom.addTimer(
            1000, []() {
                LUWU_LOG_INFO(g_logger) << "onTimer";
                static int count = 0;
                if (++count > 10) {
                    exit(0);
                }
            },
            true);
    return 0;
}

int main(int argc, char **argv) {
    LUWU_LOG_INFO(g_logger) << argc;
    YAML::Node node = YAML::LoadFile("../conf/log.yml");
    liucxi::Config::loadFromYaml(node);
    return liucxi::start_daemon(argc, argv, server_main, argc != 1);
}

// ps aux | grep test_daemon
// kill -9 5213