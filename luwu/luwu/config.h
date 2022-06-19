//
// Created by liucxi on 2022/4/9.
//

#ifndef LUWU_CONFIG_H
#define LUWU_CONFIG_H

#include <map>
#include <set>
#include <list>
#include <string>
#include <memory>
#include <sstream>
#include <vector>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <boost/lexical_cast.hpp>

//#include "log.h"
//#include "mutex.h"
#include "macro.h"

namespace liucxi {

    /**
     * @brief 配置类基类，其核心功能需要子类实现
     * */
    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        explicit ConfigVarBase(std::string name, std::string describe = "")
                : m_name(std::move(name)), m_describe(std::move(describe)) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() = default;

        const std::string &getName() const { return m_name; }

        const std::string &getDescribe() const { return m_describe; }

        virtual std::string toString() = 0;

        virtual bool fromString(const std::string &val) = 0;

        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;         /// 配置名称
        std::string m_describe;     /// 配置描述
    };

    /**
     * @brief 类型转换
     * */
    template<typename From, typename To>
    class LexicalCast {
    public:
        To operator()(const From &v) {
            return boost::lexical_cast<To>(v);
        }
    };

    /**
     * 模板偏特化，string to vector<T>
     * */
    template<typename To>
    class LexicalCast<std::string, std::vector<To>> {
    public:
        std::vector<To> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::vector<To> vec;
            std::stringstream ss;
            for (auto i: node) {
                ss.str("");
                ss << i;
                vec.push_back(LexicalCast<std::string, To>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 模板偏特化，vector<T> to string
     * */
    template<typename From>
    class LexicalCast<std::vector<From>, std::string> {
    public:
        std::string operator()(const std::vector<From> &v) {
            YAML::Node node;
            std::stringstream ss;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<From, std::string>()(i)));
            }
            ss << node;
            return ss.str();
        }
    };

    /**
     * 模板偏特化，string to set<T>
     * */
    template<typename To>
    class LexicalCast<std::string, std::set<To>> {
    public:
        std::set<To> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::set<To> vec;
            std::stringstream ss;
            for (auto i: node) {
                ss.str("");
                ss << i;
                vec.insert(LexicalCast<std::string, To>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 模板偏特化，set<T> to string
     * */
    template<typename From>
    class LexicalCast<std::set<From>, std::string> {
    public:
        std::string operator()(const std::set<From> &v) {
            YAML::Node node;
            std::stringstream ss;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<From, std::string>()(i)));
            }
            ss << node;
            return ss.str();
        }
    };

    /**
     * 模板偏特化，string to map<string, T>
     * */
    template<typename To>
    class LexicalCast<std::string, std::map<std::string, To>> {
    public:
        std::map<std::string, To> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, To> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it) {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, To>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * 模板偏特化，map<string, T> to string
     * */
    template<typename From>
    class LexicalCast<std::map<std::string, From>, std::string> {
    public:
        std::string operator()(const std::map<std::string, From> &v) {
            YAML::Node node;
            std::stringstream ss;
            for (auto &i: v) {
                node[i.first] = YAML::Load(LexicalCast<From, std::string>()(i.second));
            }
            ss << node;
            return ss.str();
        }
    };


    /**
     * @brief 配置基类的实现类
     * */
    template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar<T>> ptr;
        typedef RWMutex RWMutexType;
        /**
         * @brief 配置对象的监听函数，当值发生改变时调用
         * */
        typedef std::function<void( const T &old_value, const T &new_value)>  on_change;

        ConfigVar(const std::string &name, const T &default_val, const std::string &describe)
                : ConfigVarBase(name, describe), m_val(default_val) {
        }

        /**
         * @brief 将 T 类型的值转为字符串
         * */
        std::string toString() override {
            try {
                return ToStr()(getValue());
            } catch (std::exception &e) {
                LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "ConfigVar::toString exception " << e.what()
                                                << " convert: " << getTypeName() << "to string";
            }
            return "";
        }

        /**
         * @brief 将字符串转为 T 类型的值
         * */
        bool fromString(const std::string &val) override {
            try {
                setValue(FromStr()(val));
            } catch (std::exception &e) {
                LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what()
                                                << " convert: string to " << getTypeName();
            }
            return false;
        }

        /**
         * @brief 获取该配置项的类型
         */
        std::string getTypeName() const override { return typeid(T).name(); }

        T getValue() {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }

        void setValue(const T &val) {
            {
                // 先尝试读，看新值和旧值是否相同
                RWMutexType::ReadLock lock(m_mutex);
                if (val == m_val) {
                    return;
                }
                /// 执行回调参数，回调函数的参数为 旧值 m_val、新值 val
                for (auto &i : m_callbacks) {
                    i.second(m_val, val);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            m_val = val;
        }

        uint64_t addListener(on_change callback) {
            static uint64_t s_callbackId = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_callbackId;
            m_callbacks[s_callbackId] = callback;
            return s_callbackId;
        }

        void delListener(uint64_t key) {
            RWMutexType::WriteLock lock(m_mutex);
            m_callbacks.erase(key);
        }

        on_change getListener(uint64_t key) {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_callbacks.find(key);
            return it == m_callbacks.end() ? nullptr : it->second;
        }

        void clearListener() {
            RWMutexType::WriteLock lock(m_mutex);
            m_callbacks.clear();
        }

    private:
        T m_val;
        RWMutexType m_mutex;
        /**
         * @brief 变更回调函数
         * @param key map 的 key 要求是 uint_64_t 类型
         * */
        std::map<uint64_t, on_change> m_callbacks;
    };

    /**
     * @brief 配置管理类，单例模式
     * */
    class Config {
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;

        /**
         * @brief 初始化一个配置项
         * */
        template<typename T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name, const T &default_val,
                                                 const std::string &description = "") {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = getData().find(name);
            // 该配置项已经存在
            if (it != getData().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    // 配置项已经存在，不需要重复初始化
                    return tmp;
                } else {
                    // 配置项已经存在，但和当前配置的类型冲突
                    LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "lookup name = " << name << " exists but type not "
                                                    << typeid(T).name() << ", real type = " << it->second->getTypeName()
                                                    << " " << it->second->toString();
                    return nullptr;
                }
            }
            // 该配置项不存在
            if (name.find_first_not_of("qwertyuiopasdfghjklzxcvbnm._0123456789") != std::string::npos) {
                LUWU_LOG_ERROR(LUWU_LOG_ROOT()) << "lookup name invalid" << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_val, description));
            getData()[name] = v;
            return v;
        }

        /**
         * @brief 按照配置名查找配置项
         * */
        template<typename T>
        static typename ConfigVar<T>::ptr lookup(const std::string &name) {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = getData().find(name);
            if (it == getData().end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        /**
         * @brief 按照配置名查找配置项基类
         */
        static ConfigVarBase::ptr lookupBase(const std::string &name);

        static void loadFromYaml(const YAML::Node &root);

        static void LoadFromConfDir(const std::string &path, bool force = false);

        static void visit(const std::function<void(ConfigVarBase::ptr)>& callback);

    private:
        /**
         * @note 在此处定义静态 map，在 cpp 文件实现会出错，所以使用了下面这种方式，具体为什么可以等待实验
         * */
        // static ConfigVarMap s_data;

        /**
         * @brief 获取数据成员
         */
        static ConfigVarMap &getData() {
            static ConfigVarMap s_data;
            return s_data;
        }

        /**
         * @brief 获取数据成员的 mutex
         */
        static RWMutexType &GetMutex() {
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };

}
#endif //LUWU_CONFIG_H
