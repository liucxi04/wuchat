//
// Created by liucxi on 2022/4/18.
//

#include "luwu/fiber.h"
#include "luwu/macro.h"
#include "luwu/thread.h"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace liucxi;

liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

void run_in_fiber2() {
    LUWU_LOG_DEBUG(g_logger) << "run_in_fiber2 begin";
    LUWU_LOG_DEBUG(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    LUWU_LOG_DEBUG(g_logger) << "run_in_fiber begin";

    LUWU_LOG_DEBUG(g_logger) << "before run_in_fiber yield";
    liucxi::Fiber::GetThis()->yield();
    LUWU_LOG_DEBUG(g_logger) << "after run_in_fiber yield";

    LUWU_LOG_DEBUG(g_logger) << "run_in_fiber end";
    // fiber结束之后会自动返回主协程运行
}

void test_fiber() {
    LUWU_LOG_DEBUG(g_logger) << "test_fiber begin";

    // 初始化线程主协程
    liucxi::Fiber::GetThis();

    liucxi::Fiber::ptr fiber(new liucxi::Fiber(run_in_fiber, 0, false));
    LUWU_LOG_DEBUG(g_logger) << "use_count:" << fiber.use_count(); // 1

    LUWU_LOG_DEBUG(g_logger) << "before test_fiber resume";
    fiber->resume();
    LUWU_LOG_DEBUG(g_logger) << "after test_fiber resume";

    /**
     * 关于fiber智能指针的引用计数为3的说明：
     * 一份在当前函数的fiber指针，一份在MainFunc的cur指针
     * 还有一份在在run_in_fiber的GetThis()结果的临时变量里
     */
    LUWU_LOG_DEBUG(g_logger) << "use_count:" << fiber.use_count(); // 3

    LUWU_LOG_DEBUG(g_logger) << "fiber status: " << fiber->getState(); // READY

    LUWU_LOG_DEBUG(g_logger) << "before test_fiber resume again";
    fiber->resume();
    LUWU_LOG_DEBUG(g_logger) << "after test_fiber resume again";

    LUWU_LOG_DEBUG(g_logger) << "use_count:" << fiber.use_count(); // 1
    LUWU_LOG_DEBUG(g_logger) << "fiber status: " << fiber->getState(); // TERM

    fiber->reset(run_in_fiber2); // 上一个协程结束之后，复用其栈空间再创建一个新协程
    fiber->resume();

    LUWU_LOG_DEBUG(g_logger) << "use_count:" << fiber.use_count(); // 1
    LUWU_LOG_DEBUG(g_logger) << "test_fiber end";
}

int main(int argc, char *argv[]) {

    liucxi::setThreadName("main_thread");
    LUWU_LOG_DEBUG(g_logger) << "main begin";

    std::vector<liucxi::Thread::ptr> thrs;
    thrs.reserve(2);
    for (int i = 0; i < 2; i++) {
        thrs.push_back(std::make_shared<liucxi::Thread>(
                &test_fiber, "thread_" + std::to_string(i)));
    }

    for (const auto &i: thrs) {
        i->join();
    }

    LUWU_LOG_DEBUG(g_logger) << "main end";
    return 0;
}

