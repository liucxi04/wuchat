//
// Created by liucxi on 2022/4/25.
//

#ifndef LUWU_IOMANAGER_H
#define LUWU_IOMANAGER_H

#include <memory>
#include <vector>
#include <functional>

#include "timer.h"
#include "scheduler.h"

namespace liucxi {
    class IOManager : public Scheduler, public TimerManager {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        /**
         * @brief IO 事件，继承自 epoll 对事件的定义
         */
        enum Event {
            NONE = 0x0,         /// 无事件
            READ = 0x1,         /// 读事件
            WRITE = 0x4,        /// 写事件
        };

    private:
        /**
         * @brief socket fd 上下文
         * @details 描述符 - 事件 - 回调函数三元组定义
         */
        struct FdContext {
            typedef Mutex MutexType;
            /**
             * @brief 事件上下文
             */
            struct EventContext {
                Scheduler *scheduler = nullptr;         /// 执行回调的调度器
                Fiber::ptr fiber;                       /// 事件回调协程
                std::function<void()> cb;               /// 事件回调函数
            };

            EventContext &getEventContext(Event event);

            static void resetEventContext(EventContext &ctx);

            void triggerEvent(Event event);

            int fd = 0;                 /// 描述符
            EventContext read;          /// 读事件上下文
            EventContext write;         /// 写事件上下文
            Event event = NONE;         /// 该描述符所绑定的事件，多个事件使用 | 连接
            MutexType mutex;
        };

    public:
        explicit IOManager(size_t threads = 1, bool use_caller = true, std::string name = "");

        ~IOManager() override;

        bool addEvent(int fd, Event event, std::function<void()> cb = nullptr);

        bool delEvent(int fd, Event event);

        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        static IOManager *GetThis();

    protected:
        void tickle() override;

        void idle() override;

        bool stopping() override;

        bool stopping(uint64_t &timeout);

        void onTimerInsertAtFront() override;

        void contextResize(size_t size);

    private:
        int m_epfd{};                                       /// epoll 事件描述副
        int m_tickleFds[2]{};                               /// pipe 文件描述符， 0 读，1 写
        std::atomic<size_t> m_pendingEventCount = {0};    /// 当前等待执行的 IO 事件数量
        std::vector<FdContext *> m_fdContexts;              /// 所有的 socket fd 上下文
        RWMutexType m_mutex;
    };
}

#endif //LUWU_IOMANAGER_H
