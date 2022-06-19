//
// Created by liucxi on 2022/4/18.
//

#ifndef LUWU_FIBER_H
#define LUWU_FIBER_H

#include <functional>
#include <memory>
#include <ucontext.h>

namespace liucxi {
    /**
     * @brief 协程类
     * */
    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        typedef std::shared_ptr<Fiber> ptr;
        /**
         * @brief 协程状态
         * */
        enum State {
            READY,         /// 就绪态
            RUNNING,       /// 运行态
            TERM           /// 结束态
        };
    private:
        /**
         * @brief 私有构造函数
         * @attention 该无参构造函数只用于创建线程的第一个协程，也就是主线程对应的协程
         * */
        Fiber();

    public:
        /**
         * @param cb 协程入口函数
         * @param stackSize 协程栈大小
         * @param run_in_scheduler 本协程是否参与调度器调度
         * */
        explicit Fiber(std::function<void()> cb, size_t stackSize = 0, bool run_in_scheduler = true);

        ~Fiber();

        /**
         * @brief 重置协程状态和入口函数，复用栈空间，不重新创建栈
         * */
        void reset(std::function<void()> cb);

        /**
         * @brief 将当前协程切换到执行状态
         * */
        void resume();

        /**
         * @brief 将当前协程让出执行权
         * */
        void yield();

        uint64_t getId() const { return m_id; }

        State getState() const { return m_state; }

    public:

        /**
         * @brief 设置当前正在运行的协程，即设置线程局部变量 t_fiber 的值
         * @note static 全局只有一个，即进程级别，会根据当前正在执行的线程使用对应的线程局部变量，下同
         * */
        static void SetThis(Fiber *fiber);

        /**
         * @brief 返回当前正在执行的进程
         * */
        static Fiber::ptr GetThis();

        /**
         * @brief 协程入口函数
         * */
        static void MainFunc();

        static uint64_t GetFiberId();

        static uint64_t TotalFibers();

    private:
        uint64_t m_id = 0;              /// 协程 ID
        uint32_t m_stackSize = 0;       /// 协程栈大小
        State m_state = READY;          /// 协程状态
        ucontext_t m_context{};         /// 协程上下文
        void *m_stack = nullptr;        /// 协程栈地址
        std::function<void()> m_cb;     /// 协程入口函数
        bool m_runInScheduler = true;   /// 本协程是否参与调度器调度
    };
}
#endif //LUWU_FIBER_H
