//
// Created by liucxi on 2022/6/11.
//
#include "servlet.h"

#include <utility>
#include <fnmatch.h>

namespace liucxi {
    namespace http {

        FunctionServlet::FunctionServlet(callback cb)
                : Servlet("FunctionServlet"), m_cb(std::move(cb)) {
        }

        int32_t FunctionServlet::handle(HttpRequest::ptr request,
                                        HttpResponse::ptr response, HttpSession::ptr session) {
            return m_cb(request, response, session);
        }

        ServletDispatch::ServletDispatch()
                : Servlet("ServletDispatch"), m_default(new ServletNotFound("luwu/1.0.0")) {
        }

        int32_t ServletDispatch::handle(HttpRequest::ptr request,
                                        HttpResponse::ptr response, HttpSession::ptr session) {
            auto servlet = getMatchedServlet(request->getPath());
            if (servlet) {
                return servlet->handle(request, response, session);
            }
            return -1;
        }

        Servlet::ptr ServletDispatch::getServlet(const std::string &uri) {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_data.find(uri);
            return it == m_data.end() ? nullptr : it->second;
        }

        Servlet::ptr ServletDispatch::getGlobServlet(const std::string &uri) {
            RWMutexType::ReadLock lock(m_mutex);
            for (const auto &glob: m_glob) {
                if (glob.first == uri) {
                    return glob.second;
                }
            }
            return nullptr;
        }

        Servlet::ptr ServletDispatch::getMatchedServlet(const std::string &uri) {
            RWMutexType::ReadLock lock(m_mutex);
            auto it1 = m_data.find(uri);
            if (it1 != m_data.end()) {
                return it1->second;
            }
            for (const auto &glob: m_glob) {
                if (!fnmatch(glob.first.c_str(), uri.c_str(), 0)) {
                    return glob.second;
                }
            }
            return m_default;
        }

        void ServletDispatch::addServlet(const std::string &uri, Servlet::ptr servlet) {
            RWMutexType::WriteLock lock(m_mutex);
            m_data[uri] = std::move(servlet);
        }

        void ServletDispatch::addServlet(const std::string &uri, FunctionServlet::callback servlet) {
            RWMutexType::WriteLock lock(m_mutex);
            m_data[uri].reset(new FunctionServlet(std::move(servlet)));
        }

        void ServletDispatch::addGlobServlet(const std::string &uri, const Servlet::ptr &servlet) {
            RWMutexType::WriteLock lock(m_mutex);
            for (auto it = m_glob.begin(); it != m_glob.end(); ++it) {
                if (it->first == uri) {
                    m_glob.erase(it);
                    break;
                }
            }
            m_glob.emplace_back(uri, servlet);
        }

        void ServletDispatch::addGlobServlet(const std::string &uri, FunctionServlet::callback servlet) {
            addGlobServlet(uri, std::make_shared<FunctionServlet>(servlet));
        }

        void ServletDispatch::delServlet(const std::string &uri) {
            RWMutexType::WriteLock lock(m_mutex);
            m_data.erase(uri);
        }

        void ServletDispatch::delGlobServlet(const std::string &uri) {
            RWMutexType::WriteLock lock(m_mutex);
            for (auto it = m_glob.begin(); it != m_glob.end(); ++it) {
                if (it->first == uri) {
                    m_glob.erase(it);
                    break;
                }
            }
        }

        ServletNotFound::ServletNotFound(std::string page)
                : Servlet("ServletNotFound"), m_page(std::move(page)) {
            m_content = "<html>"
                        "<head><title>404 Not Found</title></head>"
                        "<body><center><h1>404 Not Found</h1></center><hr><center>" + m_page + "</center></body>"
                                                                                               "</html>";
        }

        int32_t ServletNotFound::handle(HttpRequest::ptr request,
                                        HttpResponse::ptr response, HttpSession::ptr session) {
            response->setStatus(HttpStatus::NOT_FOUND);
            response->setHeader("Server", "luwu/1.0.0");
            response->setHeader("Content-Type", "text/html");
            response->setBody(m_content);
            return 0;
        }

    }
}
