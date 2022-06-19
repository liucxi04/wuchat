//
// Created by liucxi on 2022/6/16.
//

#ifndef LUWU_DAEMON_H
#define LUWU_DAEMON_H

#include <unistd.h>
#include <functional>
#include "utils.h"

namespace liucxi {

    struct ProcessInfo {
        pid_t parent_id = 0;
        pid_t main_id = 0;
        uint64_t parent_start_time = 0;
        uint64_t main_start_time = 0;
        uint32_t restart_count = 0;         /// 主进程重启次数

        std::string toString() const;
    };

    typedef Singleton<ProcessInfo> ProcessInfoMgr;

    /**
     * @brief 使用守护进程的方式启动
     * @param is_daemon 是否使用守护进程方式启动
     */
    int start_daemon(int argc, char **argv,
                     const std::function<int(int argc, char **argv)>& main_cb,
                     bool is_daemon);
}
#endif //LUWU_DAEMON_H
