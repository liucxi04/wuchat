//
// Created by liucxi on 2022/4/18.
//

#ifndef LUWU_MACRO_H
#define LUWU_MACRO_H

#include "log.h"
//#include "utils.h"
#include <cstring>
#include <cassert>

/**
 * @brief 获取 root 日志器，默认级别为 DEBUG
 * */
#define LUWU_LOG_ROOT() liucxi::LoggerMgr::getInstance()->getRoot()

/**
 * @brief 获取指定名称的日志器
 * */
#define LUWU_LOG_NAME(name) liucxi::LoggerMgr::getInstance()->getLogger(name)

/**
 * @brief 流式输出
 * */
#define LUWU_LOG_LEVEL(logger, level) \
    if (level >= logger->getLevel())  \
        liucxi::LogEventWrap(logger, liucxi::LogEvent::ptr(new liucxi::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, liucxi::getElapseMS() - logger->getCreateTime(), \
            liucxi::getThreadId(), liucxi::getFiberId(), time(nullptr), liucxi::getThreadName()))).getLogEvent() \
            ->getSS()

#define LUWU_LOG_DEBUG(logger) LUWU_LOG_LEVEL(logger, liucxi::LogLevel::DEBUG)
#define LUWU_LOG_INFO(logger) LUWU_LOG_LEVEL(logger, liucxi::LogLevel::INFO)
#define LUWU_LOG_WARN(logger) LUWU_LOG_LEVEL(logger, liucxi::LogLevel::WARN)
#define LUWU_LOG_ERROR(logger) LUWU_LOG_LEVEL(logger, liucxi::LogLevel::ERROR)
#define LUWU_LOG_FATAL(logger) LUWU_LOG_LEVEL(logger, liucxi::LogLevel::FATAL)

/**
 * @brief fmt 输出
 * @note 流式输出先输出日志现场，再输出日志信息， fmt 输出则相反
 * */
#define LUWU_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(level >= logger->getLevel())                 \
        liucxi::LogEventWrap(logger, liucxi::LogEvent::ptr(new liucxi::LogEvent(logger->getName(), \
            level, __FILE__, __LINE__, liucxi::getElapseMS() - logger->getCreateTime(), \
            liucxi::getThreadId(), liucxi::getFiberId(), time(nullptr), liucxi::getThreadName()))).getLogEvent() \
            ->printf(fmt, __VA_ARGS__)

#define LUWU_LOG_FMT_DEBUG(logger, fmt, ...) LUWU_LOG_FMT_LEVEL(logger, liucxi::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LUWU_LOG_FMT_INFO(logger, fmt, ...) LUWU_LOG_FMT_LEVEL(logger, liucxi::LogLevel::INFO, fmt, __VA_ARGS__)
#define LUWU_LOG_FMT_WARN(logger, fmt, ...) LUWU_LOG_FMT_LEVEL(logger, liucxi::LogLevel::WARN, fmt, __VA_ARGS__)
#define LUWU_LOG_FMT_ERROR(logger, fmt, ...) LUWU_LOG_FMT_LEVEL(logger, liucxi::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LUWU_LOG_FMT_FATAL(logger, fmt, ...) LUWU_LOG_FMT_LEVEL(logger, liucxi::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LUWU_ASSERT(x) \
    if (!(x)) {        \
        LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "ASSERTION: " #x \
                                        << "\nbacktrace:\n" \
                                        << liucxi::BacktraceToString(64, 2, "    "); \
        assert(x);                                \
    }

#define LUWU_ASSERT2(x, w) \
    if (!(x)) {        \
        LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "ASSERTION: " #x \
                                        << "\n" \
                                        << w \
                                        << "\nbacktrace:\n" \
                                        << liucxi::BacktraceToString(64, 2, "    "); \
        assert(x);                                \
    }
#endif //LUWU_MACRO_H
