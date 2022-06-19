//
// Created by liucxi on 2022/6/11.
//

#ifndef LUWU_SERVLET_H
#define LUWU_SERVLET_H

#include <memory>
#include <functional>
#include <string>
#include <utility>
#include <unordered_map>

#include "http.h"
#include "http_session.h"
#include "luwu/mutex.h"

namespace liucxi {
    namespace http {

        class Servlet {
        public:
            typedef std::shared_ptr<Servlet> ptr;

            explicit Servlet(std::string name) : m_name(std::move(name)) {}

            virtual ~Servlet() = default;

            virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) = 0;

            const std::string &getName() const { return m_name; }

        protected:
            std::string m_name;
        };

        class FunctionServlet : public Servlet {
        public:
            typedef std::shared_ptr<FunctionServlet> ptr;
            typedef std::function<int32_t(HttpRequest::ptr request,
                                          HttpResponse::ptr response, HttpSession::ptr session)> callback;

            explicit FunctionServlet(callback cb);


            int32_t handle(HttpRequest::ptr request,
                           HttpResponse::ptr response, HttpSession::ptr session) override;

        private:
            callback m_cb;
        };

        class ServletDispatch : public Servlet {
        public:
            typedef std::shared_ptr<ServletDispatch> ptr;

            typedef RWMutex RWMutexType;

            ServletDispatch();

            int32_t handle(HttpRequest::ptr request,
                           HttpResponse::ptr response, HttpSession::ptr session) override;

            Servlet::ptr getServlet(const std::string &uri);

            Servlet::ptr getGlobServlet(const std::string &uri);

            Servlet::ptr getMatchedServlet(const std::string &uri);

            Servlet::ptr getDefault() const { return m_default; }

            void addServlet(const std::string &uri, Servlet::ptr servlet);

            void addServlet(const std::string &uri, FunctionServlet::callback servlet);

            void addGlobServlet(const std::string &uri, const Servlet::ptr& servlet);

            void setDefault(Servlet::ptr servlet) { m_default = std::move(servlet); }

            void addGlobServlet(const std::string &uri, FunctionServlet::callback servlet);

            void delServlet(const std::string &uri);

            void delGlobServlet(const std::string &uri);

        private:
            RWMutexType m_mutex;
            std::unordered_map<std::string, Servlet::ptr> m_data;
            std::vector<std::pair<std::string, Servlet::ptr>> m_glob;
            Servlet::ptr m_default;
        };

        class ServletNotFound : public Servlet {
        public:
            typedef std::shared_ptr<ServletNotFound> ptr;

            explicit ServletNotFound(std::string page);

            int32_t handle(HttpRequest::ptr request,
                           HttpResponse::ptr response, HttpSession::ptr session) override;
        private:
            std::string m_page;
            std::string m_content;
        };
    }
}
#endif //LUWU_SERVLET_H
