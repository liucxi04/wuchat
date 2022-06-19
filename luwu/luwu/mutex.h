//
// Created by liucxi on 2022/4/16.
//

#ifndef LUWU_MUTEX_H
#define LUWU_MUTEX_H

#include <semaphore.h>
#include <atomic>
#include <stdexcept>
#include "noncopyable.h"

namespace liucxi {
    /**
     * @brief 信号量
     * */
    class Semaphore : Noncopyable {
    public:
        explicit Semaphore(uint32_t count = 0) {
            if (sem_init(&m_sem, 0, count)) {
                throw std::logic_error("sem_init error");
            }
        }

        ~Semaphore() {
            sem_destroy(&m_sem);
        }

        void wait() {
            if (sem_wait(&m_sem)) {
                throw std::logic_error("sem_wait error");
            }
        }

        void notify() {
            if (sem_post(&m_sem)) {
                throw std::logic_error("sem_post error");
            }
        }

    private:
        sem_t m_sem{};
    };

    /**
     * @brief 局部锁
     * */
    template<typename T>
    class ScopedLockImpl {
    public:
        explicit ScopedLockImpl(T &mutex)
                : m_mutex(mutex) {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    /**
     * @brief 局部读锁
     * */
    template<typename T>
    class ReadScopedLockImpl {
    public:
        explicit ReadScopedLockImpl(T &mutex)
                : m_mutex(mutex) {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    /**
     * @brief 局部写锁
     * */
    template<typename T>
    class WriteScopedLockImpl {
    public:
        explicit WriteScopedLockImpl(T &mutex)
                : m_mutex(mutex) {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked = false;
    };

    /**
     * @brief 互斥量
     * */
    class Mutex : Noncopyable {
    public:
        typedef ScopedLockImpl<Mutex> Lock;

        Mutex() {
            pthread_mutex_init(&m_mutex, nullptr);
        }

        ~Mutex() {
            pthread_mutex_destroy(&m_mutex);
        }

        void lock() {
            pthread_mutex_lock(&m_mutex);
        }

        void unlock() {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex{};
    };

    /**
     * @brief 空锁 用于调试
     * */
    class NullMutex : Noncopyable {
        typedef ScopedLockImpl<NullMutex> Lock;

        NullMutex() = default;

        ~NullMutex() = default;

        void lock() {}

        void unlock() {}
    };

    /**
     * @brief 读写互斥量
     * */
    class RWMutex : Noncopyable {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        RWMutex() {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex() {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock() {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock() {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock() {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock{};
    };

    /**
     * @brief 空读写互斥量
     * */
    class NullRWMutex : Noncopyable {
    public:
        typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
        typedef WriteScopedLockImpl<NullRWMutex> WriteLock;

        NullRWMutex() = default;

        ~NullRWMutex() = default;

        void rdlock() {}

        void wrlock() {}
    };

    /**
     * @brief 自旋锁
     * */
    class Spinlock : Noncopyable {
    public:
        typedef ScopedLockImpl<Spinlock> Lock;

        Spinlock() {
            pthread_spin_init(&m_mutex, 0);
        }

        ~Spinlock() {
            pthread_spin_destroy(&m_mutex);
        }

        void lock() {
            pthread_spin_lock(&m_mutex);
        }

        void unlock() {
            pthread_spin_unlock(&m_mutex);
        }

    private:
        pthread_spinlock_t m_mutex{};
    };

    /**
     * @brief 原子锁
     * */
    class CASLock : Noncopyable {
    public:
        typedef ScopedLockImpl<CASLock> Lock;
        CASLock() {
            m_mutex.clear();
        }

        ~CASLock() = default;

        void lock() {
            while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }

        void unlock() {
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }
    private:
        volatile std::atomic_flag m_mutex{};
    };
}
#endif //LUWU_MUTEX_H
