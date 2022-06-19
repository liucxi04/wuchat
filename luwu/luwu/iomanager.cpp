//
// Created by liucxi on 2022/4/25.
//

#include "iomanager.h"
#include "macro.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

namespace liucxi {
    static Logger::ptr g_logger = LUWU_LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(Event e) {
        switch (e) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                LUWU_ASSERT2(false, "getContext")
        }
    }

    void IOManager::FdContext::resetEventContext(EventContext &ctx) {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event e) {
        LUWU_ASSERT(event & e)

        event = (Event) (event & ~e);           // 与 e 的非相与表示在 event 所关注的事件里去掉了 e
        EventContext &ctx = getEventContext(e);
        if (ctx.cb) {
            ctx.scheduler->scheduler(ctx.cb);
        } else {
            ctx.scheduler->scheduler(ctx.fiber);
        }
        resetEventContext(ctx);
    }

    IOManager::IOManager(size_t threads, bool use_caller, std::string name)
            : Scheduler(threads, use_caller, std::move(name)) {
        LUWU_LOG_INFO(g_logger) << "IOManager::IOManager";
        m_epfd = epoll_create(5000);
        LUWU_ASSERT(m_epfd > 0)

        int rt = pipe(m_tickleFds);
        LUWU_ASSERT(!rt)

        /// 以下将 m_tickleFds[0] 添加到了 m_epfd，用来监听 tickle 传来的通知
        epoll_event event{};
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        LUWU_ASSERT(!rt)

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        LUWU_ASSERT(!rt)

        contextResize(32);
        /// 这里调用 scheduler 的 start，即开始了协程调度
        start();
    }

    void IOManager::contextResize(size_t size) {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = (int) i;
            }
        }
    }

    IOManager::~IOManager() {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (auto &context: m_fdContexts) {
            delete context;
        }
    }

    bool IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
        FdContext *fdContext = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int) m_fdContexts.size() > fd) {
            fdContext = m_fdContexts[fd];
            lock.unlock();
        } else {
            lock.unlock();
            RWMutexType::WriteLock lock1(m_mutex);
            contextResize(fd * 2);
            fdContext = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock1(fdContext->mutex);
        /// 同一个 fd 不能绑定同样的事件两次
        if (fdContext->event & event) {
            LUWU_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                     << " event=" << (EPOLL_EVENTS) event
                                     << " fd_ctx.event=" << (EPOLL_EVENTS) fdContext->event;
            LUWU_ASSERT(!(fdContext->event & event))
        }

        int op = fdContext->event ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epollEvent{};
        epollEvent.events = EPOLLET | fdContext->event | event;
        epollEvent.data.ptr = fdContext;

        int rt = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (rt) {
            LUWU_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                     << op << ", " << fd << ", " << (EPOLL_EVENTS) epollEvent.events << "):"
                                     << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        /// 待执行 IO 事件数加 1
        ++m_pendingEventCount;

        fdContext->event = (Event) (fdContext->event | event);
        FdContext::EventContext &eventContext = fdContext->getEventContext(event);
        LUWU_ASSERT(!eventContext.scheduler && !eventContext.fiber && !eventContext.cb);

        eventContext.scheduler = Scheduler::GetThis();
        if (cb) {
            eventContext.cb.swap(cb);
        } else {
            /// 如果回调函数为空，则把当前协程当作回调执行体
            /// 回调函数为空，说明是一个回调函数中途 yield，需要将自己添加到监听队列里，等待再次执行
            eventContext.fiber = Fiber::GetThis();
            LUWU_ASSERT(eventContext.fiber->getState() == Fiber::RUNNING)
        }
        return true;
    }

    bool IOManager::delEvent(int fd, Event event) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int) m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext *fdContext = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock1(fdContext->mutex);
        if (!(fdContext->event & event)) {
            return false;
        }

        auto new_event = (Event) (fdContext->event & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epollEvent{};
        epollEvent.events = EPOLLET | new_event;
        epollEvent.data.ptr = fdContext;

        int rt = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (rt) {
            LUWU_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                     << op << ", " << fd << ", " << (EPOLL_EVENTS) epollEvent.events << "):"
                                     << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fdContext->event = new_event;
        FdContext::EventContext &eventContext = fdContext->getEventContext(event);
        fdContext->resetEventContext(eventContext);

        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int) m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext *fdContext = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock1(fdContext->mutex);
        if (!(fdContext->event & event)) {
            return false;
        }

        auto new_event = (Event) (fdContext->event & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epollEvent{};
        epollEvent.events = EPOLLET | new_event;
        epollEvent.data.ptr = fdContext;

        int rt = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (rt) {
            LUWU_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                     << op << ", " << fd << ", " << (EPOLL_EVENTS) epollEvent.events << "):"
                                     << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        /// 取消之前触发一次
        fdContext->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd) {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int) m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext *fdContext = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock1(fdContext->mutex);
        if (!fdContext->event) {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epollEvent{};
        epollEvent.events = 0;
        epollEvent.data.ptr = fdContext;

        int rt = epoll_ctl(m_epfd, op, fd, &epollEvent);
        if (rt) {
            LUWU_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                     << op << ", " << fd << ", " << (EPOLL_EVENTS) epollEvent.events << "):"
                                     << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        if (fdContext->event & READ) {
            fdContext->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fdContext->event & WRITE) {
            fdContext->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        LUWU_ASSERT(fdContext->event == 0);
        return true;
    }

    IOManager *IOManager::GetThis() {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    /**
     * @brief 通知调度器有任务需要调度
     */
    void IOManager::tickle() {
        if (!hasIdleThreads()) {
            return;
        }
        ssize_t rt = write(m_tickleFds[1], "T", 1);
        LUWU_ASSERT(rt == 1)
    }

    bool IOManager::stopping() {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    bool IOManager::stopping(uint64_t &timeout) {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    void IOManager::idle() {
        /// epoll 一次监听 256 个事件，如果就绪事件超过了这个值，会在下一轮继续处理
        const uint64_t MAX_EVENTS = 256;
        auto *events = new epoll_event[MAX_EVENTS]();
        std::shared_ptr<epoll_event> shared_event(events, [](epoll_event *ptr) {
            delete[] ptr;
        });

        while (true) {
            uint64_t next_timeout = 0;
            /// 获取最近一个定时器的超时时间，判断调度是否停止
            if (stopping(next_timeout)) {
                LUWU_LOG_INFO(g_logger) << "name = " << getName() << "idle stopping exit";
                break;
            }

            int rt = 0;
            do {
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull) {
                    // 有定时器
                    next_timeout = std::min((int) next_timeout, MAX_TIMEOUT);
                } else {
                    // 没有定时器
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int) next_timeout);
                if (rt < 0 && errno == EINTR) {
                    // 忽略中断错误，继续监听
                    continue;
                } else {
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty()) {
                for (const auto &cb: cbs) {
                    scheduler(cb);
                }
                cbs.clear();
            }

            for (int i = 0; i < rt; ++i) {
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0]) {
                    uint8_t dummy[256];
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                    continue;
                }

                auto *fd_ctx = (FdContext *) event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);

                if (event.events & (EPOLLERR | EPOLLHUP)) {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->event;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN) {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT) {
                    real_events |= WRITE;
                }

                if ((fd_ctx->event & real_events) == NONE) {
                    continue;
                }

                int left_events = (fd_ctx->event & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2) {
                    LUWU_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                             << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS) event.events << "):"
                                             << rt2 << "(" << errno << ")(" << strerror(errno) << ")";
                    continue;
                }

                if (real_events & READ) {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE) {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            } // end for

            /**
            * 一旦处理完所有的事件，idle 协程 yield，这样可以让调度协程 (Scheduler::run) 重新检查是否有新任务要调度
            * 上面 triggerEvent 实际也只是把对应的 fiber 重新加入调度，要执行的话还要等idle协程退出
            */
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();
            raw_ptr->yield();
        } // end while
    }

    void IOManager::onTimerInsertAtFront() {
        tickle();
    }
}