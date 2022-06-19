//
// Created by liucxi on 2022/6/10.
//

#ifndef LUWU_HTTP_SESSION_H
#define LUWU_HTTP_SESSION_H

#include "luwu/streams/socket_stream.h"
#include "http.h"

namespace liucxi {
    namespace http {
        class HttpSession : public SocketStream {
        public:
            typedef std::shared_ptr<HttpSession> ptr;

            HttpSession(Socket::ptr socket, bool owner = true);

            HttpRequest::ptr recvRequest();

            size_t sendResponse(const HttpResponse::ptr& rsp);
        };
    }
}
#endif //LUWU_HTTP_SESSION_H
