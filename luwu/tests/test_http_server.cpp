//
// Created by liucxi on 2022/6/11.
//
#include "luwu/http/http_server.h"

void run() {
    liucxi::http::HttpServer::ptr server(new liucxi::http::HttpServer(true));
    liucxi::Address::ptr address = liucxi::Address::LookupAny("0.0.0.0:1234");
    while (!server->bind(address)) {
        sleep(2);
    }
    auto sd = server->getDispatch();
    sd->addServlet("/luwu/xx", [](const liucxi::http::HttpRequest::ptr& req, 
            const liucxi::http::HttpResponse::ptr& rsp, const liucxi::http::HttpSession::ptr& session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/luwu/*", [](const liucxi::http::HttpRequest::ptr& req,
            const liucxi::http::HttpResponse::ptr& rsp, const liucxi::http::HttpSession::ptr& session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    
    server->start();
}



int main() {
    liucxi::IOManager iom;
    iom.scheduler(run);
    return 0;
}
