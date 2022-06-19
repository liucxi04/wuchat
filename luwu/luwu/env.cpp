//
// Created by liucxi on 2022/6/16.
//
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include "env.h"
#include "config.h"
#include "macro.h"


namespace liucxi {

    static Logger::ptr g_logger = LUWU_LOG_NAME("system");

    bool liucxi::Env::init(int argc, char **argv) {
        char link[1024] = {0};
        char path[1024] = {0};
        sprintf(link, "/proc/%d/exe", getpid());
        readlink(link, path, sizeof(path));
        m_exe = path;

        auto pos = m_exe.find_last_of('/');
        m_cwd = m_exe.substr(0, pos) + "/";

        m_program = argv[0];

        const char *now_key = nullptr;
        for (int i = 1; i < argc; ++i) {
            if (argv[i][0] == '-') {
                if (strlen(argv[i]) > 1) {
                    if (now_key) {
                        add(now_key, "");
                    }
                    now_key = argv[i] + 1;
                } else {
                    LUWU_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                             << " val=" << argv[i];
                    return false;
                }
            } else {
                if (now_key) {
                    add(now_key, argv[i]);
                    now_key = nullptr;
                } else {
                    LUWU_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                             << " val=" << argv[i];
                    return false;
                }
            }
        }
        if (now_key) {
            add(now_key, "");
        }
        return true;
    }

    void liucxi::Env::add(const std::string &key, const std::string &val) {
        RWMutexType::WriteLock lock(m_mutex);
        m_args[key] = val;
    }

    bool liucxi::Env::has(const std::string &key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end();
    }

    void liucxi::Env::del(const std::string &key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_args.erase(key);
    }

    std::string liucxi::Env::get(const std::string &key, const std::string &default_val) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_args.find(key);
        return it != m_args.end() ? it->second : default_val;
    }

    void liucxi::Env::addHelp(const std::string &key, const std::string &desc) {
        delHelp(key);
        RWMutexType::WriteLock lock(m_mutex);
        m_helps.emplace_back(key, desc);
    }

    void liucxi::Env::delHelp(const std::string &key) {
        RWMutexType::WriteLock lock(m_mutex);
        for (auto it = m_helps.begin(); it != m_helps.end(); ++it) {
            if (it->first == key) {
                m_helps.erase(it);
                break;
            }
        }
    }

    void liucxi::Env::printHelp() {
        RWMutexType::ReadLock lock(m_mutex);
        std::cout << "Usage: " << m_program << " [options]" << std::endl;
        for (auto &i: m_helps) {
            std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
        }
    }

    bool liucxi::Env::setEnv(const std::string &key, const std::string &val) {
        return !setenv(key.c_str(), val.c_str(), 1);
    }

    std::string liucxi::Env::getEnv(const std::string &key, const std::string &default_val) {
        const char *v = getenv(key.c_str());
        if (v == nullptr) {
            return default_val;
        }
        return v;
    }

    std::string liucxi::Env::getAbsolutePath(const std::string &path) const {
        if (path.empty()) {
            return "/";
        }
        if (path[0] == '/') {
            return path;
        }
        return m_cwd + path;
    }

    std::string liucxi::Env::getAbsoluteWorkPath(const std::string &path) const {
        if(path.empty()) {
            return "/";
        }
        if(path[0] == '/') {
            return path;
        }
        static ConfigVar<std::string>::ptr g_server_work_path = Config::lookup<std::string>("server.work_path");
        return g_server_work_path->getValue() + "/" + path;
    }

    std::string liucxi::Env::getConfigPath() {
        return getAbsolutePath(get("c", "conf"));
    }
}

