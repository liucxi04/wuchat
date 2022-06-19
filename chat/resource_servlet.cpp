//
// Created by liucxi on 2022/6/18.
//

#include "resource_servlet.h"
#include "luwu/luwu/macro.h"
#include <iostream>
#include <utility>

namespace liucxi {
    namespace http {

        static Logger::ptr g_logger = LUWU_LOG_ROOT();

        ResourceServlet::ResourceServlet(std::string path)
                : Servlet("ResourceServlet"), m_path(std::move(path)) {
        }

        int32_t ResourceServlet::handle(http::HttpRequest::ptr request,
                                        http::HttpResponse::ptr response,
                                        http::HttpSession::ptr session) {

            auto path = m_path + "/" + request->getPath();
            LUWU_LOG_INFO(g_logger) << "handle path = " << path;
            if (path.find("..") != std::string::npos) {
                response->setBody("invalid path");
                response->setStatus(http::HttpStatus::NOT_FOUND);
                return 0;
            }

            std::ifstream ifs(path);
            if (!ifs) {
                response->setBody("invalid file");
                response->setStatus(http::HttpStatus::NOT_FOUND);
                return 0;
            }

            std::stringstream ss;
            std::string line;
            while (std::getline(ifs, line)) {
                ss << line << std::endl;
            }

            response->setBody(ss.str());
            response->setHeader("content-type", "text/html;charset=utf-8");
            return 0;
        }
    }
}


