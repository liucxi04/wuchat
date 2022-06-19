//
// Created by liucxi on 2022/4/16.
//

#include "thread.h"
#include "macro.h"


namespace liucxi {
    /// 线程局部变量，指向当前正在执行的线程对象
    static thread_local Thread *t_thread;
    /// 线程局部变量，当前正在执行的线程的名称
    static thread_local std::string t_thread_name = "UNKNOWN";

    Thread::Thread(std::function<void()> callback, std::string name)
        : m_callback(std::move(callback))
        , m_name(std::move(name)){

        if (m_name.empty()) {
            m_name = "UNKNOWN";
        }
        // 创建线程之后，内核就已经开始调用 run 方法了。
        int rt = pthread_create(&m_thread, nullptr, &run, this);
        if (rt) {
            LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "pthread_create fail, rt=" << rt << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        // 只有 run 方法里添加了信号量，这里才能通过，否则一直阻塞在这里
        m_sem.wait();
    }

    Thread::~Thread() {
        // 如果主线程结束时子线程还没结束，那么分离主线程和子线程
        if (m_thread) {
            pthread_detach(m_thread);
        }
    }

    Thread *Thread::GetThis() {
        return t_thread;
    }

    const std::string &Thread::GetName() {
        return t_thread_name;
    }

    void Thread::SetName(const std::string &name) {
        if (name.empty()) {
            return;
        }
        if (t_thread) {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    void Thread::join() {
        if (m_thread) {
            int rt = pthread_join(m_thread, nullptr);
            if (rt) {
                LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "pthread_join fail, rt=" << rt << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    void *Thread::run(void *arg) {
        auto *thread = (Thread *)arg;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = liucxi::getThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> callback;
        callback.swap(thread->m_callback);

        thread->m_sem.notify();
        callback();
        return nullptr;
    }
}
