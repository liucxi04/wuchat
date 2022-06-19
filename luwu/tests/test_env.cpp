#include "luwu/env.h"
#include "luwu/config.h"
#include <iostream>

liucxi::Env *g_env = liucxi::EnvMgr::getInstance();

int main(int argc, char *argv[]) {

    YAML::Node node = YAML::LoadFile("../conf/log.yml");
    liucxi::Config::loadFromYaml(node);

    g_env->addHelp("h", "print this help message");

    if (!g_env->init(argc, argv)) {
        g_env->printHelp();
        return -1;
    }
    if (g_env->has("h")) {
        g_env->addHelp("q", "qqq");
        g_env->printHelp();
    }

    std::cout << "config  -- : " << g_env->get("t") << std::endl;

    std::cout << "exe: " << g_env->getExe() << std::endl;
    std::cout << "cwd: " << g_env->getCwd() << std::endl;
    std::cout << "absolute path of test: " << g_env->getAbsolutePath("test") << std::endl;
    std::cout << "conf path:" << g_env->getConfigPath() << std::endl;

//    g_env->add("key1", "value1");
//    std::cout  << "key1: " << g_env->get("key1") << std::endl;
//
//    g_env->setEnv("key2", "value2");
//    std::cout  << "key2: " << g_env->getEnv("key2") << std::endl;
//
//    std::cout  << g_env->getEnv("PATH") << std::endl;

    return 0;
}