/**
 * @file test_http_server.cc
 * @brief HttpServer测试
 * @version 0.1
 * @date 2021-09-28
 */
#include "luwu/luwu.h"

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

#define XX(...) #__VA_ARGS__

liucxi::IOManager::ptr worker;

void run() {
    g_logger->setLevel(liucxi::LogLevel::INFO);
    //liucxi::http::HttpServer::ptr server(new liucxi::http::HttpServer(true, worker.get(), liucxi::IOManager::GetThis()));
    liucxi::http::HttpServer::ptr server(new liucxi::http::HttpServer(true));
    liucxi::Address::ptr addr = liucxi::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getDispatch();
    sd->addServlet("/liucxi/xx", [](liucxi::http::HttpRequest::ptr req, liucxi::http::HttpResponse::ptr rsp, liucxi::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/liucxi/*", [](liucxi::http::HttpRequest::ptr req, liucxi::http::HttpResponse::ptr rsp, liucxi::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    sd->addGlobServlet("/liucxix/*", [](liucxi::http::HttpRequest::ptr req, liucxi::http::HttpResponse::ptr rsp, liucxi::http::HttpSession::ptr session) {
        rsp->setBody(XX(<html>
                                <head><title> 404 Not Found</ title></ head>
                                <body>
                                <center><h1> 404 Not Found</ h1></ center>
                                <hr><center>
                                nginx /
                                1.16.0 <
                                / center >
                                </ body>
                                </ html> < !--a padding to disable MSIE and
                                Chrome friendly error page-- >
                                < !--a padding to disable MSIE and
                                Chrome friendly error page-- >
                                < !--a padding to disable MSIE and
                                Chrome friendly error page-- >
                                < !--a padding to disable MSIE and
                                Chrome friendly error page-- >
                                < !--a padding to disable MSIE and
                                Chrome friendly error page-- >
                                < !--a padding to disable MSIE and
                                Chrome friendly error page-- >));
        return 0;
    });

    server->start();
}

int main(int argc, char **argv) {
    liucxi::IOManager iom(1, true, "main");
    worker.reset(new liucxi::IOManager(3, false, "worker"));
    iom.scheduler(run);
    return 0;
}
