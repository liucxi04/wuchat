//
// Created by liucxi on 2022/6/13.
//

#include "luwu/luwu.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_NAME("system");
static liucxi::Logger::ptr p_logger = LUWU_LOG_ROOT();

int main() {
    g_logger->setLevel(liucxi::LogLevel::Level::DEBUG);
    LUWU_LOG_INFO(g_logger) << "hello";
    LUWU_LOG_INFO(p_logger) << "world";
    return 0;
}