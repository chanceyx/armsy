#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
// __thread : each thread has its own t_cachedTid.
extern __thread int t_cached_tid;

// Use syscall(SYS_gettid) to get thread id and cache it in t_cachedTid.
void cacheTid();

// Get the thread id of its own.
inline int tid() {
  if (__builtin_expect(t_cached_tid == 0, 0)) {
    cacheTid();
  }
  return t_cached_tid;
}
}  // namespace CurrentThread