//
// Created by liucxi on 2022/6/18.
//

#include <json/value.h>
#include "protocol.h"
#include "luwu/luwu/util/json_util.h"

namespace chat {

    ChatMessage::ptr ChatMessage::Creat(const std::string &v) {
        Json::Value json;
        if (!liucxi::JsonUtil::FromString(json, v)) {
            return nullptr;
        }
        ChatMessage::ptr rt(new ChatMessage);
        auto names = json.getMemberNames();
        for (auto &i : names) {
            rt->m_datas[i] = json[i].asString();
        }
        return rt;
    }

    std::string ChatMessage::get(const std::string &name) {
        auto it = m_datas.find(name);
        return it == m_datas.end() ? "" : it->second;
    }

    void ChatMessage::set(const std::string &name, const std::string &v) {
        m_datas[name] = v;
    }

    std::string ChatMessage::toString() const {
        Json::Value json;
        for (auto & i : m_datas) {
            json[i.first] = i.second;
        }
        return liucxi::JsonUtil::ToString(json);
    }
}