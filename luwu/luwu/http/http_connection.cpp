//
// Created by liucxi on 2022/6/10.
//
#include "http_connection.h"
#include "http_parser.h"
#include "luwu/utils.h"
#include <utility>

namespace liucxi {
    namespace http {

        std::string HttpResult::toString() const {
            std::stringstream ss;
            ss << "[HttpResult error=" << (int)error
               << " errstr=" << errstr
               << " response=" << (response ? response->toString() : "nullptr")
               << "]";
            return ss.str();
        }

        HttpResult::ptr HttpConnection::DoGet(const std::string &url, uint64_t timeout,
                                              const std::map<std::string, std::string> &headers,
                                              const std::string &body) {
            URI::ptr uri = URI::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                                    "invalid url" + url, nullptr);
            }
            return DoGet(uri, timeout, headers, body);
        }

        HttpResult::ptr HttpConnection::DoGet(URI::ptr uri, uint64_t timeout,
                                              const std::map<std::string, std::string> &headers,
                                              const std::string &body) {
            return DoRequest(HttpMethod::GET, std::move(uri), timeout, headers, body);
        }

        HttpResult::ptr HttpConnection::DoPost(const std::string &url, uint64_t timeout,
                                               const std::map<std::string, std::string> &headers,
                                               const std::string &body) {
            URI::ptr uri = URI::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                                    "invalid url" + url, nullptr);
            }
            return DoPost(uri, timeout, headers, body);
        }

        HttpResult::ptr HttpConnection::DoPost(URI::ptr uri, uint64_t timeout,
                                               const std::map<std::string, std::string> &headers,
                                               const std::string &body) {
            return DoRequest(HttpMethod::POST, std::move(uri), timeout, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string &url, uint64_t timeout,
                                                  const std::map<std::string, std::string> &headers,
                                                  const std::string &body) {
            URI::ptr uri = URI::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                                    "invalid url" + url, nullptr);
            }
            return DoRequest(method, uri, timeout, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, URI::ptr uri, uint64_t timeout,
                                                  const std::map<std::string, std::string> &headers,
                                                  const std::string &body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(uri->getPath());
            req->setQuery(uri->getQuery());
            req->setFragment(uri->getFragment());
            req->setMethod(method);
            bool hasHost = false;
            for (const auto &i: headers) {
                if (strcasecmp(i.first.c_str(), "Connection") == 0) {
                    if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }

                if (!hasHost && strcasecmp(i.first.c_str(), "host") == 0) {
                    hasHost = !i.second.empty();
                }
                req->setHeader(i.first, i.second);
            }

            if (!hasHost) {
                req->setHeader("Host", uri->getHost());
            }
            req->setBody(body);
            return DoRequest(req, uri, timeout);
        }

        HttpResult::ptr HttpConnection::DoRequest(const HttpRequest::ptr &request, URI::ptr uri, uint64_t timeout) {
            Address::ptr addr = uri->createAddress();
            if (!addr) {
                return std::make_shared<HttpResult>(HttpResult::Error::INVALID_HOST,
                                                    "invalid host: " + uri->getHost(),
                                                    nullptr);
            }
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock) {
                return std::make_shared<HttpResult>(HttpResult::Error::CREATE_SOCKET_ERROR,
                                                    "create socket fail: " + addr->toString()
                                                      + " errno=" + std::to_string(errno)
                                                      + " errstr=" + std::string(strerror(errno)),
                                                    nullptr);
            }
            if (!sock->connect(addr)) {
                return std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL,
                                                    "connect fail: " + addr->toString(),
                                                    nullptr);
            }
            sock->setRecvTimeout(timeout);
            HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
            size_t rt = conn->sendRequest(request);
            if (rt == 0) {
                return std::make_shared<HttpResult>(HttpResult::Error::SEND_CLOSE_BY_PEER,
                                                    "send request closed by peer: " + addr->toString(),
                                                    nullptr);
            }
            if (rt < 0) {
                return std::make_shared<HttpResult>(HttpResult::Error::SEND_SOCKET_ERROR,
                                                    "send request socket error"
                                                    " errno=" + std::to_string(errno)
                                                    + " errstr=" + std::string(strerror(errno)),
                                                    nullptr);
            }
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT,
                                                    "recv response timeout: " + addr->toString()
                                                    + " timeout:" + std::to_string(timeout),
                                                    nullptr);
            }
            return std::make_shared<HttpResult>(HttpResult::Error::OK, "ok", rsp);
        }


        HttpConnection::HttpConnection(Socket::ptr socket, bool owner)
                : SocketStream(std::move(socket), owner) {
        }

        HttpResponse::ptr HttpConnection::recvResponse() {
            HttpResponseParser::ptr parser(new HttpResponseParser);
            uint64_t bufferSize = HttpResponseParser::GetHttpResponseBufferSize();
            std::shared_ptr<char> buffer(new char[bufferSize + 1], [](const char *ptr) { delete[] ptr; });
            char *data = buffer.get();
            size_t offset = 0;
            do {
                size_t len = read(data + offset, bufferSize - offset);
                if (len <= 0) {
                    close();
                    return nullptr;
                }

                len += offset;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len);
                if (parser->getError()) {
                    close();
                    return nullptr;
                }

                offset = len - nparse;
                if (offset == bufferSize) {
                    close();
                    return nullptr;
                }

                if (parser->isFinished()) {
                    break;
                }
            } while (true);
            return parser->getData();
        }

        size_t HttpConnection::sendRequest(const HttpRequest::ptr &req) {
            std::stringstream ss;
            ss << *req;
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size());
        }

        HttpConnectionPool::HttpConnectionPool(std::string host, std::string vhost, uint16_t port,
                                               uint32_t time, uint32_t request)
            : m_host(std::move(host))
            , m_vhost(std::move(vhost))
            , m_port(port)
            , m_maxAliveTime(time)
            , m_maxRequest(request){
        }

        HttpConnection::ptr HttpConnectionPool::getConnection() {
            uint64_t now = GetCurrentMS();
            std::vector<HttpConnection*> invalid_conns;
            HttpConnection *ptr = nullptr;
            MutexType::Lock lock(m_mutex);
            while (!m_coons.empty()) {
                auto conn = *m_coons.begin();
                m_coons.pop_front();
                if (!conn->isConnected()) {
                    invalid_conns.push_back(conn);
                    continue;
                }
                if ((conn->m_createTime + m_maxAliveTime) > now) {
                    invalid_conns.push_back(conn);
                    continue;
                }
                ptr = conn;
                break;
            }
            lock.unlock();

            for (auto i : invalid_conns) {
                delete i;
            }
            m_total -= invalid_conns.size();

            if (!ptr) {
                IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
                if(!addr) {
                    return nullptr;
                }
                addr->setPort(m_port);
                Socket::ptr sock = Socket::CreateTCP(addr);
                if(!sock) {
                    return nullptr;
                }
                if(!sock->connect(addr)) {
                    return nullptr;
                }

                ptr = new HttpConnection(sock);
                ++m_total;
            }
            return {ptr, [this](auto && PH1) {
                return HttpConnectionPool::ReleasePtr(std::forward<decltype(PH1)>(PH1), this);
            }};
        }

        void HttpConnectionPool::ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool) {
            ++ptr->m_request;
            if(!ptr->isConnected()
               || ((ptr->m_createTime + pool->m_maxAliveTime) >= GetCurrentMS())
               || (ptr->m_request >= pool->m_maxRequest)) {
                delete ptr;
                --pool->m_total;
                return;
            }
            MutexType::Lock lock(pool->m_mutex);
            pool->m_coons.push_back(ptr);
        }

        HttpResult::ptr HttpConnectionPool::doGet(const std::string &url, uint64_t timeout,
                                                  const std::map<std::string, std::string> &headers,
                                                  const std::string &body) {
            return doRequest(HttpMethod::GET, url, timeout, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doGet(const URI::ptr& uri, uint64_t timeout,
                                                  const std::map<std::string, std::string> &headers,
                                                  const std::string &body) {
            std::stringstream ss;
            ss << uri->getPath()
               << (uri->getQuery().empty() ? "" : "?")
               << uri->getQuery()
               << (uri->getFragment().empty() ? "" : "#")
               << uri->getFragment();
            return doGet(ss.str(), timeout, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doPost(const std::string &url, uint64_t timeout,
                                                   const std::map<std::string, std::string> &headers,
                                                   const std::string &body) {
            return doRequest(HttpMethod::POST, url, timeout, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doPost(const URI::ptr& uri, uint64_t timeout,
                                                   const std::map<std::string, std::string> &headers,
                                                   const std::string &body) {
            std::stringstream ss;
            ss << uri->getPath()
               << (uri->getQuery().empty() ? "" : "?")
               << uri->getQuery()
               << (uri->getFragment().empty() ? "" : "#")
               << uri->getFragment();
            return doPost(ss.str(), timeout, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string &url, uint64_t timeout,
                                                      const std::map<std::string, std::string> &headers,
                                                      const std::string &body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(url);
            req->setMethod(method);
            req->setClose(false);
            bool has_host = false;
            for(auto& i : headers) {
                if(strcasecmp(i.first.c_str(), "Connection") == 0) {
                    if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }

                if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);
            }

            if(!has_host) {
                if(m_vhost.empty()) {
                    req->setHeader("Host", m_host);
                } else {
                    req->setHeader("Host", m_vhost);
                }
            }
            req->setBody(body);
            return doRequest(req, timeout);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const URI::ptr& uri, uint64_t timeout,
                                                      const std::map<std::string, std::string> &headers,
                                                      const std::string &body) {
            std::stringstream ss;
            ss << uri->getPath()
               << (uri->getQuery().empty() ? "" : "?")
               << uri->getQuery()
               << (uri->getFragment().empty() ? "" : "#")
               << uri->getFragment();
            return doRequest(method, ss.str(), timeout, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(const HttpRequest::ptr &request, uint64_t timeout) {
            auto conn = getConnection();
            if(!conn) {
                return std::make_shared<HttpResult>(HttpResult::Error::POOL_GET_CONNECTION,
                                                    "pool host:" + m_host + " port:" + std::to_string(m_port),
                                                    nullptr);
            }
            auto sock = conn->getSocket();
            if(!sock) {
                return std::make_shared<HttpResult>(HttpResult::Error::POOL_INVALID_CONNECTION,
                                                    "pool host:" + m_host + " port:" + std::to_string(m_port),
                                                    nullptr);
            }
            sock->setRecvTimeout(timeout);
            size_t rt = conn->sendRequest(request);
            if(rt == 0) {
                return std::make_shared<HttpResult>(HttpResult::Error::SEND_CLOSE_BY_PEER,
                                                    "send request closed by peer: "
                                                    + sock->getRemoteAddress()->toString(),
                                                    nullptr);
            }
            if(rt < 0) {
                return std::make_shared<HttpResult>(HttpResult::Error::SEND_SOCKET_ERROR,
                                                    "send request socket error errno="
                                                    + std::to_string(errno)
                                                    + " errstr=" + std::string(strerror(errno)),
                                                    nullptr);
            }
            auto rsp = conn->recvResponse();
            if(!rsp) {
                return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT,
                                                    "recv response timeout: "
                                                    + sock->getRemoteAddress()->toString()
                                                    + " timeout_ms:" + std::to_string(timeout),
                                                    nullptr);
            }
            return std::make_shared<HttpResult>(HttpResult::Error::OK, "ok", rsp);
        }
    }
}
