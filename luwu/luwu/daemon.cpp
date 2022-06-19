//
// Created by liucxi on 2022/6/16.
//
#include "daemon.h"
#include "macro.h"
#include "config.h"
#include "utils.h"
#include "ext.h"
#include <sys/wait.h>

namespace liucxi {

    static Logger::ptr g_logger = LUWU_LOG_NAME("system");
    static ConfigVar<uint32_t>::ptr g_daemon_restart_interval =
            Config::lookup("daemon.restart_interval", (uint32_t) 5, "daemon restart interval");

    std::string ProcessInfo::toString() const {
        std::stringstream ss;
        ss << "[ProcessInfo parent_id=" << parent_id
           << " main_id=" << main_id
           << " parent_start_time=" << Time2Str(parent_start_time)
           << " main_start_time=" << Time2Str(main_start_time)
           << " restart_count=" << restart_count << "]";
        return ss.str();
    }

    int real_start(int argc, char **argv, const std::function<int(int argc, char **argv)> &main_cb) {
        return main_cb(argc, argv);
    }

    int real_daemon(int argc, char **argv, const std::function<int(int argc, char **argv)> &main_cb) {
//        daemon(1, 0);
        ProcessInfoMgr::getInstance()->parent_id = getpid();
        ProcessInfoMgr::getInstance()->parent_start_time = time(nullptr);
        while (true) {
            pid_t pid = fork();
            if (pid == 0) {
                ProcessInfoMgr::getInstance()->main_id = getpid();
                ProcessInfoMgr::getInstance()->main_start_time = time(nullptr);
                LUWU_LOG_INFO(g_logger) << "process start pid = " << getpid();
                return real_start(argc, argv, main_cb);
            } else if (pid < 0) {
                LUWU_LOG_ERROR(g_logger) << "fork fail return = " << pid
                                         << " errno = " << errno << " errstr = " << strerror(errno);
                return -1;
            } else {
                int status = 0;
                waitpid(pid, &status, 0);
                if (status) {
                    LUWU_LOG_ERROR(g_logger) << "child crash pid = " << pid << " status = " << status;
                } else {
                    LUWU_LOG_INFO(g_logger) << "child finish pid = " << pid << " status = " << status;
                    break;
                }
                ProcessInfoMgr::getInstance()->restart_count += 1;
                sleep(g_daemon_restart_interval->getValue());
            }
        }
        return 0;
    }

    int start_daemon(int argc, char **argv,
                     const std::function<int(int argc, char **argv)> &main_cb,
                     bool is_daemon) {
        if (!is_daemon) {
            return real_start(argc, argv, main_cb);
        }
        return real_daemon(argc, argv, main_cb);
    }
}
