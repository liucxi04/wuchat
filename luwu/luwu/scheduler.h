//
// Created by liucxi on 2022/4/20.
//

#ifndef LUWU_SCHEDULER_H
#define LUWU_SCHEDULER_H

#include <list>
#include <vector>
#include <memory>

//#include "mutex.h"
#include "thread.h"
#include "fiber.h"

namespace liucxi {
    /**
     * @brief 协程调度器
     * */
    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        /**
         * @brief 创建调度器
         * @param use_call 将当前线程也作为调度线程
         * */
        explicit Scheduler(size_t threads = 1, bool use_call = true, std::string name = "Scheduler");

        virtual ~Scheduler();

        /**
         * @brief 添加调度任务
         * */
        template<typename FiberOrCb>
        void scheduler(FiberOrCb f, int thread = -1) {
            bool need_tickle;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = schedulerNoLock(f, thread);
            }
            if (need_tickle) {
                tickle();
            }
        }

        /**
         * @brief 启动调度器
         * */
        void start();

        /**
         * @brief 停止调度器，等所有调度任务都执行完了再返回
         * */
        void stop();

        const std::string &getName() const { return m_name; }

    protected:

        /**
         * @brief 通知协程调度器有任务了
         * */
        virtual void tickle();

        /**
         * @brief 返回是否可以停止
         * */
        virtual bool stopping();

        /**
         * @brief 无任务时执行该协程
         * */
        virtual void idle();

        /**
         * @brief 协程调度函数
         */
        void run();

        /**
         * @brief 设置当前线程的协程调度器
         * */
        void setThis();

        bool hasIdleThreads() { return m_idleThreadCount > 0; }

    public:
        /**
         * @brief 获取当前线程的调度器
         * */
        static Scheduler *GetThis();

        /**
         * @brief 获取当前线程的调度协程
         * */
        static Fiber *GetMainFiber();

    private:

        /**
         * @brief 添加调度任务，无锁
         */
        template<typename FiberOrCb>
        bool schedulerNoLock(FiberOrCb f, int thread) {
            bool need_tickle = m_fibers.empty();
            SchedulerTask task(f, thread);
            if (task.fiber || task.cb) {
                m_fibers.push_back(task);
            }
            return need_tickle;
        }

        /**
         * @brief 调度任务，可以是协程或函数
         */
        struct SchedulerTask {
            Fiber::ptr fiber = nullptr;
            std::function<void()> cb = nullptr;
            int thread = -1;

            SchedulerTask() = default;
            SchedulerTask(Fiber::ptr f, int thr)
                : fiber(std::move(f))
                , thread(thr) {
            }
            SchedulerTask(std::function<void()> f, int thr)
                : cb(std::move(f))
                , thread(thr) {
            }
            void reset() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };
    private:
        MutexType m_mutex;
        std::string m_name;                     /// 协程调度器名称
        std::vector<Thread::ptr> m_threads;     /// 线程池
        std::list<SchedulerTask> m_fibers;      /// 任务队列

        std::vector<int> m_threadIds;           /// 线程池的线程 ID 数组
        size_t m_threadCount = 0;               /// 工作线程数量，不包括 use_caller 主线程
        std::atomic<size_t> m_activeThreadCount = {0};  /// 活跃线程数量
        std::atomic<size_t> m_idleThreadCount = {0};    /// idle 线程数量

        bool m_useCaller;           /// 协程调度器所在的线程也参与调度
        Fiber::ptr m_rootFiber;     /// 调度器所在线程的调度协程，use call 为 true 时
        int m_rootThread = 0;       /// 调度器所在线程的 id，use call 为 true 时

        bool m_stopping = false;    /// 是否正在停止
    };
}
#endif //LUWU_SCHEDULER_H
