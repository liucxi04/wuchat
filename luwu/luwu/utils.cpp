//
// Created by liucxi on 2022/4/8.
//

#include "utils.h"
#include "fiber.h"

#include <iostream>
#include <unistd.h>
#include <sstream>
#include <execinfo.h>  // backtrace
#include <sys/syscall.h>

namespace liucxi {

    pid_t getThreadId() {
        return (pid_t)syscall(SYS_gettid);
    }

    u_int64_t getFiberId() {
        return Fiber::GetFiberId();
    }

    /**
     * @brief 单调时间，即从某个时间点开始到现在过去的时间
     */
    uint64_t getElapseMS() {
        struct timespec ts{0};
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    std::string getThreadName() {
        char threadName[16] = {0};
        pthread_getname_np(pthread_self(), threadName, 16);
        return std::string{threadName};
    }

    void setThreadName(const std::string &name) {
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip) {
        void **array = (void **) malloc((sizeof(void *) * size));
        int s = ::backtrace(array, size);

        char ** strings = backtrace_symbols(array, s);
        if (strings == nullptr) {
            std::cout << "backtrace_symbols error" << std::endl;
            return;
        }

        for (int i = skip; i < s; ++i) {
            bt.emplace_back(strings[i]);
        }

        free(strings);
        free(array);
    }

    /**
     * @brief 获得程序调用堆栈信息
     * @param size 需要获取几层信息
     * @param skip 忽略前面几层
     */
    std::string BacktraceToString(int size, int skip, const std::string &prefix) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;

        for (const auto &i : bt) {
            ss << prefix << i << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS() {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }
}
