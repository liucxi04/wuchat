//
// Created by liucxi on 2022/4/6.
//

#include <set>
#include <iostream>
#include <functional>
#include <cstdarg>  // va_start ...
#include <yaml-cpp/yaml.h>

#include "log.h"

namespace liucxi {

    // 通过传统的 switch case 分支完成 Level 到 string 的转化
    std::string LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
            case DEBUG:
                return "DEBUG";
            case INFO:
                return "INFO";
            case WARN:
                return "WARN";
            case ERROR:
                return "ERROR";
            case FATAL:
                return "FATAL";
            default:
                return "UNKNOWN";
        }
    }

    // 通过宏定义的方式完成 string 到 Level 的转化
    // define 定义了一个函数宏， undef 取消该定义
    // #v 给变量 v 加上双引号，即表示 v 是一个字符串， #@x 给变量 x 加上单引号，即表示 x 是一个字符
    LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) if (str == #v) { return LogLevel::level; }
        XX(DEBUG, DEBUG)
        XX(INFO, INFO)
        XX(WARN, WARN)
        XX(ERROR, ERROR)
        XX(FATAL, FATAL)
#undef XX
        return LogLevel::UNKNOWN;
    }

    LogEvent::LogEvent(std::string loggerName, LogLevel::Level level, const char *file, int32_t line, int64_t elapse, 
                       uint32_t threadId, uint64_t fiberId, time_t time, std::string threadName)
            : m_loggerName(std::move(loggerName)), m_level(level), m_file(file), m_line(line), m_elapse(elapse),
              m_threadId(threadId), m_fiberId(fiberId), m_time(time), m_threadName(std::move(threadName)) {
    }

    // 需要包含 cstdarg 和 cstdio 两个头文件
    // 这四行代码为解决不定参数格式化输出的模板
    void LogEvent::printf(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    /**
     * @note 下面许多类构造函数的 string 参数没有作用，是为了与需要 string 成员的类保持一致而存在的
     */
    class LoggerNameFormatItem : public LogFormatter::FormatItem {
    public:
        explicit LoggerNameFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getLoggerName();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        explicit LevelFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << LogLevel::ToString(event->getLevel());
        }
    };

    class FileFormatItem : public LogFormatter::FormatItem {
    public:
        explicit FileFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        explicit LineFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        explicit MessageFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        explicit ElapseFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        explicit ThreadIdFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        explicit FiberIdFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class TimeFormatItem : public LogFormatter::FormatItem {
    public:
        explicit TimeFormatItem(std::string format = "%Y-%m-%d %H:%M:%S")
                : m_format(std::move(format)) {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, LogEvent::ptr event) override {
            struct tm tm{};
            auto time = (time_t) event->getTime();
            localtime_r(&time, &tm); // 将给定的时间戳(time)转换为本地时间(tm)
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm); // 格式化时间表示
            os << buf;
        }

    private:
        std::string m_format;
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem {
    public:
        explicit ThreadNameFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        explicit NewLineFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << "\n";
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        explicit TabFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << "\t";
        }
    };

    /**
     * @brief 用来输出模板字符串里的常规字符
     * */
    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        explicit StringFormatItem(std::string str)
                : m_string(std::move(str)) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    LogFormatter::LogFormatter(std::string pattern)
            : m_pattern(std::move(pattern)) {
        init();
    }

    /**
     * @brief 状态机来实现字符串的解析
     * */
    void LogFormatter::init() {
        // 按顺序存储解析到的 pattern 项
        // 每个 pattern 包括一个整数类型和一个字符串，类型为 0 表示该 pattern 是常规字符串，为 1 表示该 pattern 需要转义
        // 日期格式单独用下面的 data_format 存储
        std::vector<std::pair<int, std::string>> patterns;
        // 临时存储常规字符串
        std::string tmp;
        // 日期格式字符串，默认把 %d 后面的内容全部当作格式字符串，不校验是否合法
        std::string data_format;
        // 是否解析出错
        bool error = false;
        // 正在解析常规字符串
        bool parsing_string = true;

        size_t i = 0;
        while (i < m_pattern.size()) {
            std::string c = std::string(1, m_pattern[i]);

            if (c == "%") {
                if (parsing_string) {
                    if (!tmp.empty()) {
                        patterns.emplace_back(0, tmp); // 在解析常规字符时遇到 %，表示开始解析模板字符
                    }
                    tmp.clear();
                    parsing_string = false;
                    ++i;
                    continue;
                }
                patterns.emplace_back(1, c); // 在解析模板字符时遇到 %，表示这里是一个转义字符
                parsing_string = true;
                ++i;
                continue;
            } else {
                if (parsing_string) {
                    tmp += c;
                    ++i;
                    continue;
                }
                patterns.emplace_back(1, c); // 模板字符直接添加，因为模板字符只有 1 个字母
                parsing_string = true;
                if (c != "d") {
                    ++i;
                    continue;
                }

                // 下面是对 %d 的特殊处理，直接取出 { } 内内容
                ++i;
                if (i < m_pattern.size() && m_pattern[i] != '{') {
                    continue; //不符合规范，不是 {
                }
                ++i;
                while (i < m_pattern.size() && m_pattern[i] != '}') {
                    data_format.push_back(m_pattern[i]);
                    ++i;
                }
                if (m_pattern[i] != '}') {
                    error = true;
                    break; //不符合规范，不是 }
                }
                ++i;
                continue;
            }
        } // end while
        if (error) {
            m_error = true;
            return;
        }
        // 模板解析最后的常规字符也要记得加进去
        if (!tmp.empty()) {
            patterns.emplace_back(0, tmp);
            tmp.clear();
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string)>> s_format_items = {
#define XX(str, C) { #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } },
                XX(m, MessageFormatItem)
                XX(p, LevelFormatItem)
                XX(c, LoggerNameFormatItem)
                XX(d, TimeFormatItem)
                XX(r, ElapseFormatItem)
                XX(f, FileFormatItem)
                XX(l, LineFormatItem)
                XX(t, ThreadIdFormatItem)
                XX(b, FiberIdFormatItem)
                XX(n, ThreadNameFormatItem)
                XX(N, NewLineFormatItem)
                XX(T, TabFormatItem)
#undef XX
        };

        for (const auto &v: patterns) {
            if (v.first == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
            } else if (v.second == "d") {
                m_items.push_back(FormatItem::ptr(new TimeFormatItem(data_format)));
            } else {
                auto it = s_format_items.find(v.second);
                if (it == s_format_items.end()) {
                    error = true;
                    break;
                } else {
                    m_items.push_back(it->second(v.second));
                }
            }
        }

        if (error) {
            m_error = true;
            return;
        }
    }

    std::string LogFormatter::format(const LogEvent::ptr &event) {
        std::stringstream ss;
        for (const auto &i: m_items) {
            i->format(ss, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &os, const LogEvent::ptr &event) {
        for (const auto &i: m_items) {
            i->format(os, event);
        }
        return os;
    }

    void StdoutLogAppender::log(LogEvent::ptr event) {
        m_formatter->format(std::cout, event);
    }

    std::string StdoutLogAppender::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["pattern"] = m_formatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(std::string filename)
            : LogAppender(std::make_shared<LogFormatter>())
            , m_filename(std::move(filename)) {
        reopen();
    }

    bool FileLogAppender::reopen() {
        MutexType::Lock lock(m_mutex);
        if (m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        m_reopenError = !m_filestream;
        return !m_reopenError;
    }

    /**
     * @note 如果一个日志事件距离上次写日志超过 3 秒，那就重新打开一次日志文件
     * 应该是为了确保日志文件可以正常写入
     * */
    void FileLogAppender::log(LogEvent::ptr event) {
        uint64_t now = event->getTime();
        if (now >= m_lastTime + 3) {
            reopen();
            m_lastTime = now;
        }
        if (m_reopenError) {
            return;
        }
        MutexType::Lock lock(m_mutex);

        m_formatter->format(m_filestream, event);
    }

    std::string FileLogAppender::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        node["pattern"] = m_formatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void Logger::addAppender(const LogAppender::ptr &appender) {
        MutexType::Lock lock(m_mutex);
        m_appenderList.push_back(appender);
    }

    void Logger::delAppender(const LogAppender::ptr &appender) {
        MutexType::Lock lock(m_mutex);
        m_appenderList.remove(appender);
    }

    void Logger::clearAppenders() {
        MutexType::Lock lock(m_mutex);
        m_appenderList.clear();
    }

    void Logger::log(const LogEvent::ptr &event) {
        if (event->getLevel() >= m_level) {
            for (const auto &i: m_appenderList) {
                i->log(event);
            }
        }
    }

    std::string Logger::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        for (const auto &i : m_appenderList) {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LoggerManager::LoggerManager() {
        m_root.reset(new Logger("root")); // 智能指针的 reset 方法
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
    }

    std::string LoggerManager::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (const auto &i : m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name) {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        return logger;
    }
}
