//
// Created by liucxi on 2022/6/18.
//

#ifndef CHAT_SERVLET_H
#define CHAT_SERVLET_H

#include "luwu/luwu/http/ws_servlet.h"
#include "protocol.h"
#include "luwu/luwu/mutex.h"

namespace liucxi {
    namespace http {
        class ChatServlet : public WSServlet {
        public:
            typedef std::shared_ptr<ChatServlet> ptr;
            typedef RWMutex RWMutexType;

            ChatServlet();

            bool sessionExists(const std::string &id);

            void sessionAdd(const std::string &id, WSSession::ptr session);

            void sessionDel(const std::string &id);

            void sessionNotify(chat::ChatMessage::ptr msg, WSSession::ptr session = nullptr);

            int32_t onConnect(HttpRequest::ptr header, WSSession::ptr session) override;

            int32_t onClose(HttpRequest::ptr header, WSSession::ptr session) override;

            int32_t handle(HttpRequest::ptr header, WSFrameMessage::ptr msg, WSSession::ptr session) override;

        private:
            std::map<std::string, WSSession::ptr> m_sessions;
            RWMutexType m_mutex;
        };
    }
}
#endif //CHAT_SERVLET_H
