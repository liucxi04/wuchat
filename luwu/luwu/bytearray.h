//
// Created by liucxi on 2022/6/3.
//

#ifndef LUWU_BYTEARRAY_H
#define LUWU_BYTEARRAY_H

#include <memory>
#include <string>
#include <vector>
#include <sys/socket.h>

namespace liucxi {
    /**
     * @brief 二进制数组，提供基础的序列化、反序列化功能
     */
    class ByteArray {
    public:
        typedef std::shared_ptr<ByteArray> ptr;

        struct Node {
            Node();

            /**
             * @brief 构建指定大小的内存块
             */
            explicit Node(size_t s);

            ~Node();

            char *ptr;      /// 内存块地址指针
            Node *next;     /// 下一个内存块地址
            size_t size;    /// 内存块大小
        };

        explicit ByteArray(size_t base_size = 4096);

        ~ByteArray();

        /**
         * @brief 写入固定长度的数据
         */
        void writeFixInt8(int8_t val);

        void writeFixUint8(uint8_t val);

        void writeFixInt16(int16_t val);

        void writeFixUint16(uint16_t val);

        void writeFixInt32(int32_t val);

        void writeFixUint32(uint32_t val);

        void writeFixInt64(int64_t val);

        void writeFixUint64(uint64_t val);

        /**
         * @brief 写入数据，按实际占用内存写入
         */
        void writeInt32(int32_t val);

        void writeUint32(uint32_t val);

        void writeInt64(int64_t val);

        void writeUint64(uint64_t val);

        void writeFloat(float val);

        void writeDouble(double val);

        /**
         * @brief 写入字符串，写之前还会写入一个长度
         */
        void writeStringFix16(const std::string &val);

        void writeStringFix32(const std::string &val);

        void writeStringFix64(const std::string &val);

        void writeStringVint(const std::string &val);

        /**
         * @brief 写入字符串，不写入长度
         */
        void writeStringWithoutLength(const std::string &val);

        /**
         * @brief 读取固定长度的数据
         */
        int8_t readFixInt8();

        uint8_t readFixUint8();

        int16_t readFixInt16();

        uint16_t readFixUint16();

        int32_t readFixInt32();

        uint32_t readFixUint32();

        int64_t readFixInt64();

        uint64_t readFixUint64();

        /**
         * @brief 读取数据，按实际占用内存读取
         */
        int32_t readInt32();

        uint32_t readUint32();

        int64_t readInt64();

        uint64_t readUint64();

        float readFloat();

        double readDouble();

        /**
         * @brief 读取字符串，读之前还会读取一个长度
         */
        std::string readStringFix16();

        std::string readStringFix32();

        std::string readStringFix64();

        std::string readStringVint();

        /**
         * @brief 清空所有数据
         */
        void clear();

        /**
         * @brief 写入数据
         */
        void write(const void *buf, size_t size);

        /**
         * @brief 读取数据
         */
        void read(void *buf, size_t size);

        /**
         * @brief 从指定位置读数据，不改变位置指针
         */
        void read(void *buf, size_t size, size_t position) const;

        bool writeToFile(const std::string &name) const;

        bool readFromFile(const std::string &name);

        size_t getBaseSize() const { return m_baseSize; }

        size_t getReadSize() const { return m_size - m_position; }

        size_t getPosition() const { return m_position; }

        void setPosition(size_t pos);

        bool isLittleEndian() const { return m_endian == LITTLE_ENDIAN; }

        void setLittleEndian(bool v);

        size_t getSize() const { return m_size; }

        std::string toString() const;

        uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len = ~0ull);

        uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position);

        uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

    private:
        void addCapacity(size_t size);

        size_t getCapacity() const { return m_capacity - m_position; }

    private:
        size_t m_baseSize;      /// 内存块的大小
        size_t m_position;      /// 当前操作位置
        size_t m_capacity;      /// 当前的总容量
        size_t m_size;          /// 当前数据的大小
        uint32_t m_endian;      /// 字节序，默认大端
        Node *m_root;           /// 第一个内存块指针
        Node *m_cur;            /// 当前内存块指针
    };
}

#endif //LUWU_BYTEARRAY_H
