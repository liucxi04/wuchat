//
// Created by liucxi on 2022/5/29.
//

#ifndef LUWU_FD_MANAGER_H
#define LUWU_FD_MANAGER_H

#include <memory>
#include <vector>

#include "mutex.h"
#include "utils.h"

namespace liucxi {
    /**
     * @brief 文件描述符上下文
     */
    class FdContext : public std::enable_shared_from_this<FdContext> {
    public:
        typedef std::shared_ptr<FdContext> ptr;

        explicit FdContext(int fd);

        ~FdContext() = default;

        bool init();

        bool isInit() const { return m_isInit; }

        bool isSocket() const { return m_isSocket; }

        bool isClose() const { return m_isClosed; }

        void setUserNonBlock(bool v) { m_isUserNonBlock = v; }

        bool getUserNonBlock() const { return m_isUserNonBlock; }

        void setSysNonBlock(bool v) { m_isSysNonBlock =v; }

        bool getSysNonBlock() const { return m_isSysNonBlock; }

        void setTimeout(int type, uint64_t v);

        uint64_t getTimeout(int type) const;

    private:
        bool m_isInit:1;
        bool m_isSocket:1;
        bool m_isSysNonBlock:1;         /// 用户主动设置了非阻塞
        bool m_isUserNonBlock:1;        /// hook 设置了非阻塞
        bool m_isClosed:1;
        int m_fd;

        uint64_t m_recvTimeout;         /// 读超时时间（毫秒）
        uint64_t m_sendTimeout;         /// 写超时时间（毫秒）
    };

    /**
     * @brief 文件描述符管理类
     */
    class FdManager {
    public:
        typedef RWMutex RWMutexType;

        FdManager();

        /**
         * @brief 获取/创建文件描述符
         * @param auto_create 是否自动创建
         */
        FdContext::ptr get(int fd, bool auto_create = false);

        /**
         * @brief 删除文件描述符
         */
        void del(int fd);

    private:
        RWMutexType m_mutex;
        std::vector<FdContext::ptr> m_fds;
    };

    /**
     * @brief 文件描述符管理也是单例模式
     */
    typedef Singleton<FdManager> FdMgr;
}
#endif //LUWU_FD_MANAGER_H
