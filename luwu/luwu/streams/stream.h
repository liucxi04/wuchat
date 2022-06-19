//
// Created by liucxi on 2022/6/10.
//

#ifndef LUWU_STREAM_H
#define LUWU_STREAM_H

#include <memory>
#include "luwu/bytearray.h"

namespace liucxi {
    /**
     * @brief 流结构，提供字节流读写接口
     */
    class Stream {
    public:
        typedef std::shared_ptr<Stream> ptr;

        virtual ~Stream() = default;

        virtual size_t read(void *buffer, size_t length) = 0;

        virtual size_t read(ByteArray::ptr ba, size_t length) = 0;

        virtual size_t readFixSize(void *buffer, size_t length);

        virtual size_t readFixSize(ByteArray::ptr ba, size_t length);

        virtual size_t write(const void *buffer, size_t length) = 0;

        virtual size_t write(ByteArray::ptr ba, size_t length) = 0;

        virtual size_t writeFixSize(const void *buffer, size_t length);

        virtual size_t writeFixSize(ByteArray::ptr ba, size_t length);
        
        virtual void close() = 0;
    };
}
#endif //LUWU_STREAM_H
