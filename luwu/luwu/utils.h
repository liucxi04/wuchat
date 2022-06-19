//
// Created by liucxi on 2022/4/8.
//

#ifndef LUWU_UTILS_H
#define LUWU_UTILS_H

#include <sys/types.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <byteswap.h>

namespace liucxi {

    /**
     * @brief 单例模式
     */
    template<typename T>
    class Singleton {
    public:
        static T *getInstance() {
            static T v;
            return &v;
        }
    };

    pid_t getThreadId();

    u_int64_t getFiberId();

    u_int64_t getElapseMS();

    std::string getThreadName();

    void setThreadName(const std::string &name);

    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

    std::string BacktraceToString(int size = 64, int skip = 1, const std::string &prefix="");

    uint64_t GetCurrentMS();

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteSwap(T value) {
        return (T)bswap_64((uint64_t)value);
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteSwap(T value) {
        return (T)bswap_32((uint32_t)value);
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteSwap(T value) {
        return (T)bswap_16((uint16_t)value);
    }

#if BYTE_ORDER == BIG_ENDIAN
    template<typename T>
    T byteSwapOnLittleEndian(T t) {
        return t;
    }

    template<typename T>
    T byteSwapOnBigEndian(T t) {
        return byteSwap(t);
    }
#else
    template<typename T>
    T byteSwapOnLittleEndian(T t) {
        return byteSwap(t);
    }

    template<typename T>
    T byteSwapOnBigEndian(T t) {
        return t;
    }
#endif
}


#endif //LUWU_UTILS_H
