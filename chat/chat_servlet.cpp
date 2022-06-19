//
// Created by liucxi on 2022/6/18.
//


#include "chat_servlet.h"
#include "luwu/luwu/macro.h"
#include "luwu/luwu/ext.h"

namespace liucxi {
    namespace http {

        static Logger::ptr g_logger = LUWU_LOG_ROOT();

        ChatServlet::ChatServlet()
            : WSServlet("ChatServlet"){
        }

        int32_t ChatServlet::onConnect(http::HttpRequest::ptr header, http::WSSession::ptr session) {
            LUWU_LOG_INFO(g_logger) << "onConnect";
            return 0;
        }

        int32_t ChatServlet::onClose(http::HttpRequest::ptr header, http::WSSession::ptr session) {
            auto id = header->getHeader("$id");
            if (!id.empty()) {
                sessionDel(id);
                chat::ChatMessage::ptr nty(new chat::ChatMessage);
                nty->set("type", "user_leave");
                nty->set("time", Time2Str());
                nty->set("name", id);
                sessionNotify(nty, session);
            }
            return 0;
        }

        int32_t sendMessage(http::WSSession::ptr session, chat::ChatMessage::ptr rsp) {
            return session->sendMessage(rsp->toString()) > 0 ? 0 : 1;
        }

        int32_t ChatServlet::handle(http::HttpRequest::ptr header,
                                    http::WSFrameMessage::ptr msg,
                                    http::WSSession::ptr session) {

            auto message = chat::ChatMessage::Creat(msg->getData());
            auto id = header->getHeader("$id");
            if (!message) {
                if (!id.empty()) {
                    sessionDel(id);
                }
                return 1;
            }

            chat::ChatMessage::ptr rsp(new chat::ChatMessage);
            auto type = message->get("type");
            if (type == "login_request") {
                rsp->set("type", "login_response");
                auto name = message->get("name");
                if (name.empty()) {
                    rsp->set("result", "400");
                    rsp->set("msg", "name is null");
                    return sendMessage(session, rsp);
                }
                if (!id.empty()) {
                    rsp->set("result", "401");
                    rsp->set("msg", "already login");
                    return sendMessage(session, rsp);
                }
                if (sessionExists(name)) {
                    rsp->set("result", "402");
                    rsp->set("msg", "name exists");
                    return sendMessage(session, rsp);
                }
                id = name;
                header->setHeader("$id", id);
                rsp->set("result", "200");
                rsp->set("msg", "ok");
                sessionAdd(id, session);

                chat::ChatMessage::ptr nty(new chat::ChatMessage);
                nty->set("type", "user_center");
                nty->set("time", Time2Str());
                nty->set("name", name);
                sessionNotify(nty, session);
                // notify
                return sendMessage(session, rsp);
            } else if (type == "send_request") {
                rsp->set("type", "send_response");
                auto context = message->get("msg");
                if (context.empty()) {
                    rsp->set("result", "500");
                    rsp->set("msg", "mag is null");
                    return sendMessage(session, rsp);
                }
                if (id.empty()) {
                    rsp->set("result", "501");
                    rsp->set("msg", "not login");
                    return sendMessage(session, rsp);
                }
                rsp->set("result", "200");
                rsp->set("msg", "ok");

                // notify
                chat::ChatMessage::ptr nty(new chat::ChatMessage);
                nty->set("type", "msg");
                nty->set("time", Time2Str());
                nty->set("name", id);
                nty->set("msg", context);
                sessionNotify(nty);

                return sendMessage(session, rsp);
            }
            return 0;
        }

        bool ChatServlet::sessionExists(const std::string &id) {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_sessions.find(id);
            return it != m_sessions.end();
        }

        void ChatServlet::sessionAdd(const std::string &id, WSSession::ptr session) {
            RWMutexType::WriteLock lock(m_mutex);
            m_sessions[id] = session;
        }

        void ChatServlet::sessionDel(const std::string &id) {
            RWMutexType::WriteLock lock(m_mutex);
            m_sessions.erase(id);
        }

        void ChatServlet::sessionNotify(chat::ChatMessage::ptr msg, WSSession::ptr session) {
            RWMutexType::ReadLock lock(m_mutex);
            auto sessions = m_sessions;
            lock.unlock();

            for (auto &i : sessions) {
                if (i.second == session) {
                    continue;
                }
                sendMessage(i.second, msg);
            }
        }
    }
}