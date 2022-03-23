#include "buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

void Buffer::pushState(size_t len) {
  if (len < readableBytes()) {
    read_index_ += len;
  } else {
    read_index_ = KCheapPrepend;
    write_index_ = KCheapPrepend;
  }
}

std::string Buffer::get(size_t len) {
  std::string result(reader(), len);
  pushState(len);
  return result;
}

void Buffer::put(const char *data, size_t len) {
  // make sure enough space
  checkBuffer(len);
  std::copy(data, data + len, writer());
  write_index_ += len;
}

std::string Buffer::getAll() { return get(readableBytes()); }

void Buffer::makeSpace(size_t len) {
  // if buffer size is not enough for put data, resize the buffer
  if (writableBytes() + prependableBytes() < len + KCheapPrepend) {
    buffer_.resize(write_index_ + len);
  } else  // move data backward to make space
  {
    size_t readable_size = readableBytes();
    std::copy(begin() + read_index_, begin() + write_index_,
              begin() + KCheapPrepend);
    read_index_ = KCheapPrepend;
    write_index_ = read_index_ + readable_size;
  }
}

ssize_t Buffer::read(int fd, int *save_errno) {
  // An extrabuf on stack incase the buffer space is not enough
  // to put the whole data read from fd.
  // more details: man readv
  char extrabuf[65536] = {0};  // 64KB

  /*
  struct iovec {
      ptr_t iov_base;
      size_t iov_len;
  };
  */

  // two main readbuf
  struct iovec vec[2];
  const size_t writable_bytes = writableBytes();

  // space in buffer
  vec[0].iov_base = begin() + write_index_;
  vec[0].iov_len = writable_bytes;

  // space in stack
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;

  const int iovcnt = (writable_bytes < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);

  if (n < 0) {
    *save_errno = errno;
  } else if (n <= writable_bytes) {
    write_index_ += n;
  } else {
    write_index_ = buffer_.size();
    put(extrabuf, n - writable_bytes);
  }
  return n;
}

ssize_t Buffer::write(int fd, int *save_errno) {
  ssize_t n = ::write(fd, reader(), readableBytes());
  if (n < 0) {
    *save_errno = errno;
  }
  return n;
}