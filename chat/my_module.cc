#include "my_module.h"
#include "resource_servlet.h"
#include "chat_servlet.h"

#include "luwu/luwu/macro.h"
#include "luwu/luwu/env.h"
#include "luwu/luwu/tcp_server.h"
#include "luwu/luwu/application.h"
#include "luwu/luwu/http/http_server.h"
#include "luwu/luwu/http/ws_server.h"


namespace chat {

    static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

    MyModule::MyModule()
            : liucxi::Module("chat", "1.0", "") {
    }

    bool MyModule::onLoad() {
        LUWU_LOG_INFO(g_logger) << "onLoad";
        return true;
    }

    bool MyModule::onUnload() {
        LUWU_LOG_INFO(g_logger) << "onUnload";
        return true;
    }

    //static int32_t handle(liucxi::http::HttpRequest::ptr request
    //            , liucxi::http::HttpResponse::ptr response
    //            , liucxi::http::HttpSession::ptr session) {
    //    LUWU_LOG_INFO(g_logger) << "handle";
    //    response->setBody("handle");
    //    return 0;
    //}

    bool MyModule::onServerReady() {
        LUWU_LOG_INFO(g_logger) << "onServerReady";
        std::vector<liucxi::TCPServer::ptr> servs;
        if (!liucxi::Application::GetInstance()->getServer("http", servs)) {
            LUWU_LOG_INFO(g_logger) << "no http server alive";
            return false;
        }

        for (auto &i: servs) {
            liucxi::http::HttpServer::ptr http_server =
                    std::dynamic_pointer_cast<liucxi::http::HttpServer>(i);
            if (!i) {
                continue;
            }
            liucxi::http::ResourceServlet::ptr slt(new liucxi::http::ResourceServlet(
                    liucxi::EnvMgr::getInstance()->getCwd()
            ));
            auto slt_dis = http_server->getDispatch();
            slt_dis->addGlobServlet("/html/*", slt);
        }

        servs.clear();
        if (!liucxi::Application::GetInstance()->getServer("ws", servs)) {
            LUWU_LOG_INFO(g_logger) << "no ws server alive";
            return false;
        }
        for (auto &i: servs) {
            liucxi::http::WSServer::ptr ws_server =
                    std::dynamic_pointer_cast<liucxi::http::WSServer>(i);
            if (!i) {
                continue;
            }
            liucxi::http::ChatServlet::ptr slt(new liucxi::http::ChatServlet());
            liucxi::http::ServletDispatch::ptr slt_dis = ws_server->getWSServletDispatch();
            slt_dis->addServlet("/liucxi/chat", slt);

        }
        return true;
    }

    bool MyModule::onServerUp() {
        LUWU_LOG_INFO(g_logger) << "onServerUp";
        return true;
    }

}

extern "C" {

liucxi::Module *CreateModule() {
    liucxi::Module *module = new chat::MyModule;
    LUWU_LOG_INFO(chat::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(liucxi::Module *module) {
    LUWU_LOG_INFO(chat::g_logger) << "CreateModule " << module;
    delete module;
}

}
