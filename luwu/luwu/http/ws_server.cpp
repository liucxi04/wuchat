#include "ws_server.h"
#include "luwu/macro.h"

namespace liucxi {
    namespace http {

        static Logger::ptr g_logger = LUWU_LOG_NAME("system");

        WSServer::WSServer(IOManager *worker, IOManager *accept_worker)
                : TCPServer(worker, accept_worker) {
            m_dispatch.reset(new WSServletDispatch);
        }

        void WSServer::handleClient(Socket::ptr client) {
            LUWU_LOG_DEBUG(g_logger) << "handleClient " << *client;
            WSSession::ptr session(new WSSession(client));
            do {
                HttpRequest::ptr header = session->handleShake();
                if (!header) {
                    LUWU_LOG_DEBUG(g_logger) << "handleShake error";
                    break;
                }
                WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
                if (!servlet) {
                    LUWU_LOG_DEBUG(g_logger) << "no match WSServlet";
                    break;
                }
                int rt = servlet->onConnect(header, session);
                if (rt) {
                    LUWU_LOG_DEBUG(g_logger) << "onConnect return " << rt;
                    break;
                }
                while (true) {
                    auto msg = session->recvMessage();
                    if (!msg) {
                        break;
                    }
                    rt = servlet->handle(header, msg, session);
                    if (rt) {
                        LUWU_LOG_DEBUG(g_logger) << "handle return " << rt;
                        break;
                    }
                }
                servlet->onClose(header, session);
            } while (0);
            session->close();
        }

    }
}
