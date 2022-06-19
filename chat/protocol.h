//
// Created by liucxi on 2022/6/18.
//

#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <string>
#include <map>
#include <memory>

namespace chat {

    class ChatMessage {
    public:
        typedef std::shared_ptr<ChatMessage> ptr;

        static ChatMessage::ptr Creat(const std::string &v);

        std::string get(const std::string &name);

        void set(const std::string &name, const std::string &v);

        std::string toString() const;
    private:
        std::map<std::string, std::string> m_datas;
    };
}
#endif //CHAT_PROTOCOL_H
