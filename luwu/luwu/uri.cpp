//
// Created by liucxi on 2022/6/11.
//
#include "uri.h"
#include "http/http-parser/http_parser.h"

#include <sstream>

namespace liucxi {
    URI::ptr URI::Create(const std::string &url) {
        URI::ptr uri(new URI);
        struct http_parser_url parser{};

        http_parser_url_init(&parser);

        if (http_parser_parse_url(url.c_str(), url.length(), 0, &parser) != 0) {
            return nullptr;
        }

        if (parser.field_set & (1 << UF_SCHEMA)) {
            uri->setScheme(std::string(url.c_str() + parser.field_data[UF_SCHEMA].off,
                           parser.field_data[UF_SCHEMA].len));
        }

        if (parser.field_set & (1 << UF_USERINFO)) {
            uri->setUserinfo(std::string(url.c_str() + parser.field_data[UF_USERINFO].off,
                             parser.field_data[UF_USERINFO].len));
        }

        if (parser.field_set & (1 << UF_HOST)) {
            uri->setHost(std::string(url.c_str() + parser.field_data[UF_HOST].off,
                         parser.field_data[UF_HOST].len));
        }

        if (parser.field_set & (1 << UF_PORT)) {
            uri->setPort((uint16_t) std::stoi(std::string(url.c_str() + parser.field_data[UF_PORT].off,
                         parser.field_data[UF_PORT].len)));
        } else {
            //默认端口号解析只支持http/ws/https
            if (uri->getScheme() == "http" || uri->getScheme() == "ws") {
                uri->setPort(80);
            } else if (uri->getScheme() == "https" || uri->getScheme() == "wss") {
                uri->setPort(443);
            }
        }

        if (parser.field_set & (1 << UF_PATH)) {
            uri->setPath(std::string(url.c_str() + parser.field_data[UF_PATH].off,
                         parser.field_data[UF_PATH].len));
        }

        if (parser.field_set & (1 << UF_QUERY)) {
            uri->setQuery(std::string(url.c_str() + parser.field_data[UF_QUERY].off,
                          parser.field_data[UF_QUERY].len));
        }

        if (parser.field_set & (1 << UF_FRAGMENT)) {
            uri->setFragment(std::string(url.c_str() + parser.field_data[UF_FRAGMENT].off,
                             parser.field_data[UF_FRAGMENT].len));
        }

        return uri;
    }

    const std::string &URI::getPath() const {
        static std::string s_default_path = "/";
        return m_path.empty() ? s_default_path : m_path;
    }

    uint16_t URI::getPort() const {
        if (m_port) {
            return m_port;
        }
        if (m_scheme == "http" || m_scheme == "ws") {
            return 80;
        } else if (m_scheme == "https" || m_scheme == "wss") {
            return 443;
        }
        return m_port;
    }

    std::string URI::toString() const {
        std::stringstream ss;

        ss << m_scheme
           << "://"
           << m_userinfo
           << (m_userinfo.empty() ? "" : "@")
           << m_host << (isDefaultPort() ? "" : ":" + std::to_string(getPort()))
           << getPath()
           << (m_query.empty() ? "" : "?")
           << m_query
           << (m_fragment.empty() ? "" : "#")
           << m_fragment;

        return ss.str();
    }

    Address::ptr URI::createAddress() const {
        auto addr = Address::LookupAnyIPAddress(m_host);
        if (addr) {
            addr->setPort(getPort());
        }
        return addr;
    }

    bool URI::isDefaultPort() const {
        if (m_scheme == "http" || m_scheme == "ws") {
            return m_port == 80;
        } else if (m_scheme == "https" || m_scheme == "wss") {
            return m_port == 443;
        }
        return false;
    }
}
