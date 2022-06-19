//
// Created by liucxi on 2022/4/6.
//

#ifndef LUWU_LOG_H
#define LUWU_LOG_H

#include <map>
#include <list>
#include <vector>
#include <memory>  // shared_ptr
#include <string>
#include <sstream>
#include <fstream>

#include "utils.h"
#include "mutex.h"

namespace liucxi {

    /**
     * @brief 定义日志级别，以及日志级别与字符串的相互转化
     * */
    class LogLevel {
    public:
        enum Level {
            /**
             * @note 默认输出级别以上的日志，设置为 UNKNOWN 则不会输出任何日志
             */
            UNKNOWN = 100,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static std::string ToString(LogLevel::Level level);

        static LogLevel::Level FromString(const std::string &str);
    };

    /**
     * @brief 定义日志事件，保存日志现场的所有信息
     * */
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        /**
         * @note 为9个数据成员进行了赋值，m_ss 成员 TODO
         * */
        LogEvent(std::string loggerName, LogLevel::Level level, const char *file, int32_t line, int64_t elapse,
                 uint32_t threadId, uint64_t fiberId, time_t time, std::string threadName);

        const std::string &getLoggerName() const { return m_loggerName; }

        LogLevel::Level getLevel() const { return m_level; }

        /**
         * @note getSS() 不可返回 const 类型
         * @details 返回的是一个 string stream 的引用，在宏定义的最后会给 m_ss 赋值
         * */
        std::stringstream &getSS() { return m_ss; }

        std::string getContent() const { return m_ss.str(); }

        const char *getFile() const { return m_file; }

        int32_t getLine() const { return m_line; }

        uint32_t getElapse() const { return m_elapse; }

        uint32_t getThreadId() const { return m_threadId; }

        uint32_t getFiberId() const { return m_fiberId; }

        uint64_t getTime() const { return m_time; }

        const std::string &getThreadName() const { return m_threadName; }

        /**
         * @brief 实现了不定参数的格式化输出
         */
        static void printf(const char *fmt, ...);

    private:
        std::string m_loggerName;       /// 日志器名称
        LogLevel::Level m_level;        /// 日志级别
        std::stringstream m_ss;         /// 日志内容，使用 string_stream 存储，方便流式输出
        const char *m_file = nullptr;   /// 文件名，__FILE__
        int32_t m_line = 0;             /// 行号，__LINE__
        uint32_t m_elapse = 0;          /// 程序启动到现在的毫秒数
        uint32_t m_threadId = 0;        /// 线程 ID
        uint32_t m_fiberId = 0;         /// 协程 ID
        uint64_t m_time;                /// 时间戳
        std::string m_threadName;       /// 线程名称
    };

    /**
     * @brief 定义日志格式器，用来将一个日志事件转化成指定的格式
     * */
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        /**
        * @brief 构造函数
        * @details 参数说明：
        * - %m 消息
        * - %p 日志级别
        * - %c 日志器名称
        * - %d 日期时间，后面跟一对括号指定时间格式，如%d{%Y-%m-%d %H:%M:%S}
        * - %r 程序运行到现在的毫秒数
        * - %f 文件名
        * - %l 行号
        * - %t 线程id
        * - %b 协程id
        * - %n 线程名称
        * - %T 制表符
        * - %N 换行
        *
        * 默认格式："%d{%Y-%m-%d %H:%M:%S}%T[%rms]%T%t%T%n%T%b%T[%p]%T[%c]%T%f:%l%T%m%N"
        * 格式描述：年-月-日 时:分:秒 [累计运行毫秒数] 线程id 线程名称 协程id [日志级别] [日志器名称] 文件名:行号 日志消息
        */
        explicit LogFormatter(std::string pattern =
        "%d{%Y-%m-%d %H:%M:%S}%T[%rms]%T%t%T%n%T%b%T[%p]%T[%c]%T%f:%l%T%m%N");

        std::string format(const LogEvent::ptr &event);

        std::ostream &format(std::ostream &os, const LogEvent::ptr &event);

        /**
         * 字符串解析
         * */
        void init();

        bool getError() const { return m_error; }

        const std::string &getPattern() const { return m_pattern; }

    public:
        /**
         * @brief 成员类，用来控制某一类型的格式输出，需要被继承以实现 format 方法
         * */
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;

            virtual ~FormatItem() = default;

            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
        };

    private:
        bool m_error = false;  /// 解析是否出错
        std::string m_pattern; /// 日志输出的模板
        std::vector<FormatItem::ptr> m_items; /// 解析完模板字符串得到的一组按序的 FormatItem 对象
    };

    /**
     * @brief 定义日志输出器，需要被继承
     * */
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Spinlock MutexType;

        explicit LogAppender(LogFormatter::ptr formatter)
                : m_formatter(std::move(formatter)) {
        };

        virtual ~LogAppender() = default;

        virtual std::string toYamlString() = 0;

        /**
         * @brief 将日志事件进行输出，需要子类实现
         * */
        virtual void log(LogEvent::ptr event) = 0;

        void setFormatter(const LogFormatter::ptr &formatter) {
            MutexType::Lock lock(m_mutex);
            m_formatter = formatter;
        }

        LogFormatter::ptr getFormatter() {
            MutexType::Lock lock(m_mutex);
            return m_formatter;
        }

    protected:
        MutexType m_mutex;
        LogFormatter::ptr m_formatter;
    };

    /**
     * @brief 输出到控制台的 LogAppender
     * */
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        /**
         * @brief StdoutLogAppender 构造函数调用父类构造函数，传入一个 LogFormatter 它有一个默认格式
         * */
        StdoutLogAppender()
                : LogAppender(std::make_shared<LogFormatter>()) {
        };

        void log(LogEvent::ptr event) override;

        std::string toYamlString() override;
    };

    /**
     * @brief 输出到文件的 LogAppender
     * */
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        explicit FileLogAppender(std::string filename);

        void log(LogEvent::ptr event) override;

        std::string toYamlString() override;
        /**
         * @brief 重新打开文件，文件打开成功返回 true
         * */
        bool reopen();

    private:
        bool m_reopenError = false;
        std::string m_filename;
        std::ofstream m_filestream;
        uint64_t m_lastTime = 0;        /// 文件上次打开时间
    };

    /**
     * @brief 日志器，含有多个日志输出地，默认日志级别为最低的 LogLevel::DEBUG
     * */
    class Logger {
    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        explicit Logger(std::string name = "default")
                : m_name(std::move(name))
                , m_level(LogLevel::DEBUG)
                , m_creatTime(getElapseMS()){
        };

        void log(const LogEvent::ptr &event);

        std::string toYamlString();

        const std::string &getName() const { return m_name; }

        LogLevel::Level getLevel() const { return m_level; };

        void setLevel(LogLevel::Level level) { m_level = level; }

        void addAppender(const LogAppender::ptr &appender);

        void delAppender(const LogAppender::ptr &appender);

        void clearAppenders();

        uint64_t getCreateTime() const { return m_creatTime; }

    private:
        MutexType m_mutex;
        std::string m_name;                                 /// 日志名称
        LogLevel::Level m_level;                            /// 日志级别
        std::list<LogAppender::ptr> m_appenderList;         /// 输出地列表
        uint64_t m_creatTime{};                             /// 创建时间
    };

    /**
     * @brief 日志器和日志事件的包装类
     * @note 这个类的最大特点是在析构时打印日志事件，所以我们可以临时构造 LogEventWrap 对象快速打印一个事件
     * */
    class LogEventWrap {
    public:
        LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
                : m_logger(std::move(logger)), m_event(std::move(event)) {
        };

        ~LogEventWrap() {
            m_logger->log(m_event);
        };

        LogEvent::ptr getLogEvent() const { return m_event; }

    private:
        Logger::ptr m_logger;
        LogEvent::ptr m_event;
    };

    /**
     * @brief 日志器的管理类，内含一个默认的 root 日志器，还有多个用户添加的 Logger
     * */
    class LoggerManager {
    public:
        typedef Spinlock MutexType;
        LoggerManager();

        Logger::ptr getLogger(const std::string &name);

        Logger::ptr getRoot() const { return m_root; }

        std::string toYamlString();

    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    /**
     * @brief 日志器管理器为单例
     * */
    typedef liucxi::Singleton<LoggerManager> LoggerMgr;
}
#endif //LUWU_LOG_H
