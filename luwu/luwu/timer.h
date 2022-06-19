//
// Created by liucxi on 2022/4/26.
//

#ifndef LUWU_TIMER_H
#define LUWU_TIMER_H

#include <set>
#include <memory>
#include <vector>

#include "thread.h"

namespace liucxi {
    class TimerManager;

    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;

    public:
        typedef std::shared_ptr<Timer> ptr;

        /**
         * @brief 取消定时器
         */
        bool cancel();

        /**
         * @brief 重新设置定时器的执行时间
         * @details m_next = GetCurrentMS() + m_ms;
         */
        bool refresh();

        /**
         * @brief 重置定时器
         * @param ms 定时器执行间隔时间
         * @param from_now 是否从当前时间开始计算
         */
        bool reset(uint64_t ms, bool from_now);

    private:
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);

        explicit Timer(uint64_t next) : m_next(next) {}

    private:
        bool m_recurring = false;               /// 是否是循环定时器
        uint64_t m_ms = 0;                      /// 执行周期
        uint64_t m_next = 0;                    /// 下一次执行的准确时间
        std::function<void()> m_cb;             /// 回调函数
        TimerManager *m_manager = nullptr;      /// 定时器管理器
    private:
        struct Comparator {
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    class TimerManager {
        friend class Timer;

    public:
        typedef RWMutex RWMutexType;

        TimerManager() = default;

        virtual ~TimerManager() = default;

        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

        /**
         * @brief 添加条件定时器
         * @param ms 定时器执行间隔时间
         * @param cb 定时器回调函数
         * @param weak_cond 条件
         * @param recurring 是否循环
         */
        Timer::ptr addConditionTimer(uint64_t ms, const std::function<void()> &cb,
                                     const std::weak_ptr<void> &weak_cond, bool recurring = false);

        /**
         * @brief 到最近一个定时器执行的时间间隔
         * @return
         */
        uint64_t getNextTimer();

        /**
         * @brief 获得所有需要执行的定时器的回调函数列表
         */
        void listExpiredCb(std::vector<std::function<void()>> &cbs);

    protected:
        virtual void onTimerInsertAtFront() = 0;

        void addTimer(const Timer::ptr &val, RWMutexType::WriteLock &lock);

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        bool m_tickled = false;
    };
}

#endif //LUWU_TIMER_H
