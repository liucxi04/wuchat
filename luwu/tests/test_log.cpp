//
// Created by liucxi on 2022/4/7.
//

#include <iostream>
#include "luwu/macro.h"
using namespace liucxi;

int main() {

    Logger::ptr g_logger = LUWU_LOG_ROOT(); // 默认 DEBUG 级别

    StdoutLogAppender::ptr appender1(new StdoutLogAppender);
    StdoutLogAppender::ptr appender2(new StdoutLogAppender);

    Logger::ptr logger(new Logger);
    logger->addAppender(appender1);
    logger->addAppender(appender2);

    logger->delAppender(appender2);

    std::cout << LogLevel::ToString(g_logger->getLevel()) << std::endl;
    std::cout << "********1********"<<std::endl;
    LUWU_LOG_FATAL(g_logger) << "fatal msg";
    LUWU_LOG_ERROR(g_logger) << "err msg";
    LUWU_LOG_WARN(g_logger) << "warn msg";
    LUWU_LOG_INFO(g_logger) << "info msg";
    LUWU_LOG_DEBUG(g_logger) << "debug msg";

    std::cout << "********2********"<<std::endl;
    LUWU_LOG_FMT_FATAL(g_logger, "fatal %s:%d ", __FILE__, __LINE__);
    LUWU_LOG_FMT_ERROR(g_logger, "err %s:%d ", __FILE__, __LINE__);
    LUWU_LOG_FMT_WARN(g_logger, "warn %s:%d ", __FILE__, __LINE__);
    LUWU_LOG_FMT_INFO(g_logger, "info %s:%d ", __FILE__, __LINE__);
    LUWU_LOG_FMT_DEBUG(g_logger, "debug %s:%d ", __FILE__, __LINE__);

    std::cout << "********3********"<<std::endl;
    setThreadName("brand_new_thread");
    g_logger->setLevel(LogLevel::WARN);
    LUWU_LOG_FATAL(g_logger) << "fatal msg";
    LUWU_LOG_ERROR(g_logger) << "err msg";
    LUWU_LOG_WARN(g_logger) << "warn msg";
    LUWU_LOG_INFO(g_logger) << "info msg";// no
    LUWU_LOG_DEBUG(g_logger) << "debug msg";// no


    std::cout << "********4********"<<std::endl;
    FileLogAppender::ptr fileAppender(new FileLogAppender("log.txt"));
    g_logger->addAppender(fileAppender);
    LUWU_LOG_FATAL(g_logger) << "fatal msg";
    LUWU_LOG_ERROR(g_logger) << "err msg";
    LUWU_LOG_WARN(g_logger) << "warn msg";
    LUWU_LOG_INFO(g_logger) << "info msg";// no
    LUWU_LOG_DEBUG(g_logger) << "debug msg"; // no

    std::cout << "********5********"<<std::endl;
    Logger::ptr test_logger = LUWU_LOG_NAME("test_logger");
    StdoutLogAppender::ptr appender(new StdoutLogAppender);
    LogFormatter::ptr formatter(new LogFormatter("%d:%rms%T%p%T%c%T%f:%l %m %n")); // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
    appender->setFormatter(formatter);
    test_logger->addAppender(appender);
    test_logger->setLevel(LogLevel::WARN);

    LUWU_LOG_ERROR(test_logger) << "err msg";
    LUWU_LOG_INFO(test_logger) << "info msg"; // no
    return 0;
}
/*
int main() {
    // 定义日志器，默认名字为 default
    Logger::ptr logger(new Logger);
    // 定义日志输出地，此处选择了标准输出
    LogAppender::ptr logAppender1(new StdoutLogAppender);
    // 定义日志格式
    LogFormatter::ptr logFormatter(new LogFormatter("[%rms]%T%t%T%n%T%b%T[%p]%T[%c]%T%f:%l%T%m%N"));

    // 为日志输出地设置格式
    logAppender1->setFormatter(logFormatter);
    // 为日志器添加输出地
    logger->addAppender(logAppender1);

    // 流式和 fmt 方式输出日志
    LUWU_LOG_INFO(logger) << "test log";
    LUWU_LOG_FMT_INFO(logger, "%c ", 'x');
    return 0;
}*/
/*
int main() {
    // 定义日志器，默认名字为 default
    Logger::ptr logger(new Logger);
    // 定义日志输出地，此处选择了标准输出
    LogAppender::ptr logAppender(new FileLogAppender("/home/liucxi/Documents/luwu/logs/fff.txt"));
    // 定义日志格式
    LogFormatter::ptr logFormatter(new LogFormatter("[%rms]%T%t%T%n%T%b%T[%p]%T[%c]%T%f:%l%T%m%N"));

    // 为日志输出地设置格式
    logAppender->setFormatter(logFormatter);
    // 为日志器添加输出地
    logger->addAppender(logAppender);

    // 流式和 fmt 方式输出日志
    LUWU_LOG_INFO(logger) << "test log";
    return 0;
}*/

