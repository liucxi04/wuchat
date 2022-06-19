#include "luwu/luwu.h"

liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();
liucxi::IOManager::ptr worker;
void run() {
    liucxi::http::HttpServer::ptr http_server(new liucxi::http::HttpServer(true));
    liucxi::Address::ptr addr = liucxi::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        LUWU_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    while(!http_server->bind(addr)) {
        LUWU_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    http_server->start();
}

int main(int argc, char** argv) {
    liucxi::IOManager iom(2);
    iom.scheduler(run);
    return 0;
}
