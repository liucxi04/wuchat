//
// Created by liucxi on 2022/6/16.
//

#ifndef LUWU_ENV_H
#define LUWU_ENV_H

#include "utils.h"
#include "mutex.h"
#include <map>
#include <vector>

namespace liucxi {
    class Env {
    public:
        typedef liucxi::RWMutex RWMutexType;

        bool init(int argc, char **argv);

        void add(const std::string &key, const std::string &val);

        bool has(const std::string &key);

        void del(const std::string &key);

        std::string get(const std::string &key, const std::string &default_val = "");

        void addHelp(const std::string &key, const std::string &desc);

        void delHelp(const std::string &key);

        void printHelp();

        const std::string &getExe() const { return m_exe; }

        const std::string &getCwd() const { return m_cwd; }

        static bool setEnv(const std::string &key, const std::string &val);

        static std::string getEnv(const std::string &key, const std::string &default_val = "");

        std::string getAbsolutePath(const std::string &path) const;

        std::string getAbsoluteWorkPath(const std::string &path) const;

        std::string getConfigPath();

    private:
        RWMutexType m_mutex;
        std::map<std::string, std::string> m_args;                      /// 存储程序的自定义环境变量
        std::vector<std::pair<std::string, std::string>> m_helps;       /// 存储帮助选项与描述

        std::string m_program;      /// 程序名
        std::string m_exe;          /// 程序完整路径
        std::string m_cwd;          /// 当前路径
    };

    typedef Singleton<Env> EnvMgr;
}
#endif //LUWU_ENV_H
