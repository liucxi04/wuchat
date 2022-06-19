//
// Created by liucxi on 2022/4/18.
//

#include "fiber.h"
//#include "macro.h"
#include "config.h"
#include "scheduler.h"

#include <utility>

namespace liucxi {
    static Logger::ptr g_logger = LUWU_LOG_NAME("system");

    /// 全局静态变量，用于生成协程 ID
    static std::atomic<uint64_t> s_fiberId{0};
    /// 全局静态变量，用于统计当前的协程数量（所有线程的）
    static std::atomic<uint64_t> s_fiberCount{0};

    /// 线程局部变量，当前线程正在运行的协程
    static thread_local Fiber *t_fiber = nullptr;
    /// 线程局部变量，当前线程的主协程
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    static ConfigVar<uint32_t>::ptr g_fiberStackSize =
            Config::lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

    /**
     * @brief 栈内存分配器
     * */
    class StackAllocator {
    public:
        static void *Alloc(size_t size) {
            return malloc(size);
        }
        static void Dealloc(void *vp) {
            return free(vp);
        }
    };

    Fiber::Fiber() {
        SetThis(this);
        m_state = RUNNING;

        if (getcontext(&m_context)) {
            LUWU_ASSERT2(false, "getcontext")
        }

        ++s_fiberCount;
        m_id = s_fiberId++;
    }

    /**
     * @brief 带参数的构造函数用于创建其他协程，需要分配栈
     * */
    Fiber::Fiber(std::function<void()> cb, size_t stackSize, bool run_in_scheduler)
        : m_id(s_fiberId++)
        , m_cb(std::move(cb))
        , m_runInScheduler(run_in_scheduler) {
        ++s_fiberCount;
        m_stackSize = stackSize ? stackSize : g_fiberStackSize->getValue();
        m_stack = StackAllocator::Alloc(m_stackSize);

        if (getcontext(&m_context)) {
            LUWU_ASSERT2(false, "getcontext")
        }

        m_context.uc_link = nullptr;
        m_context.uc_stack.ss_sp = m_stack;
        m_context.uc_stack.ss_size = m_stackSize;

        makecontext(&m_context, &Fiber::MainFunc, 0);
    }

    /**
     * @brief 线程主协程析构时需要特殊处理，因为主协程没有分配栈和协程入口函数
     * */
    Fiber::~Fiber() {
        --s_fiberCount;
        if (m_stack) {
            // 有栈，说明是子协程，需要确保子协程一定是结束状态
            LUWU_ASSERT(m_state == TERM)
            StackAllocator::Dealloc(m_stack);
        } else {
            // 没有栈，说明是线程主协程
            LUWU_ASSERT(!m_cb)                 // 主协程没有协程入口函数
            LUWU_ASSERT(m_state == RUNNING)    // 主协程一定是执行状态
            Fiber *cur = t_fiber;
            if (cur == this) {
                SetThis(nullptr);
            }
        }
    }

    /**
     * @note 这里只有 TERM 状态的协程才可以重置
     * */
    void Fiber::reset(std::function<void()> cb) {
        LUWU_ASSERT(m_stack)
        LUWU_ASSERT(m_state == TERM)
        m_cb = std::move(cb);

        if (getcontext(&m_context)) {
            LUWU_ASSERT2(false, "getcontext")
        }

        m_context.uc_link = nullptr;
        m_context.uc_stack.ss_sp = m_stack;
        m_context.uc_stack.ss_size = m_stackSize;

        makecontext(&m_context, &Fiber::MainFunc, 0);
        m_state = READY;
    }

    void Fiber::resume() {
        LUWU_ASSERT(m_state == READY)
        SetThis(this);
        m_state = RUNNING;

        // 如果协程参与调度器调度，那么应该和该线程的调度协程进行交换，而不是和线程主协程
        if (m_runInScheduler) {
            if (swapcontext(&(Scheduler::GetMainFiber()->m_context), &m_context)) {
                LUWU_ASSERT2(false, "swapcontext error")
            }
        } else {
            if (swapcontext(&(t_threadFiber->m_context), &m_context)) {
                LUWU_ASSERT2(false, "swapcontext error")
            }
        }
    }

    void Fiber::yield() {
        /// 协程运行完之后会自动 yield 一次，用于回到主协程，此时为结束状态
        LUWU_ASSERT(m_state == TERM || m_state == RUNNING)
        SetThis(t_threadFiber.get());
        if (m_state == RUNNING) {
            m_state = READY;
        }

        // 如果协程参与调度器调度，那么应该和该线程的调度协程进行交换，而不是和线程主协程
        if (m_runInScheduler) {
            if (swapcontext(&m_context, &(Scheduler::GetMainFiber()->m_context))) {
                LUWU_ASSERT2(false, "swapcontext error")
            }
        } else {
            if (swapcontext(&m_context, &(t_threadFiber->m_context))) {
                LUWU_ASSERT2(false, "swapcontext error")
            }
        }
    }

    void Fiber::SetThis(Fiber *fiber) {
        t_fiber = fiber;
    }

    /**
     * @brief 获取当前协程，同时充当初始化线程主协程的作用，这个函数在使用协程之前要调用一下
     * */
    Fiber::ptr Fiber::GetThis() {
        // 当前子协程存在，直接返回
        if (t_fiber) {
            return t_fiber->shared_from_this();
        }

        // 走到这说明当前线程还没有创建协程，那么使用私有构造函数创建线程主协程
        Fiber::ptr main_fiber(new Fiber);
        LUWU_ASSERT(t_fiber == main_fiber.get())
        t_threadFiber = main_fiber;
        // 创建完成后，线程主协程和当前正在运行的协程都是这个
        return t_fiber->shared_from_this();
    }

    uint64_t Fiber::TotalFibers() {
        return s_fiberCount;
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }

    /**
     * @brief 协程入口函数，内部调用用户提供的协程入口函数
     * */
    void Fiber::MainFunc() {
        Fiber::ptr cur = GetThis();
        LUWU_ASSERT(cur)

        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;

        /// 手动让 t_fiber 的引用计数减 1
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->yield();
    }

}

