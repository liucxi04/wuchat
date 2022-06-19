//
// Created by liucxi on 2022/4/12.
//

#ifndef LUWU_THREAD_H
#define LUWU_THREAD_H

#include <memory>
#include <functional>

#include "mutex.h"

namespace liucxi {
    /**
     * @brief 线程类
     * */
    class Thread : Noncopyable {
    public:
        typedef std::shared_ptr<Thread> ptr;

        /**
         * @brief 构造函数
         * @param callback 线程执行函数
         * @param name 线程名
         */
        Thread(std::function<void()> callback, std::string name);

        /**
         * @brief 析够函数
         * @details 使用 detach 系统调用使得主线程与子线程分离，子线程结束后，资源自动回收
         * @note 如果用户在构造函数之后调用了 join，那么这里的 detach 就没有作用了，因为 join 之后子线程已经结束了。
         */
        ~Thread();

        pid_t getId() const { return m_id; }

        const std::string &getName() const { return m_name; }

        /**
         * @brief 等待线程执行完成
         * @details 构造函数返回时，线程入口函数已经在执行了，join 方法是为了让主线程等待子线程执行完成。
         * */
        void join();

        /**
         * @brief 获取当前线程指针
         * @details 一个进程下面有多个线程，也就是说有多个 Thread 实例，
         * 使用此方法获得当前正在使用 CPU 的线程对象。
         * 下同
         * */
        static Thread *GetThis();

        /**
         * @brief 获取当前线程名称
         * */
        static const std::string &GetName();

        /**
         * @brief 设置当前线程名称
         * */
        static void SetName(const std::string &name);

    private:
        /**
         * @brief 线程执行函数
         * */
        static void *run(void *arg);

    private:
        pid_t m_id = -1;                    /// 线程 id
        pthread_t m_thread = 0;             /// 线程结构，线程标识符
        std::function<void()> m_callback;   /// 线程执行函数，无参数，无返回值
        std::string m_name;                 /// 线程名

        Semaphore m_sem;
    };
}

#endif //LUWU_THREAD_H
