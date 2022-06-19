//
// Created by liucxi on 2022/6/10.
//
#include "http_session.h"
#include "http_parser.h"
#include <utility>

namespace liucxi {
    namespace http {
        HttpSession::HttpSession(Socket::ptr socket, bool owner)
            : SocketStream(std::move(socket), owner){
        }

        HttpRequest::ptr HttpSession::recvRequest() {
            HttpRequestParser::ptr parser(new HttpRequestParser);
            uint64_t bufferSize = HttpRequestParser::GetHttpRequestBufferSize();
            std::shared_ptr<char> buffer(new char[bufferSize], [](const char *ptr){ delete[] ptr; });
            char *data = buffer.get();
            size_t offset = 0;
            do {
                size_t len = read(data + offset, bufferSize - offset);
                if (len <= 0) {
                    close();
                    return nullptr;
                }

                len += offset;
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

            parser->getData()->init();
            return parser->getData();
        }

        size_t HttpSession::sendResponse(const HttpResponse::ptr& rsp) {
            std::stringstream ss;
            ss << *rsp;
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size());
        }
    }
}
