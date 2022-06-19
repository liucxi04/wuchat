#include "luwu/log.h"
#include "luwu/config.h"
#include "luwu/ready.h"
#include "luwu/env.h"
#include <iostream>

liucxi::ConfigVar<int>::ptr g_int =
        liucxi::Config::lookup("global.int", (int) 8080, "global int");

liucxi::ConfigVar<float>::ptr g_float =
        liucxi::Config::lookup("global.float", (float) 10.2f, "global float");

// 字符串需显示构造，不能传字符串常量
liucxi::ConfigVar<std::string>::ptr g_string =
        liucxi::Config::lookup("global.string", std::string("helloworld"), "global string");

liucxi::ConfigVar<std::vector<int>>::ptr g_int_vec =
        liucxi::Config::lookup("global.int_vec", std::vector<int>{1, 2, 3}, "global int vec");

liucxi::ConfigVar<std::set<int>>::ptr g_int_set =
        liucxi::Config::lookup("global.int_set", std::set<int>{1, 2, 3}, "global int set");

liucxi::ConfigVar<std::map<std::string, int>>::ptr g_map_string2int =
        liucxi::Config::lookup("global.map_string2int", std::map<std::string, int>
                {{"key1", 1},
                 {"key2", 2}}, "global map string2int");

////////////////////////////////////////////////////////////
// 自定义配置
class Person {
public:
    Person() = default;

    std::string m_name;
    int m_age = 0;
    bool m_sex = false;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

// 实现自定义配置的YAML序列化与反序列化，这部分要放在liucxi命名空间中
namespace liucxi {

    template<>
    class LexicalCast<std::string, Person> {
    public:
        Person operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template<>
    class LexicalCast<Person, std::string> {
    public:
        std::string operator()(const Person &p) {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

} // end namespace liucxi

liucxi::ConfigVar<Person>::ptr g_person =
        liucxi::Config::lookup("class.person", Person(), "system person");

liucxi::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
        liucxi::Config::lookup("class.map", std::map<std::string, Person>(), "system person map");

liucxi::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map =
        liucxi::Config::lookup("class.vec_map", std::map<std::string, std::vector<Person>>(),
                               "system vec map");

void test_class() {
    static uint64_t id = 14;

    if (!g_person->getListener(id)) {
        g_person->addListener([](const Person &old_value, const Person &new_value) {
            std::cout << "g_person value change, old value:" << old_value.toString()
                      << ", new value:" << new_value.toString() << std::endl;
        });
    }

    std::cout << g_person->getValue().toString() << std::endl;

    for (const auto &i: g_person_map->getValue()) {
        std::cout << i.first << ":" << i.second.toString() << std::endl;
    }

    for (const auto &i: g_person_vec_map->getValue()) {
        std::cout << i.first;
        for (const auto &j: i.second) {
            std::cout << j.toString() << std::endl;
        }
    }
}

////////////////////////////////////////////////////////////

template<class T>
std::string formatArray(const T &v) {
    std::stringstream ss;
    ss << "[";
    for (const auto &i: v) {
        ss << " " << i;
    }
    ss << " ]";
    return ss.str();
}

template<class T>
std::string formatMap(const T &m) {
    std::stringstream ss;
    ss << "{";
    for (const auto &i: m) {
        ss << " {" << i.first << ":" << i.second << "}";
    }
    ss << " }";
    return ss.str();
}

void test_config() {
    std::cout << "g_int value: " << g_int->getValue() << std::endl;
    std::cout << "g_float value: " << g_float->getValue() << std::endl;
    std::cout << "g_string value: " << g_string->getValue() << std::endl;
    std::cout << "g_int_vec value: " << formatArray<std::vector<int>>(g_int_vec->getValue()) << std::endl;
    std::cout << "g_int_set value: " << formatArray<std::set<int>>(g_int_set->getValue()) << std::endl;
    std::cout << "g_int_map value: " << formatMap<std::map<std::string, int>>(g_map_string2int->getValue())
              << std::endl;
    // 自定义配置项
    test_class();
}

//int main(int argc, char *argv[]) {
//    // 设置g_int的配置变更回调函数
//    g_int->addListener([](const int &old_value, const int &new_value) {
//        std::cout << "g_int value changed, old_value: " << old_value << ", new_value: " << new_value << std::endl;
//    });
//
//    std::cout << "before============================" << std::endl;
//    test_config();
//
//    YAML::Node node = YAML::LoadFile("../conf/test_config.yml");
//    liucxi::Config::loadFromYaml(node);
//
//    std::cout << "after============================" << std::endl;
//    test_config();
//
//    return 0;
//}
int main(int argc, char *argv[]) {

//    std::cout << "before============================" << std::endl;
//    std::cout << liucxi::g_logDefines->toString() << std::endl;
//
//    YAML::Node node = YAML::LoadFile("../conf/log.yml");
//    liucxi::Config::loadFromYaml(node);
//
//    std::cout << "after============================" << std::endl;
//    std::cout << liucxi::g_logDefines->toString() << std::endl;

    // 从配置文件中加载配置，由于更新了配置，会触发配置项的配置变更回调函数
    liucxi::EnvMgr::getInstance()->init(argc, argv);
    std::cout << liucxi::EnvMgr::getInstance()->getAbsolutePath("conf") << std::endl;
    liucxi::Config::LoadFromConfDir("conf");
    liucxi::Config::visit([](const liucxi::ConfigVarBase::ptr &var) {
        std::cout << "name=" << var->getName()
                  << " description=" << var->getDescribe()
                  << " value=" << var->toString()
                  << std::endl;
    });
    return 0;
}