//
// Created by liucxi on 2022/6/7.
//

#include <iostream>
#include "http_parser.h"
#include "luwu/macro.h"
#include "luwu/config.h"

namespace liucxi {
    namespace http {

        static liucxi::Logger::ptr g_logger = LUWU_LOG_NAME("http");

        static liucxi::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
                liucxi::Config::lookup("http.request.buffer_size",
                                       (uint64_t) (4 * 1024), "http request buffer_size");

        static liucxi::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
                liucxi::Config::lookup("http.request.max_body_size",
                                       (uint64_t) (64 * 1024 * 1024), "http request max_body_size");

        static liucxi::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
                liucxi::Config::lookup("http.response.buffer_size",
                                       (uint64_t) (4 * 1024), "http response buffer_size");

        static liucxi::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
                liucxi::Config::lookup("http.response.max_body_size",
                                       (uint64_t) (64 * 1024 * 1024), "http response max_body_size");

        static uint64_t s_http_request_buffer_size = 0;
        static uint64_t s_http_request_max_body_size = 0;
        static uint64_t s_http_response_buffer_size = 0;
        static uint64_t s_http_response_max_body_size = 0;

        uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
            return s_http_request_buffer_size;
        }

        uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
            return s_http_request_max_body_size;
        }

        uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
            return s_http_response_buffer_size;
        }

        uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
            return s_http_response_max_body_size;
        }

        namespace {
            struct HttpInit {
                HttpInit() {
                    s_http_request_buffer_size = g_http_request_buffer_size->getValue();
                    s_http_request_max_body_size = g_http_request_max_body_size->getValue();
                    s_http_response_buffer_size = g_http_response_buffer_size->getValue();
                    s_http_response_max_body_size = g_http_response_max_body_size->getValue();

                    g_http_request_buffer_size->addListener(
                            [](const uint64_t &oldVal, const uint64_t &newVal) {
                                s_http_request_buffer_size = newVal;
                            });
                    g_http_request_max_body_size->addListener(
                            [](const uint64_t &oldVal, const uint64_t &newVal) {
                                s_http_request_max_body_size = newVal;
                            });
                    g_http_response_buffer_size->addListener(
                            [](const uint64_t &oldVal, const uint64_t &newVal) {
                                s_http_response_buffer_size = newVal;
                            }
                    );
                    g_http_response_max_body_size->addListener(
                            [](const uint64_t &oldVal, const uint64_t &newVal) {
                                s_http_response_max_body_size = newVal;
                            });
                }
            };

            static HttpInit _init;
        }

        /**
         * @brief http 请求开始解析
         */
        static int on_request_message_begin_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_message_begin_cb";
            return 0;
        }

        /**
         * @brief http 头部字段解析结束
         */
        static int on_request_headers_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_headers_complete_cb";
            auto *parser = static_cast<HttpRequestParser *>(p->data);
            parser->getData()->setVersion(((p->http_major) << 0x4 | (p->http_minor)));
            parser->getData()->setMethod((HttpMethod) (p->method));
            return 0;
        }

        /**
         * @brief http 请求解析结束
         */
        static int on_request_message_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_message_complete_cb";
            auto *parser = static_cast<HttpRequestParser *>(p->data);
            parser->setFinished(true);
            return 0;
        }

        /**
         * @brief http 分段头部开始
         */
        static int on_request_chunk_header_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_chunk_header_cb";
            return 0;
        }

        /**
         * @brief http 分段头部结束
         */
        static int on_request_chunk_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_chunk_complete_cb";
            return 0;
        }

        /**
         * @brief http url 解析完成后
         */
        static int on_request_url_cb(http_parser *p, const char *buf, size_t len) {
            LUWU_LOG_DEBUG(g_logger) << "on_request_url_cb, url is: " << std::string(buf, len);
            auto *parser = static_cast<HttpRequestParser *>(p->data);

            struct http_parser_url url_parser{};
            http_parser_url_init(&url_parser);
            int rt = http_parser_parse_url(buf, len, 0, &url_parser);

            if (rt != 0) {
                LUWU_LOG_ERROR(g_logger) << "parser url error";
                return 1;
            }

            if (url_parser.field_set & (1 << UF_PATH)) {
                parser->getData()->setPath(std::string(buf + url_parser.field_data[UF_PATH].off,
                                                       url_parser.field_data[UF_PATH].len));
            }
            if (url_parser.field_set & (1 << UF_QUERY)) {
                parser->getData()->setQuery(std::string(buf + url_parser.field_data[UF_QUERY].off,
                                                       url_parser.field_data[UF_QUERY].len));
            }
            if (url_parser.field_set & (1 << UF_FRAGMENT)) {
                parser->getData()->setFragment(std::string(buf + url_parser.field_data[UF_FRAGMENT].off,
                                                       url_parser.field_data[UF_FRAGMENT].len));
            }
            return 0;
        }

        /**
         * @brief http 请求头部字段名称解析完成
         */
        static int on_request_header_filed_cb(http_parser *p, const char *buf, size_t len) {
            std::string filed(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_request_header_filed_cb, filed is: " << filed;
            auto *parser = static_cast<HttpRequestParser *>(p->data);
            parser->setFiled(filed);
            return 0;
        }

        /**
         * @brief http 请求头部字段值解析完成
         */
        static int on_request_header_value_cb(http_parser *p, const char *buf, size_t len) {
            std::string value(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_request_header_value_cb, value is: " << value;
            auto *parser = static_cast<HttpRequestParser *>(p->data);
            parser->getData()->setHeader(parser->getFiled(), value);
            return 0;
        }

        /**
         * @brief http 请求相应状态， 没有用
         */
        static int on_request_status_cb(http_parser *p, const char *buf, size_t len) {
            return 0;
        }

        /**
         * @brief http 消息体
         */
        static int on_request_body_cb(http_parser *p, const char *buf, size_t len) {
            std::string body(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_request_body_cb, body is: " << body;
            auto *parser = static_cast<HttpRequestParser *>(p->data);
            parser->getData()->appendBody(body);
            return 0;
        }

        static http_parser_settings httpParserSettings1 = {
                .on_message_begin    = on_request_message_begin_cb,
                .on_url              = on_request_url_cb,
                .on_status           = on_request_status_cb,
                .on_header_field     = on_request_header_filed_cb,
                .on_header_value     = on_request_header_value_cb,
                .on_headers_complete = on_request_headers_complete_cb,
                .on_body             = on_request_body_cb,
                .on_message_complete = on_request_message_complete_cb,
                .on_chunk_header     = on_request_chunk_header_cb,
                .on_chunk_complete   = on_request_chunk_complete_cb
        };

        HttpRequestParser::HttpRequestParser() {
            http_parser_init(&m_parser, HTTP_REQUEST);
            m_data.reset(new HttpRequest);
            m_parser.data = this;
            m_error = 0;
            m_finished = false;
        }

        size_t HttpRequestParser::execute(char *data, size_t len) {
            size_t parsed = http_parser_execute(&m_parser, &httpParserSettings1, data, len);
            if (m_parser.upgrade) {
                LUWU_LOG_DEBUG(g_logger) << "found upgrade, ignore";
                setError(HPE_UNKNOWN);
            } else if ((int) m_parser.http_errno != 0) {
                LUWU_LOG_ERROR(g_logger) << "parse request fail: " << http_errno_name(HTTP_PARSER_ERRNO(&m_parser));
                setError((int) m_parser.http_errno);
            } else {
                if (parsed < len) {
                    memmove(data, data + parsed, (len - parsed));
                }
            }
            return parsed;
        }

        /**
         * @brief http 响应开始解析
         */
        static int on_response_message_begin_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_message_begin_cb";
            return 0;
        }

        /**
         * @brief http 头部字段解析结束
         */
        static int on_response_headers_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_headers_complete_cb";
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->getData()->setVersion(((p->http_major) << 0x4 | (p->http_minor)));
            parser->getData()->setStatus((HttpStatus) (p->status_code));
            return 0;
        }

        /**
         * @brief http 响应解析结束
         */
        static int on_response_message_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_message_complete_cb";
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->setFinished(true);
            return 0;
        }

        /**
         * @brief http 分段头部开始
         */
        static int on_response_chunk_header_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_chunk_header_cb";
            return 0;
        }

        /**
         * @brief http 分段头部结束
         */
        static int on_response_chunk_complete_cb(http_parser *p) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_chunk_complete_cb";
            return 0;
        }

        /**
         * @brief http url 解析完成后，没有用
         */
        static int on_response_url_cb(http_parser *p, const char *buf, size_t len) {
            return 0;
        }

        /**
         * @brief http 响应头部字段名称解析完成
         */
        static int on_response_header_filed_cb(http_parser *p, const char *buf, size_t len) {
            std::string filed(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_response_header_filed_cb, filed is: " << filed;
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->setFiled(filed);
            return 0;
        }

        /**
         * @brief http 头部字段值解析完成
         */
        static int on_response_header_value_cb(http_parser *p, const char *buf, size_t len) {
            std::string value(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_response_header_value_cb, value is: " << value;
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->getData()->setHeader(parser->getFiled(), value);
            return 0;
        }

        /**
         * @brief http 响应相应状态
         */
        static int on_response_status_cb(http_parser *p, const char *buf, size_t len) {
            LUWU_LOG_DEBUG(g_logger) << "on_response_status_cb, status code is: " << p->status_code
                                     << ", status msg is: " << std::string(buf, len);
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->getData()->setStatus((HttpStatus) (p->status_code));
            return 0;
        }

        /**
         * @brief http 消息体
         */
        static int on_response_body_cb(http_parser *p, const char *buf, size_t len) {
            std::string body(buf, len);
            LUWU_LOG_DEBUG(g_logger) << "on_response_body_cb, body is: " << body;
            auto *parser = static_cast<HttpResponseParser *>(p->data);
            parser->getData()->appendBody(body);
            return 0;
        }

        static http_parser_settings httpParserSettings2 = {
                .on_message_begin    = on_response_message_begin_cb,
                .on_url              = on_response_url_cb,
                .on_status           = on_response_status_cb,
                .on_header_field     = on_response_header_filed_cb,
                .on_header_value     = on_response_header_value_cb,
                .on_headers_complete = on_response_headers_complete_cb,
                .on_body             = on_response_body_cb,
                .on_message_complete = on_response_message_complete_cb,
                .on_chunk_header     = on_response_chunk_header_cb,
                .on_chunk_complete   = on_response_chunk_complete_cb
        };

        HttpResponseParser::HttpResponseParser() {
            http_parser_init(&m_parser, HTTP_RESPONSE);
            m_data.reset(new HttpResponse);
            m_parser.data = this;
            m_error = 0;
            m_finished = false;
        }

        size_t HttpResponseParser::execute(char *data, size_t len) {
            size_t parsed = http_parser_execute(&m_parser, &httpParserSettings2, data, len);
            if ((int) m_parser.http_errno != 0) {
                LUWU_LOG_ERROR(g_logger) << "parse request fail: " << http_errno_name(HTTP_PARSER_ERRNO(&m_parser));
                setError((int) m_parser.http_errno);
            } else {
                if (parsed < len) {
                    memmove(data, data + parsed, (len - parsed));
                }
            }
            return parsed;
        }
    }
}