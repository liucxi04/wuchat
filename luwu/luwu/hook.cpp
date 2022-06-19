//
// Created by liucxi on 2022/5/29.
//
#include "hook.h"
#include "fiber.h"
//#include "macro.h"
#include "config.h"
#include "iomanager.h"
#include "fd_manager.h"

#include <dlfcn.h>
#include <cstdarg>

namespace liucxi {

    static ConfigVar<int>::ptr g_tcp_connect_timeout =
            Config::lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

    static uint64_t s_connect_timeout = -1;

    static thread_local bool t_hook_enable = false;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

#define HOOK_FUN(XX)    \
    XX(sleep)           \
    XX(usleep)          \
    XX(socket)          \
    XX(connect)         \
    XX(accept)          \
    XX(close)           \
    XX(read)            \
    XX(recv)            \
    XX(readv)           \
    XX(recvfrom)        \
    XX(recvmsg)         \
    XX(write)           \
    XX(send)            \
    XX(writev)          \
    XX(sendto)          \
    XX(sendmsg)         \
    XX(fcntl)           \
    XX(ioctl)           \
    XX(getsockopt)      \
    XX(setsockopt)      \

    void hook_init() {
        static bool is_init = false;
        if (is_init) {
            return;
        }
        /**
         *  ## 用于字符串链接、 # 表示是一个字符串
         *  以下宏展开类似于
         *  sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
         *  usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep"); \
         */
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX)
#undef XX
    }

    struct HookInit {
        HookInit() {
            hook_init();

            s_connect_timeout = g_tcp_connect_timeout->getValue();
            g_tcp_connect_timeout->addListener([](const int &oldVal, const int &newVal){
                LUWU_LOG_INFO(LUWU_LOG_NAME("system")) << "tcp connect timeout changed from "
                    << oldVal << " to " << newVal;
                s_connect_timeout = newVal;
            });
        }
    };
    /// 全局变量会在 main 函数之前被初始化，完成 hook_init 和 加载配置文件
    /// 在 main 函数运行之前就会获取各个符号的地址并保存在全局变量中
    static HookInit s_hook_init;
}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&... args) {
    if (!liucxi::is_hook_enable()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonBlock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }

    if (n == -1 && errno == EAGAIN) {
        liucxi::IOManager *iom = liucxi::IOManager::GetThis();
        std::weak_ptr<timer_info> winfo(tinfo);
        liucxi::Timer::ptr timer;

        // 如果设置了超时时间
        if (timeout != (uint64_t)-1) {
            timer = iom->addConditionTimer(timeout, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (liucxi::IOManager::Event) (event));
            }, winfo);
        }

        int rt = iom->addEvent(fd, (liucxi::IOManager::Event) (event));
        if (rt) {
            liucxi::Fiber::GetThis()->yield();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        } else {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << hook_fun_name << " addEvent("
                                                    << fd << " ," << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        }
    }
    return n;
}

/**
 * 接口的 hook 实现，要放在 extern "C" 中，以防止 C++ 编译器对符号名称添加修饰。
 */
extern "C" {
/**
 *  初始化
 *  sleep_fun sleep_f = nullptr;
 *  usleep_fun usleep_f = nullptr;
 */
#define XX(name) name ## _fun name ## _f = nullptr;
        HOOK_FUN(XX)
#undef XX

unsigned int sleep(unsigned int seconds) {
    if (!liucxi::is_hook_enable()) {
        return sleep_f(seconds);
    }
    auto fiber = liucxi::Fiber::GetThis();
    auto iom = liucxi::IOManager::GetThis();
    iom->addTimer(seconds * 1000, [iom, fiber]() {
        iom->scheduler(fiber);
    });
    liucxi::Fiber::GetThis()->yield();
    return 0;
}

int usleep(useconds_t usec) {
    if (!liucxi::is_hook_enable()) {
        return usleep_f(usec);
    }
    auto fiber = liucxi::Fiber::GetThis();
    auto iom = liucxi::IOManager::GetThis();
    iom->addTimer(usec / 1000, [iom, fiber]() {
        iom->scheduler(fiber);
    });
    liucxi::Fiber::GetThis()->yield();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!liucxi::is_hook_enable()) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd >= 0) {
        liucxi::FdMgr::getInstance()->get(fd, true);
    }
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addlen, uint64_t timeout) {
    if (!liucxi::is_hook_enable()) {
        return connect_f(sockfd, addr, addlen);
    }

    liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(sockfd);
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonBlock()) {
        return connect_f(sockfd, addr, addlen);
    }

    int n = connect_f(sockfd, addr, addlen); // 这里的 sockfd 是非阻塞的
    if (n == 0) {
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {
        // 连接正常，或者不是连接正在进行的情况
        return n;
    }

    // n == -1 && errno == EINPROGRESS，表示连接还在进行中
    // 后面可以通过 epoll 来判断 socket 是否可写，如果可以写，说明连接完成了。
    liucxi::IOManager *iom = liucxi::IOManager::GetThis();
    liucxi::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    // 如果设置了超时参数，那么添加一个条件定时器
    if (timeout != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout, [winfo, iom, sockfd](){
            auto t = winfo.lock();
            if (!t || t->cancelled) {
                return;
            }
            // 设置超时标志并触发一次 WRITE 事件
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(sockfd, liucxi::IOManager::WRITE);     /// ???
        }, winfo);
    }

    bool rt = iom->addEvent(sockfd, liucxi::IOManager::WRITE);
    if (rt) {
        liucxi::Fiber::GetThis()->yield();
        // 如果是套接字可写，那么直接取消定时器
        if (timer) {
            timer->cancel();
        }
        // 如果是超时了，那么设置错误信息并返回
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "connect addEvent("
            << sockfd << ", WRITE) error";
    }

    // 走到这是套接字可写或者是添加写事件失败
    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addlen) {
    return connect_with_timeout(sockfd, addr, addlen, liucxi::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addlen) {
    auto fd = (int) do_io(sockfd, accept_f, "accept",
                          liucxi::IOManager::READ, SO_RCVTIMEO, addr, addlen);
    if (fd >= 0) {
        liucxi::FdMgr::getInstance()->get(fd, true);
    }
    return fd;
}

int close(int fd) {
    if (!liucxi::is_hook_enable()) {
        return close_f(fd);
    }
    auto ctx = liucxi::FdMgr::getInstance()->get(fd);
    if (ctx) {
        auto iom = liucxi::IOManager::GetThis();
        if (iom) {
            // 需要取消掉 fd 上的全部事件
            iom->cancelAll(fd);
        }
        liucxi::FdMgr::getInstance()->del(fd);
    }
    return close_f(fd);
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read",
                 liucxi::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv",
                 liucxi::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv",
                 liucxi::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom",
                 liucxi::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg",
                 liucxi::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write",
                 liucxi::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send",
                 liucxi::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev",
                 liucxi::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto",
                 liucxi::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg",
                 liucxi::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int fcntl(int fd, int cmd, ... /* arg */) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }

            ctx->setUserNonBlock(arg & O_NONBLOCK);
            if (ctx->getSysNonBlock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETFL:
        {
            va_end(va);
            int flags = fcntl_f(fd, cmd);
            liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return flags;
            }

            if (ctx->getUserNonBlock()) {
                return flags | O_NONBLOCK;
            } else {
                return flags & ~O_NONBLOCK;
            }
        }
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock *arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        default:
            va_end(va);
            return fcntl_f(fd, cmd);

    }
}

int ioctl(int fd, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void*);
    va_end(va);

    if (request == FIONBIO) {
        bool user_nonblock = !!*(int*)arg;
        liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonBlock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if (liucxi::is_hook_enable()) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            liucxi::FdContext::ptr ctx = liucxi::FdMgr::getInstance()->get(sockfd);
            if (ctx) {
                auto v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
