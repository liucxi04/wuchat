/**
 * @file test_http_connection.cc
 * @brief HTTP客户端类测试
 * @version 0.1
 * @date 2021-12-09
 */
#include "luwu/http/http_connection.h"
#include "luwu/iomanager.h"
#include "luwu/macro.h"
#include <iostream>

static liucxi::Logger::ptr g_logger = LUWU_LOG_ROOT();

void test_pool() {
    liucxi::http::HttpConnectionPool::ptr pool(new liucxi::http::HttpConnectionPool(
            "www.sylar.top", "", 80, 1000 * 30, 5));

    liucxi::IOManager::GetThis()->addTimer(
            1000, [pool]() {
                auto r = pool->doGet("/", 300);
                std::cout << r->toString() << std::endl;
            },
            true);
}

void run() {
//    liucxi::Address::ptr addr = liucxi::Address::LookupAnyIPAddress("www.liucxi.xyz:80");
//    if (!addr) {
//        LUWU_LOG_INFO(g_logger) << "get addr error";
//        return;
//    }
//
//    liucxi::Socket::ptr sock = liucxi::Socket::CreateTCP(addr);
//    bool rt = sock->connect(addr);
//    if (!rt) {
//        LUWU_LOG_INFO(g_logger) << "connect " << *addr << " failed";
//        return;
//    }
//
//    liucxi::http::HttpConnection::ptr conn(new liucxi::http::HttpConnection(sock));
//    liucxi::http::HttpRequest::ptr req(new liucxi::http::HttpRequest);
//
//    req->setHeader("host", "www.liucxi.xyz");
//    // 小bug，如果设置了keep-alive，那么要在使用前先调用一次init
//    req->setHeader("connection", "keep-alive");
//    req->init();
//    std::cout << "req:" << std::endl
//              << *req << std::endl;
//
//    conn->sendRequest(req);
//    auto rsp = conn->recvResponse();
//
//    if (!rsp) {
//        LUWU_LOG_INFO(g_logger) << "recv response error";
//        return;
//    }
//    std::cout << "rsp:" << std::endl
//              << *rsp << std::endl;

//    std::cout << "=========================" << std::endl;
//
//    auto r = liucxi::http::HttpConnection::DoGet("http://www.liucxi.xyz/", 300);
//    std::cout << "error=" << (int)r->error
//              << " errstr=" << r->errstr
//              << " rsp=" << (r->response ? r->response->toString() : "")
//              << std::endl;
//
//    std::cout << "=========================" << std::endl;
    test_pool();
}

int main(int argc, char **argv) {
    liucxi::IOManager iom;
    iom.scheduler(run);
    return 0;
}
