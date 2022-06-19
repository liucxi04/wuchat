//
// Created by liucxi on 2022/4/26.
//

#include "timer.h"
#include "utils.h"

namespace liucxi {

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const {
        if (!lhs && !rhs) {
            return false;
        }
        if (!lhs) {
            return true;
        }
        if (!rhs) {
            return false;
        }

        if (lhs->m_next < rhs->m_next) {
            return true;
        }
        if (lhs->m_next > rhs->m_next) {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
            : m_recurring(recurring), m_ms(ms), m_cb(std::move(cb)), m_manager(manager) {
        m_next = GetCurrentMS() + m_ms;
    }

    bool Timer::cancel() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            auto it = m_manager->m_timers.find(shared_from_this());
            if (it == m_manager->m_timers.end()) {
                return false;
            }
            m_manager->m_timers.erase(it);
            m_next = GetCurrentMS() + m_ms;
            m_manager->m_timers.insert(shared_from_this());
            return true;
        }
        return false;
    }

    bool Timer::reset(uint64_t ms, bool from_now) {
        if (ms == m_ms && !from_now) {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            auto it = m_manager->m_timers.find(shared_from_this());
            if (it == m_manager->m_timers.end()) {
                return false;
            }
            m_manager->m_timers.erase(it);
            uint64_t start;
            if (from_now) {
                start = GetCurrentMS();
            } else {
                start = m_next - m_ms;
            }
            m_ms = ms;
            m_next = start + m_ms;
            // 有可能出现在队头
            m_manager->addTimer(shared_from_this(), lock);
            return true;
        }
        return false;
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, std::move(cb), recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }

    void TimerManager::addTimer(const Timer::ptr &val, RWMutexType::WriteLock &lock) {
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front) {
            m_tickled = true;
        }
        lock.unlock();

        if (at_front) {
            onTimerInsertAtFront();
        }
    }

    static void OnTimer(const std::weak_ptr<void> &weak_cond, const std::function<void()> &cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp) {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, const std::function<void()> &cb,
                                               const std::weak_ptr<void> &weak_cond, bool recurring) {
        return addTimer(ms, [weak_cond, cb] { return OnTimer(weak_cond, cb); }, recurring);
    }

    uint64_t TimerManager::getNextTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty()) {
            return ~0ull;
        }

        const Timer::ptr &next = *m_timers.begin();
        uint64_t now_ms = GetCurrentMS();
        if (now_ms >= next->m_next) {
            return 0;
        } else {
            return next->m_next - now_ms;
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
        uint64_t now_ms = GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty()) {
                return;
            }
        }

        RWMutexType::WriteLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
        Timer::ptr now_timer(new Timer(now_ms));
        auto it = m_timers.upper_bound(now_timer);
        expired.insert(expired.begin(), m_timers.begin(), it);
        cbs.reserve(expired.size());

        for (auto &timer: expired) {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring) {
                m_timers.erase(timer);
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            } else {
                m_timers.erase(timer);
//                timer->m_cb = nullptr;
            }
        }
    }


}
