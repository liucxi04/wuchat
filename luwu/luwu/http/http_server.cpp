//
// Created by liucxi on 2022/6/11.
//
#include "http_server.h"
#include "luwu/macro.h"

namespace liucxi {
    namespace http {
        HttpServer::HttpServer(bool keepalive, liucxi::IOManager *worker, liucxi::IOManager *accept)
                : TCPServer(worker, accept), m_keepalive(keepalive) {
            m_dispatch.reset(new ServletDispatch);
        }

        void HttpServer::setName(std::string name) {
            TCPServer::setName(std::move(name));
            m_dispatch->setDefault(std::make_shared<ServletNotFound>(name));
        }

        void HttpServer::handleClient(Socket::ptr sock) {
            HttpSession::ptr session(new HttpSession(sock));
            do {
                auto req = session->recvRequest();
                if (!req) {
                    LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "recv http request failed, errno=" << errno
                                                            << " errstr=" << strerror(errno)
                                                            << "client:" << *sock;
                    break;
                }
                HttpResponse::ptr rsp(new HttpResponse(req->getVersion(),
                                                               req->isClose() || !m_keepalive));
                rsp->setHeader("Server", getName());
                m_dispatch->handle(req, rsp, session);
                session->sendResponse(rsp);

                if (!m_keepalive || req->isClose()) {
                    break;
                }
            } while (true);
            session->close();
        }
    }
}
