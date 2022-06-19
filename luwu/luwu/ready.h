//
// Created by liucxi on 2022/5/19.
//

#ifndef LUWU_READY_H
#define LUWU_READY_H

#include "log.h"
//#include "config.h"

#include <iostream>

namespace liucxi {
    /// 从配置文件中加载日志配置

    /**
     * @brief 日志输出器配置结构体
     * */
    struct LogAppenderDefine {
        int type = 0; // 1 File, 2 Stdout
        std::string pattern;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const {
            return type == oth.type && pattern == oth.pattern && file == oth.file;
        }
    };

    /**
     * @brief 日志器配置结构体
     * */
    struct LoggerDefine {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOWN;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LoggerDefine &oth) const {
            return name == oth.name && level == oth.level && appenders == oth.appenders;
        }

        bool operator<(const LoggerDefine &oth) const {
            return name < oth.name;
        }

        bool isValid() const {
            return !name.empty();
        }
    };

    /**
     * @brief 实现自定义的 YAML 序列化与反序列化
     */
    template<>
    class LexicalCast<std::string, LoggerDefine> {
    public:
        LoggerDefine operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            LoggerDefine loggerDefine;

            if (!node["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << node << std::endl;
                throw std::logic_error("log config name is null");
            }
            loggerDefine.name = node["name"].as<std::string>();
            loggerDefine.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");

            if (node["appenders"].IsDefined()) {
                for (const auto &appender : node["appenders"]) {
                    if (!appender["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, " << appender << std::endl;
                        continue;
                    }
                    std::string type = appender["type"].as<std::string>();
                    LogAppenderDefine logAppenderDefine;
                    if (type == "FileLogAppender") {
                        logAppenderDefine.type = 1;
                        if (!appender["file"].IsDefined()) {
                            std::cout << "log config error: appender file is null, " << appender << std::endl;
                        }
                        logAppenderDefine.file = appender["file"].as<std::string>();
                        if (appender["pattern"].IsDefined()) {
                            logAppenderDefine.pattern = appender["pattern"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender") {
                        logAppenderDefine.type = 2;
                        if (appender["pattern"].IsDefined()) {
                            logAppenderDefine.pattern = appender["pattern"].as<std::string>();
                        }
                    } else {
                        std::cout << "log config error: appender type is invalid, " << appender << std::endl;
                    }
                    loggerDefine.appenders.push_back(logAppenderDefine);
                }
            }
            return loggerDefine;
        }
    };

    /**
     * @brief 实现自定义的 YAML 序列化与反序列化
     */
    template<>
    class LexicalCast<LoggerDefine, std::string> {
    public:
        std::string operator()(const LoggerDefine &loggerDefine) {
            YAML::Node node;
            node["name"] = loggerDefine.name;
            node["level"] = LogLevel::ToString(loggerDefine.level);
            for (const auto &appender : loggerDefine.appenders) {
                YAML::Node appenderNode;
                if (appender.type == 1) {
                    appenderNode["type"] = "FileLogAppender";
                    appenderNode["file"] = appender.file;
                } else if (appender.type == 2) {
                    appenderNode["type"] = "StdoutLogAppender";
                }
                if (!appender.pattern.empty()) {
                    appenderNode["pattern"] = appender.pattern;
                }
                node["appenders"].push_back(appenderNode);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    liucxi::ConfigVar<std::set<LoggerDefine>>::ptr g_logDefines =
            liucxi::Config::lookup("logs", std::set<LoggerDefine>(), "logs config");

    struct LogInit {
        LogInit() {
            g_logDefines->addListener([](const std::set<LoggerDefine> &oldVal, const std::set<LoggerDefine> &newVal){
                LUWU_LOG_INFO(LUWU_LOG_ROOT()) << "log config changed";
                for (const auto & val : newVal) {
                    auto it = oldVal.find(val);
                    liucxi::Logger::ptr logger;
                    if (it == oldVal.end()) {
                        // 新增
                        logger = LUWU_LOG_NAME(val.name);
                    } else {
                        if (!(val == *it)) {
                            // 修改，对比一下，完全相等自然就不需要该了
                            logger = LUWU_LOG_NAME(val.name);
                        } else {
                            continue;
                        }
                    }
                    logger->setLevel(val.level);
                    logger->clearAppenders();
                    for (const auto &appender : val.appenders) {
                        liucxi::LogAppender::ptr logAppender;
                        if (appender.type == 1) {
                            logAppender.reset(new liucxi::FileLogAppender(appender.file));
                        } else if (appender.type == 2) {
                            logAppender.reset(new liucxi::StdoutLogAppender);
                        }
                        if (!appender.pattern.empty()) {
                            logAppender->setFormatter(std::make_shared<LogFormatter>(appender.pattern));
                        } else {
                            logAppender->setFormatter(std::make_shared<LogFormatter>());
                        }
                        logger->addAppender(logAppender);
                    }
                }

                for (const auto &val : oldVal) {
                    auto it = newVal.find(val);
                    if (it == newVal.end()) {
                        auto logger = LUWU_LOG_NAME(val.name);

                        logger->setLevel(liucxi::LogLevel::UNKNOWN);
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

    static LogInit __log_init;
}
#endif //LUWU_READY_H
