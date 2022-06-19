//
// Created by liucxi on 2022/6/10.
//

#ifndef LUWU_HTTP_CONNECTION_H
#define LUWU_HTTP_CONNECTION_H

#include <utility>
#include <list>
#include "luwu/streams/socket_stream.h"
#include "http.h"
#include "luwu/uri.h"
#include "luwu/mutex.h"

namespace liucxi {
    namespace http {

        /**
         * @brief HTTP 响应结果
         */
        struct HttpResult {
            typedef std::shared_ptr<HttpResult> ptr;

            enum class Error {
                /// 正常
                OK = 0,
                /// 非法URL
                INVALID_URL = 1,
                /// 无法解析HOST
                INVALID_HOST = 2,
                /// 连接失败
                CONNECT_FAIL = 3,
                /// 连接被对端关闭
                SEND_CLOSE_BY_PEER = 4,
                /// 发送请求产生Socket错误
                SEND_SOCKET_ERROR = 5,
                /// 超时
                TIMEOUT = 6,
                /// 创建Socket失败
                CREATE_SOCKET_ERROR = 7,
                /// 从连接池中取连接失败
                POOL_GET_CONNECTION = 8,
                /// 无效的连接
                POOL_INVALID_CONNECTION = 9,
                /// 不是 web socket
                NOT_WEBSOCKET = 10,
            };

            HttpResult(Error e, std::string s, HttpResponse::ptr rsp)
                    : error(e), errstr(std::move(s)), response(std::move(rsp)) {
            }

            std::string toString() const;

            Error error;
            std::string errstr;
            HttpResponse::ptr response;
        };

        class HttpConnectionPool;

        class HttpConnection : public SocketStream {
            friend class HttpConnectionPool;
        public:
            typedef std::shared_ptr<HttpConnection> ptr;

            static HttpResult::ptr DoGet(const std::string &url, uint64_t timeout,
                                         const std::map<std::string, std::string> &headers = {},
                                         const std::string &body = "");

            static HttpResult::ptr DoGet(URI::ptr uri, uint64_t timeout,
                                         const std::map<std::string, std::string> &headers = {},
                                         const std::string &body = "");

            static HttpResult::ptr DoPost(const std::string &url, uint64_t timeout,
                                          const std::map<std::string, std::string> &headers = {},
                                          const std::string &body = "");

            static HttpResult::ptr DoPost(URI::ptr uri, uint64_t timeout,
                                          const std::map<std::string, std::string> &headers = {},
                                          const std::string &body = "");

            static HttpResult::ptr DoRequest(HttpMethod method, const std::string &url,
                                             uint64_t timeout, const std::map<std::string, std::string> &headers = {},
                                             const std::string &body = "");

            static HttpResult::ptr DoRequest(HttpMethod method, URI::ptr uri,
                                             uint64_t timeout, const std::map<std::string, std::string> &headers = {},
                                             const std::string &body = "");

            static HttpResult::ptr DoRequest(const HttpRequest::ptr &request, URI::ptr uri, uint64_t timeout);

            explicit HttpConnection(Socket::ptr socket, bool owner = true);

            HttpResponse::ptr recvResponse();

            size_t sendRequest(const HttpRequest::ptr &req);

        private:
            /// 创建时间
            uint64_t m_createTime = 0;
            /// 该连接已使用的次数，只在使用连接池的情况下有用
            uint64_t m_request = 0;
        };

        class HttpConnectionPool {
        public:
            typedef std::shared_ptr<HttpConnectionPool> ptr;
            typedef Mutex MutexType;

            HttpConnectionPool(std::string host, std::string vhost,
                               uint16_t port, uint32_t time, uint32_t request);

            HttpConnection::ptr getConnection();

            HttpResult::ptr doGet(const std::string &url, uint64_t timeout,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

            HttpResult::ptr doGet(const URI::ptr& uri, uint64_t timeout,
                                  const std::map<std::string, std::string> &headers = {},
                                  const std::string &body = "");

            HttpResult::ptr doPost(const std::string &url, uint64_t timeout,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");

            HttpResult::ptr doPost(const URI::ptr& uri, uint64_t timeout,
                                   const std::map<std::string, std::string> &headers = {},
                                   const std::string &body = "");

            HttpResult::ptr doRequest(HttpMethod method, const std::string &url,
                                      uint64_t timeout, const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");

            HttpResult::ptr doRequest(HttpMethod method, const URI::ptr& uri,
                                      uint64_t timeout, const std::map<std::string, std::string> &headers = {},
                                      const std::string &body = "");

            HttpResult::ptr doRequest(const HttpRequest::ptr &request, uint64_t timeout);

        private:
            static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
        private:
            MutexType m_mutex;
            std::string m_host;
            std::string m_vhost;
            uint16_t m_port;
            uint32_t m_maxAliveTime;
            uint32_t m_maxRequest;
            std::list<HttpConnection *> m_coons;
            std::atomic<uint32_t> m_total = {0};
        };
    }
}
#endif //LUWU_HTTP_CONNECTION_H
