#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "config.h"
#include "util.h"

typedef int (*connectfunc_t)(int, const struct sockaddr *, socklen_t);

int connect(int socket, const struct sockaddr *addr, socklen_t addrlen) {
  static _Atomic(connectfunc_t) orig_connect = ATOMIC_VAR_INIT(NULL);

  // Atomically load the original connect function pointer.
  connectfunc_t connectfunc = atomic_load_explicit(&orig_connect, memory_order_acquire);

  // Look it up if it's NULL.
  if (!connectfunc) {
    connectfunc = (connectfunc_t)dlsym(RTLD_NEXT, "connect");
    if (!connectfunc) {
      errno = ENOSYS;
      return -1;
    }
    // Store it atomically.
    atomic_store_explicit(&orig_connect, connectfunc, memory_order_release);
  }

  // call the original connect function
  int res =  (*connectfunc)(socket, addr, addrlen);
  if (res != 0 && errno != EINPROGRESS) {
    return res;
  }
  int connect_errno = errno;

  // See if it matches the config.
  struct Config *config = config_match("connect", addr);
  if (config) {
    int type;
    socklen_t len = sizeof(type);
    // Check for TCP.
    if (getsockopt(socket, SOL_SOCKET, SO_TYPE, &type, &len) == 0 && type == SOCK_STREAM) {
      // Set congestion protocol.
      len = strlen(config->cong);
      if (setsockopt(socket, IPPROTO_TCP, TCP_CONGESTION, config->cong, len) < 0) {
        errorf("connect: could not set congestion control to %s", config->cong);
      }
    }
  }

  errno = connect_errno;
  return res;
}
