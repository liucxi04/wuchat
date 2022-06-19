#ifndef LUWU_WS_SERVER_H
#define LUWU_WS_SERVER_H

#include "luwu/tcp_server.h"
#include "ws_session.h"
#include "ws_servlet.h"

namespace liucxi {
    namespace http {

        class WSServer : public TCPServer {
        public:
            typedef std::shared_ptr<WSServer> ptr;

            WSServer(IOManager *worker = IOManager::GetThis(),
                     IOManager *accept_worker = IOManager::GetThis());

            WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch; }

            void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v; }

        protected:
            virtual void handleClient(Socket::ptr client) override;

        protected:
            WSServletDispatch::ptr m_dispatch;
        };

    }
}

#endif
