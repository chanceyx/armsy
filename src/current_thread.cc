#include "current_thread.h"

namespace CurrentThread {
// __thread : each thread has its own t_cachedTid.
__thread int t_cached_tid = 0;

void cacheTid() {
  if (t_cached_tid == 0) {
    t_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
  }
}
}  // namespace CurrentThread
