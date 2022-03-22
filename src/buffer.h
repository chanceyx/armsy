#pragma once

#include <algorithm>
#include <stddef.h>
#include <string>
#include <vector>

class Buffer
{
public:
    static const size_t KCheapPrepend = 0;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t init_size = kInitialSize)
        : buffer_(KCheapPrepend + init_size),
          read_index_(KCheapPrepend),
          write_index_(KCheapPrepend)
    {
    }

    // Return readable bytes of buffer.
    size_t readableBytes() const { return write_index_ - read_index_; }

    // Return writable bytes of buffer.
    size_t writableBytes() const { return buffer_.capacity() - write_index_; }

    // Return prependable bytes of buffer.
    size_t prependableBytes() const { return read_index_; }

    // Return the ptr of the read_index_ in buffer.
    const char *reader() const { return begin() + read_index_; }

    // Return the ptr of write_index_ in buffer.
    const char *writer() const { return begin() + write_index_; }
    char *writer() { return begin() + write_index_; }

    // Change buffer state after retrieve data.
    void pushState(size_t len);

    // Check buffer state and make sure buffer has enough space to put data.
    void checkBuffer(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // Put data into the buffer.
    void put(const char *data, size_t len);

    // Get data from buffer.
    std::string get(size_t len);
    std::string getAll();

    ssize_t read(int fd, int *save_errno);
    ssize_t write(int fd, int *save_errno);

private:
    // Return the ptr of the begin_index in buffer.
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    // Make sure buffer has enough space to put data.
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
};