//
// Created by liucxi on 2022/6/11.
//

#ifndef LUWU_HTTP_SERVER_H
#define LUWU_HTTP_SERVER_H

#include <utility>

#include "http_session.h"
#include "luwu/tcp_server.h"
#include "servlet.h"

namespace liucxi {
    namespace http {
        class HttpServer : public TCPServer {
        public:
            typedef std::shared_ptr<HttpServer> ptr;

            explicit HttpServer(bool keepalive = false,
                                liucxi::IOManager *worker = liucxi::IOManager::GetThis(),
                                liucxi::IOManager *accept = liucxi::IOManager::GetThis());

            ServletDispatch::ptr getDispatch() const { return m_dispatch; }

            void setDispatch(ServletDispatch::ptr dispatch) { m_dispatch = std::move(dispatch); }

            void setName(std::string name) override;

        protected:
            void handleClient(Socket::ptr sock) override;

        private:
            bool m_keepalive;
            ServletDispatch::ptr m_dispatch;
        };
    }
}



#endif //LUWU_HTTP_SERVER_H
