//
// Created by liucxi on 2022/6/11.
//

#ifndef LUWU_URI_H
#define LUWU_URI_H

#include <memory>
#include "address.h"

namespace liucxi {

    /**
     * @brief URI 封装类
     */

    /*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
    */
    class URI {
    public:
        typedef std::shared_ptr<URI> ptr;

        static URI::ptr Create(const std::string &url);

        const std::string &getScheme() const { return m_scheme; }

        const std::string &getUserinfo() const { return m_userinfo; }

        const std::string &getHost() const { return m_host; }

        const std::string &getPath() const;

        const std::string &getQuery() const { return m_query; }

        const std::string &getFragment() const { return m_fragment; }

        uint16_t getPort() const;

        void setScheme(const std::string &v) { m_scheme = v; }

        void setUserinfo(const std::string &v) { m_userinfo = v; }

        void setHost(const std::string &v) { m_host = v; }

        void setPath(const std::string &v) { m_path = v; }

        void setQuery(const std::string &v) { m_query = v; }

        void setFragment(const std::string &v) { m_fragment = v; }

        void setPort(uint16_t v) { m_port = v; }

        std::string toString() const;

        Address::ptr createAddress() const;

    private:
        bool isDefaultPort() const;

    private:
        std::string m_scheme;
        std::string m_userinfo;
        std::string m_host;
        std::string m_path;
        std::string m_query;
        std::string m_fragment;
        uint16_t m_port = 0;
    };
}
#endif //LUWU_URI_H
