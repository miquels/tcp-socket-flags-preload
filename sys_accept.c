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

typedef int (*acceptfunc_t)(int, struct sockaddr *, socklen_t *);
typedef int (*accept4func_t)(int, struct sockaddr *, socklen_t *, int);

int accept(int socket, struct sockaddr *addr, socklen_t *addrlen) {
  static _Atomic(acceptfunc_t) orig_accept = ATOMIC_VAR_INIT(NULL);

  // Atomically load the original accept function pointer.
  acceptfunc_t acceptfunc = atomic_load_explicit(&orig_accept, memory_order_acquire);

  // Look it up if it's NULL.
  if (!acceptfunc) {
    acceptfunc = (acceptfunc_t)dlsym(RTLD_NEXT, "accept");
    if (!acceptfunc) {
      errno = ENOSYS;
      return -1;
    }
    // Store it atomically.
    atomic_store_explicit(&orig_accept, acceptfunc, memory_order_release);
  }

  // We need the peer's address.
  struct sockaddr_storage ss;
  socklen_t ss_len = sizeof(ss);
  if (!addr) {
    addr = (struct sockaddr *)&ss;
    addrlen = &ss_len;
  }

  // call the original accept function
  int res =  (*acceptfunc)(socket, addr, addrlen);
  if (res != 0) {
    return res;
  }

  // See if it matches the config.
  struct Config *config = config_match("accept", addr);
  if (config) {
    // Set congestion protocol.
    int len = strlen(config->cong);
    if (setsockopt(socket, IPPROTO_TCP, TCP_CONGESTION, config->cong, len) < 0) {
      errorf("accept: could not set congestion control to %s", config->cong);
    }
  }

  return 0;
}

int accept4(int socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
  static _Atomic(accept4func_t) orig_accept4 = ATOMIC_VAR_INIT(NULL);

  // Atomically load the original accept4 function pointer.
  accept4func_t acceptfunc = atomic_load_explicit(&orig_accept4, memory_order_acquire);

  // Look it up if it's NULL.
  if (!acceptfunc) {
    acceptfunc = (accept4func_t)dlsym(RTLD_NEXT, "accept4");
    if (!acceptfunc) {
      errno = ENOSYS;
      return -1;
    }
    // Store it atomically.
    atomic_store_explicit(&orig_accept4, acceptfunc, memory_order_release);
  }

  // We need the peer's address.
  struct sockaddr_storage ss;
  socklen_t ss_len = sizeof(ss);
  if (!addr) {
    addr = (struct sockaddr *)&ss;
    addrlen = &ss_len;
  }

  // call the original accept4 function
  int res =  (*acceptfunc)(socket, addr, addrlen, flags);
  if (res != 0) {
    return res;
  }

  // See if it matches the config.
  struct Config *config = config_match("accept", addr);
  if (config) {
    // Set congestion protocol.
    int len = strlen(config->cong);
    if (setsockopt(socket, IPPROTO_TCP, TCP_CONGESTION, config->cong, len) < 0) {
      errorf("accept4: could not set congestion control to %s", config->cong);
    }
  }

  return 0;
}
