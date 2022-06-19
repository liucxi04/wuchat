//
// Created by liucxi on 2022/6/7.
//

#ifndef LUWU_HTTP_PARSER_H
#define LUWU_HTTP_PARSER_H

#include "http.h"

namespace liucxi {
    namespace http {

        class HttpRequestParser {
        public:
            typedef std::shared_ptr<HttpRequestParser> ptr;

            static uint64_t GetHttpRequestBufferSize();

            static uint64_t GetHttpRequestMaxBodySize();

            HttpRequestParser();

            size_t execute(char *data, size_t len);

            bool isFinished() const { return m_finished; }

            void setFinished(bool finished) { m_finished = finished; }

            int getError() const { return m_error; }

            void setError(int error) { m_error = error; }

            HttpRequest::ptr getData() const { return m_data; }

            const http_parser &getParser() const { return m_parser; }

            const std::string &getFiled() const { return m_filed; }

            void setFiled(const std::string &filed) { m_filed = filed; }

        private:
            http_parser m_parser{};
            HttpRequest::ptr m_data;
            int m_error;
            bool m_finished;
            std::string m_filed;
        };

        class HttpResponseParser {
        public:
            typedef std::shared_ptr<HttpResponseParser> ptr;

            static uint64_t GetHttpResponseBufferSize();

            static uint64_t GetHttpResponseMaxBodySize();

            HttpResponseParser();

            size_t execute(char *data, size_t len);

            bool isFinished() const { return m_finished; }

            void setFinished(bool finished) { m_finished = finished; }

            int getError() const { return m_error; }

            void setError(int error) { m_error = error; }

            HttpResponse::ptr getData() const { return m_data; }

            const http_parser &getParser() const { return m_parser; }

            const std::string &getFiled() const { return m_filed; }

            void setFiled(const std::string &filed) { m_filed = filed; }

        private:
            http_parser m_parser{};
            HttpResponse::ptr m_data;
            int m_error;
            bool m_finished;
            std::string m_filed;
        };
    }
}
#endif //LUWU_HTTP_PARSER_H
