//
// Created by liucxi on 2022/6/3.
//
#include <cstring>
#include <fstream>
#include <cmath>
#include "bytearray.h"
#include "utils.h"
#include "macro.h"

namespace liucxi {

    ByteArray::Node::Node()
            : ptr(nullptr), next(nullptr), size(0) {
    }

    ByteArray::Node::Node(size_t s)
            : ptr(new char[s]), next(nullptr), size(s) {
    }

    ByteArray::Node::~Node() {
        delete[] ptr;
    }

    ByteArray::ByteArray(size_t base_size)
            : m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0), m_endian(BIG_ENDIAN),
              m_root(new Node(base_size)), m_cur(m_root) {
    }

    ByteArray::~ByteArray() {
        Node *tmp = m_root;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }

    void ByteArray::writeFixInt8(int8_t val) {
        write(&val, 1);
    }

    void ByteArray::writeFixUint8(uint8_t val) {
        write(&val, 1);
    }

    void ByteArray::writeFixInt16(int16_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    void ByteArray::writeFixUint16(uint16_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    void ByteArray::writeFixInt32(int32_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    void ByteArray::writeFixUint32(uint32_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    void ByteArray::writeFixInt64(int64_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    void ByteArray::writeFixUint64(uint64_t val) {
        if (m_endian != BYTE_ORDER) {
            val = byteSwap(val);
        }
        write(&val, sizeof(val));
    }

    static uint32_t EncodeZigzag32(const int32_t &val) {
        if (val < 0) {
            return ((uint32_t) (-val)) * 2 - 1;
        } else {
            return val * 2;
        }
    }

    void ByteArray::writeInt32(int32_t val) {
        writeUint32(EncodeZigzag32(val));
    }

    void ByteArray::writeUint32(uint32_t val) {
        uint8_t tmp[5];
        uint8_t i = 0;
        while (val >= 0x80) {
            tmp[i++] = (val & 0x7f) | 0x80;
            val >>= 7;
        }
        tmp[i++] = val;
        write(tmp, i);
    }

    static uint64_t EncodeZigzag64(const int64_t &val) {
        if (val < 0) {
            return ((uint64_t) (-val)) * 2 - 1;
        } else {
            return val * 2;
        }
    }

    void ByteArray::writeInt64(int64_t val) {
        writeUint64(EncodeZigzag64(val));
    }

    void ByteArray::writeUint64(uint64_t val) {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (val >= 0x80) {
            tmp[i++] = (val & 0x7f) | 0x80;
            val >>= 7;
        }
        tmp[i++] = val;
        write(tmp, i);
    }

    void ByteArray::writeFloat(float val) {
        uint32_t v;
        memcpy(&v, &val, sizeof(val));
        writeFixUint32(v);
    }

    void ByteArray::writeDouble(double val) {
        uint64_t v;
        memcpy(&v, &val, sizeof(val));
        writeFixUint64(v);
    }

    void ByteArray::writeStringFix16(const std::string &val) {
        writeFixUint16(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringFix32(const std::string &val) {
        writeFixUint32(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringFix64(const std::string &val) {
        writeFixUint64(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringVint(const std::string &val) {
        writeUint64(val.size());
        write(val.c_str(), val.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string &val) {
        write(val.c_str(), val.size());
    }

    int8_t ByteArray::readFixInt8() {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFixUint8() {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

#define XX(type)                    \
    type v;                         \
    read(&v, sizeof(v));            \
    if (m_endian == BYTE_ORDER) {   \
        return v;                   \
    } else {                        \
        return byteSwap(v);         \
    }                               \


    int16_t ByteArray::readFixInt16() {
        XX(int16_t)
    }

    uint16_t ByteArray::readFixUint16() {
        XX(uint16_t)
    }

    int32_t ByteArray::readFixInt32() {
        XX(int32_t)
    }

    uint32_t ByteArray::readFixUint32() {
        XX(uint32_t)
    }

    int64_t ByteArray::readFixInt64() {
        XX(int64_t)
    }

    uint64_t ByteArray::readFixUint64() {
        XX(uint64_t)
    }

#undef XX

    static int32_t DecodeZigzag32(const uint32_t &val) {
        return (val >> 1) ^ -(val & 1);
    }

    int32_t ByteArray::readInt32() {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32() {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7) {
            uint8_t b = readFixUint8();
            if (b < 0x80) {
                result |= ((uint32_t) b) << i;
                break;
            } else {
                result |= ((uint32_t) (b & 0x7f) << i);
            }
        }
        return result;
    }

    static int64_t DecodeZigzag64(const uint64_t &val) {
        return (val >> 1) ^ -(val & 1);
    }

    int64_t ByteArray::readInt64() {
        return DecodeZigzag64(readUint64());
    }

    uint64_t ByteArray::readUint64() {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7) {
            uint8_t b = readFixUint8();
            if (b < 0x80) {
                result |= ((uint64_t) b) << i;
                break;
            } else {
                result |= ((uint64_t) (b & 0x7f) << i);
            }
        }
        return result;
    }

    float ByteArray::readFloat() {
        uint32_t v = readFixUint32();
        float value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    double ByteArray::readDouble() {
        uint64_t v = readFixUint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    std::string ByteArray::readStringFix16() {
        uint16_t len = readFixUint16();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::readStringFix32() {
        uint32_t len = readFixUint32();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::readStringFix64() {
        uint64_t len = readFixUint64();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::readStringVint() {
        uint64_t len = readUint64();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    void ByteArray::clear() {
        m_position = m_size = 0;
        m_capacity = m_baseSize;
        Node *tmp = m_root->next;       // 第一块不删除
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = nullptr;
    }

    void ByteArray::write(const void *buf, size_t size) {
        if (size == 0) {
            return;
        }

        addCapacity(size);

        size_t npos = m_position % m_baseSize;      // 在当前内存块的位置
        size_t ncap = m_cur->size - npos;           // 当前内存快的剩余容量
        size_t bpos = 0;                            // 需要写入的数据当前写了多少

        while (size > 0) {
            if (ncap >= size) {
                memcpy(m_cur->ptr + npos, (const char *) buf + bpos, size);
                if (m_cur->size == (npos + size)) {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy(m_cur->ptr + npos, (const char *) buf + bpos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }

        if (m_position > m_size) {
            m_size = m_position;
        }
    }

    void ByteArray::read(void *buf, size_t size) {
        if (size > getReadSize()) {
            throw std::out_of_range("not enough length");
        }

        size_t npos = m_position % m_baseSize;      // 在当前内存块的位置
        size_t ncap = m_cur->size - npos;           // 当前内存快的剩余容量
        size_t bpos = 0;                            // 需要读取的数据当前读了多少

        while (size > 0) {
            if (ncap >= size) {
                memcpy((char *) buf + bpos, m_cur->ptr + npos, size);
                if (m_cur->size == npos + size) {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy((char *) buf + bpos, m_cur->ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }
        }
    }

    void ByteArray::read(void *buf, size_t size, size_t position) const {
        if (size > getReadSize()) {
            throw std::out_of_range("not enough length");
        }

        size_t npos = position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        size_t bpos = 0;

        Node * cur = m_cur;
        while (size > 0) {
            if (ncap >= size) {
                memcpy((char *) buf + bpos, cur->ptr + npos, size);
                if (cur->size == npos + size) {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy((char *) buf + bpos, cur->ptr + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
        }
    }

    bool ByteArray::writeToFile(const std::string &name) const {
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary);
        if (!ofs) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "ByteArray::writeToFile (" << name
                                                    << ") error, errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return false;
        }

        size_t readSize = getReadSize();
        size_t pos = m_position;
        Node *cur = m_cur;

        while (readSize > 0) {
            size_t diff = pos % m_baseSize;
            ssize_t len = (readSize > m_baseSize ? m_baseSize : readSize) - diff;
            ofs.write(cur->ptr + diff, len);
            cur = cur->next;
            pos += len;
            readSize -= len;
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string &name) {
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);
        if (!ifs) {
            LUWU_LOG_ERROR(LUWU_LOG_NAME("system")) << "ByteArray::readFromFile (" << name
                                                    << ") error, errno=" << errno
                                                    << " errstr=" << strerror(errno);
            return false;
        }

        std::shared_ptr<char> buf(new char[m_baseSize], [](const char *ptr) { delete[] ptr; });
        while (!ifs.eof()) {
            ifs.read(buf.get(), m_baseSize);
            write(buf.get(), ifs.gcount());
        }
        return true;
    }

    void ByteArray::setPosition(size_t pos) {
        if (pos > m_capacity) {
            throw std::out_of_range("set_position out of range");
        }
        m_position = pos;
        if (m_position > m_size) {
            m_size = m_position;
        }
        m_cur = m_root;
        while (pos > m_cur->size) {
            pos -= m_cur->size;
            m_cur = m_cur->next;
        }
        if (pos == m_cur->size) {
            m_cur = m_cur->next;
        }
    }

    void ByteArray::setLittleEndian(bool v) {
        if (v) {
            m_endian = LITTLE_ENDIAN;
        } else {
            m_endian = BIG_ENDIAN;
        }
    }

    void ByteArray::addCapacity(size_t size) {
        size_t old = getCapacity();
        if (old >= size) {
            return;
        }

        size = size - old;
        size_t count = size / m_baseSize;
        if (size % m_baseSize) {
            ++count;
        }

        Node *tmp = m_root;
        while (tmp->next) {
            tmp = tmp->next;
        }

        Node *first = nullptr;
        for (size_t i = 0; i < count; ++i) {
            tmp->next = new Node(m_baseSize);
            if (first == nullptr) {
                first = tmp->next;
            }
            tmp = tmp->next;
            m_capacity += m_baseSize;
        }

        if (old == 0) {
            m_cur = first;
        }
    }

    std::string ByteArray::toString() const {
        std::string str;
        str.resize(getReadSize());
        if (str.empty()) {
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len) {
        len = len > getReadSize() ? getReadSize() : len;
        if(len == 0) {
            return 0;
        }

        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        struct iovec iov{};
        Node* cur = m_cur;

        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) {
        len = len > getReadSize() ? getReadSize() : len;
        if(len == 0) {
            return 0;
        }

        uint64_t size = len;

        size_t npos = position % m_baseSize;
        size_t count = position / m_baseSize;
        Node* cur = m_root;
        while(count > 0) {
            cur = cur->next;
            --count;
        }

        size_t ncap = cur->size - npos;
        struct iovec iov{};
        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
        if(len == 0) {
            return 0;
        }
        addCapacity(len);
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        struct iovec iov{};
        Node* cur = m_cur;
        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;

                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }
}