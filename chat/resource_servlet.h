//
// Created by liucxi on 2022/6/18.
//

#ifndef CHAT_RESOURCE_SERVLET_H
#define CHAT_RESOURCE_SERVLET_H

#include "luwu/luwu/http/servlet.h"

namespace liucxi {
    namespace http {

        class ResourceServlet : public Servlet {
        public:
            typedef std::shared_ptr<ResourceServlet> ptr;

            explicit ResourceServlet(std::string path);

            int32_t handle(http::HttpRequest::ptr request,
                           http::HttpResponse::ptr response,
                           http::HttpSession::ptr session) override;

        private:
            std::string m_path;
        };
    }
}
#endif //CHAT_RESOURCE_SERVLET_H
