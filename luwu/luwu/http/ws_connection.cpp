#include "ws_connection.h"
#include "luwu/util/hash_util.h"

namespace liucxi {
    namespace http {

        WSConnection::WSConnection(Socket::ptr sock, bool owner)
                : HttpConnection(sock, owner) {
        }

        std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::Create(const std::string &url, uint64_t timeout_ms,
                                                                           const std::map<std::string, std::string> &headers) {
            URI::ptr uri = URI::Create(url);
            if (!uri) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                                                   "invalid url:" + url, nullptr), nullptr);
            }
            return Create(uri, timeout_ms, headers);
        }

        std::pair<HttpResult::ptr, WSConnection::ptr>
        WSConnection::Create(URI::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string> &headers) {
            Address::ptr addr = uri->createAddress();
            if (!addr) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::INVALID_HOST,
                                                                   "invalid host: " + uri->getHost(), nullptr),
                                      nullptr);
            }
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock) {
                return std::make_pair(
                        std::make_shared<HttpResult>(HttpResult::Error::CREATE_SOCKET_ERROR,
                                                     "create socket fail: " + addr->toString()
                                                     + " errno=" + std::to_string(errno)
                                                     + " errstr=" + std::string(strerror(errno)), nullptr),
                                                     nullptr);
            }
            if (!sock->connect(addr)) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL,
                                                                   "connect fail: " + addr->toString(), nullptr),
                                                                    nullptr);
            }
            sock->setRecvTimeout(timeout_ms);
            WSConnection::ptr conn = std::make_shared<WSConnection>(sock);

            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(uri->getPath());
            req->setQuery(uri->getQuery());
            req->setFragment(uri->getFragment());
            req->setMethod(HttpMethod::GET);
            bool has_host = false;
            bool has_conn = false;
            for (auto &i: headers) {
                if (strcasecmp(i.first.c_str(), "connection") == 0) {
                    has_conn = true;
                } else if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);
            }
            req->setWebSocket(true);
            if (!has_conn) {
                req->setHeader("connection", "Upgrade");
            }
            req->setHeader("Upgrade", "websocket");
            req->setHeader("Sec-webSocket-Version", "13");
            req->setHeader("Sec-webSocket-Key", base64encode(random_string(16)));
            if (!has_host) {
                req->setHeader("Host", uri->getHost());
            }

            int rt = conn->sendRequest(req);
            if (rt == 0) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::SEND_CLOSE_BY_PEER,
                                                                   "send request closed by peer: " + addr->toString(),
                                                                   nullptr), nullptr);
            }
            if (rt < 0) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::SEND_SOCKET_ERROR,
                                                                   "send request socket error errno=" +
                                                                   std::to_string(errno)
                                                                   + " errstr=" + std::string(strerror(errno)),
                                                                   nullptr), nullptr);
            }
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT,
                                                                   "recv response timeout: " + addr->toString()
                                                                   + " timeout_ms:" + std::to_string(timeout_ms),
                                                                   nullptr), nullptr);
            }

            if (rsp->getStatus() != HttpStatus::SWITCHING_PROTOCOLS) {
                return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::NOT_WEBSOCKET,
                                                                   "not websocket server " + addr->toString(),
                                                                   rsp), nullptr);
            }
            return std::make_pair(std::make_shared<HttpResult>(HttpResult::Error::OK, "ok", rsp), conn);
        }

        WSFrameMessage::ptr WSConnection::recvMessage() {
            return WSRecvMessage(this, true);
        }

        int32_t WSConnection::sendMessage(WSFrameMessage::ptr msg, bool fin) {
            return WSSendMessage(this, msg, true, fin);
        }

        int32_t WSConnection::sendMessage(const std::string &msg, int32_t opcode, bool fin) {
            return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), true, fin);
        }

        int32_t WSConnection::ping() {
            return WSPing(this);
        }

        int32_t WSConnection::pong() {
            return WSPong(this);
        }

    }
}
