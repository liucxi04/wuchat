//
// Created by liucxi on 2022/4/20.
//

#include <utility>
#include "hook.h"
#include "macro.h"
#include "scheduler.h"

namespace liucxi {

    /// 当前线程的调度器，同一个调度器下的所有线程共享同一个实例
    static thread_local Scheduler *t_scheduler = nullptr;
    /// 当前线程的调度协程，每个线程独有一份
    static thread_local Fiber *t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_call, std::string name)
        : m_name(std::move(name))
        , m_useCaller(use_call){

        LUWU_ASSERT(threads > 0)
        if (use_call) {
            --threads;
            Fiber::GetThis(); // 初始化当前线程的主协程
            LUWU_ASSERT(GetThis() == nullptr) // 当前线程还没有调度器
            t_scheduler = this;

            /**
             * 调度器所在线程的主协程和调度协程不是同一个，上面初始化了主协程，下面初始化调度协程
             * 把调度程暂时保存起来，等其他线程调度协程结束时，再执行
             */
            m_rootFiber.reset(new Fiber([this] { run(); }, 0, false));

            Thread::SetName(m_name);
            t_scheduler_fiber = m_rootFiber.get();
            m_rootThread = getThreadId();
            m_threadIds.push_back(m_rootThread);
        } else {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler() {
        LUWU_ASSERT(m_stopping)
        if (GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis() {
        return t_scheduler;
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    Fiber *Scheduler::GetMainFiber() {
        return t_scheduler_fiber;
    }

    void Scheduler::start() {
        MutexType::Lock lock(m_mutex);
        if (m_stopping) {
            return;
        }
        LUWU_ASSERT(m_threads.empty())
        m_threads.resize(m_threadCount);
        // 如果 use_caller 为 ture，并且线程数为1，那么下面就不会被执行，start 函数什么也不做
        for (size_t i = 0; i < m_threadCount; ++i) {
            // 线程入口函数为 run，即线程主协程就是 run 对应的协程
            m_threads[i].reset(new Thread([this] { run(); },
                                          m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    /**
     * @note 只有当所有的任务都被执行完之后，调度器才可以停止
     */
    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::tickle() {
    }

    /**
     * @details 当 stopping 为 true 时，idle 协程就会退出 while 循环，idle 协程结束， idle 协程状态就变成了 TERM
     */
    void Scheduler::idle() {
        while (!stopping()) {
            Fiber::GetThis()->yield();
        }
    }

    void Scheduler::stop() {
        if (stopping()) {
            return;
        }
        m_stopping = true;

        /// 如果 use caller，那么只能由 caller 线程发起 stop
        if (m_useCaller) {
            LUWU_ASSERT(GetThis() == this)
        } else {
            LUWU_ASSERT(GetThis() != this)
        }

        /// 通知其他调度线程的调度协程退出调度
        for (size_t i = 0; i < m_threadCount; ++i) {
            tickle();
        }

        /// 通知当前线程的调度协程退出调度
        if (m_rootFiber) {
            tickle();
        }

        /// 在 use caller 情况下，协程调度器结束时，应该调用 caller 协程
        if (m_rootFiber) {
            m_rootFiber->resume();
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        /// 等待所有任务都调度完成才可以退出
        for (auto &i : thrs) {
            i->join();
        }
    }

    /**
     * @brief 调度协程入口函数
     * @details caller 线程的调度协程不是主协程，其他线程的调度协程就是主协程
     */
    void Scheduler::run() {
        /// 需要 hook
        set_hook_enable(true);
        setThis();
        /// 当前线程不是调度器所在的线程，所以线程的主协程需要初始化，调度协程也需要初始化
        if (getThreadId() != m_rootThread) {
            t_scheduler_fiber = Fiber::GetThis().get();
        }

        Fiber::ptr idle_fiber(new Fiber([this] { idle(); }));
        Fiber::ptr cb_fiber;

        SchedulerTask task;
        while (true) {
            task.reset();
            bool tickle_me = false; // 是否需要 tickle 其他线程进行任务调度
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end()) {
                    // 指定了调度线程，但是不是当前线程，所以需要通知其他线程
                    if (it->thread != -1 && it->thread != getThreadId()) {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    LUWU_ASSERT(it->fiber || it->cb)

                    //hook 模块需要
//                    if (it->fiber && it->fiber->getState() == Fiber::RUNNING) {
//                        ++it;
//                        continue;
//                    }
                    // 找到了一个任务，开始调度
                    task = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    break;
                }
                // 当前线程拿完一个任务后，任务队列还有剩余，也需要 tickle 其他线程
                tickle_me |= (it != m_fibers.end());
            }

            if (tickle_me) {
                tickle();
            }

            if (task.fiber) {
                task.fiber->resume();
                --m_activeThreadCount;
                task.reset();
            } else if (task.cb) {
                // cb_fiber 本身是有值的，调用 Fiber::reset 复用栈空间
                if (cb_fiber) {
                    cb_fiber->reset(task.cb);
                // cb_fiber 没有值，使用智能指针提供的 reset 创建
                } else {
                    cb_fiber.reset(new Fiber(task.cb));
                }
                task.reset();
                cb_fiber->resume();
                --m_activeThreadCount;
                cb_fiber.reset();
            } else {
                // 进到这里说明队列空了，调度 idle 协程即可
                /// 只有当调度器停止时，idle 协程才会退出，调度协程也随之退出
                if (idle_fiber->getState() == Fiber::TERM) {
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->resume();
                --m_idleThreadCount;
            }
        }
    }
}
