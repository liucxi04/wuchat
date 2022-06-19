//
// Created by liucxi on 2022/4/9.
//

#include "config.h"
#include "ready.h"
#include "env.h"
#include "ext.h"
#include "utils.h"
#include <sys/stat.h>

namespace liucxi {

    // Config::ConfigVarMap Config::s_data;

    ConfigVarBase::ptr Config::lookupBase(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = getData().find(name);
        return it == getData().end() ? nullptr : it->second;
    }

    /**
     * @brief 递归解析 Node 配置
     */
    static void listAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output) {
        if (prefix.find_first_not_of("qwertyuiopasdfghjklzxcvbnm._0123456789") != std::string::npos) {
            LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "config invalid name: " << prefix << " : " << node;
            return;
        }
        output.emplace_back(prefix, node);
        if (node.IsMap()) {
            for (const auto &it : node) {
                listAllMember(prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar(),
                              it.second, output);
            }
        } else if (node.IsSequence()) {
            for (const auto &i : node) {
                if (i.IsMap()) {
                    for (const auto &it : i) {
                        listAllMember(prefix.empty() ? it.first.Scalar() : prefix + "." + it.first.Scalar(),
                                      it.second, output);
                    }
                }
            }
        }
    }

    /**
     * @brief 从 YAML::Node 载入所有配置项
     */
    void Config::loadFromYaml(const YAML::Node& root) {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        listAllMember("", root, all_nodes);


        for (const auto &i : all_nodes) {
            std::string key = i.first;
            if (key.empty()) {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = lookupBase(key);

            if (var) {
                if (i.second.IsScalar()) {
                    var->fromString(i.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    /// 记录每个文件的修改时间
    static std::map<std::string, uint64_t> s_file2modifyTime;
    /// 是否强制加载配置文件，非强制加载的情况下，如果记录的文件修改时间未变化，则跳过该文件的加载
    static Mutex s_mutex;

    void Config::LoadFromConfDir(const std::string &path, bool force) {
        std::string absolute_path = EnvMgr::getInstance()->getAbsolutePath(path);
        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absolute_path, ".yml");

        for (auto &i : files) {
            {
                struct stat st{};
                lstat(i.c_str(), &st);
                Mutex::Lock lock(s_mutex);
                if (!force && s_file2modifyTime[i] == (uint64_t)st.st_mtime) {
                    continue;
                }
                s_file2modifyTime[i] = st.st_mtime;
            }
            try {
                YAML::Node root = YAML::LoadFile(i);
                loadFromYaml(root);
            } catch (...) {
            }
        }
    }

    /**
     * @brief 遍历所有配置项，如何操作每一项由用户定义的回调函数决定
     */
    void Config::visit(const std::function<void(ConfigVarBase::ptr)>& callback) {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = getData();
        for (const auto &i : m) {
            callback(i.second);
        }
    }
}
