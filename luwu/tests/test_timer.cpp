//
// Created by liucxi on 2022/4/27.
//

#include <iostream>
#include "luwu/config.h"
#include "luwu/luwu.h"
#include "luwu/timer.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

static int timeout = 1000;
static liucxi::Timer::ptr s_timer;

void timer_callback() {
    LUWU_LOG_INFO(g_logger) << "timer callback, timeout = " << timeout;
    timeout += 1000;
    if(timeout < 5000) {
        s_timer->reset(timeout, true);
    } else {
        s_timer->cancel();
    }
}

int main() {
    liucxi::IOManager iom;

    // 循环定时器
    s_timer = iom.addTimer(1000, timer_callback, true);
    // 单次定时器
    iom.addTimer(500, []{
        LUWU_LOG_INFO(g_logger) << "500ms timeout";
    });
    iom.addTimer(5000, []{
        LUWU_LOG_INFO(g_logger) << "5000ms timeout";
    });
    return 0;
}