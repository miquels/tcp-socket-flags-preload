#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"

// helper, since we don't have u128 literals.
u128 make_u128(unsigned long long part1, unsigned long long part2) {
  return ((u128)part1<<64) | part2;
}

// See if the environment variable TCPSOCKETFLAGS_DEBUG was set.
int tcpsocketflags_debug() {
  static long long debug = -1;
  if (debug == -1) {
    debug = getenv("TCPSOCKETFLAGS_DEBUG") ? 1 : 0;
  }
  return debug;
}

// IPv4 or IPv6 address to 128 bit little-endian integer.
u128 sockaddr_to_uint128(const struct sockaddr *sa) {
  u128 res = 0;
  unsigned char *data;
  int len;

  if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
    data = sin6->sin6_addr.s6_addr;
    len = 16;
  } else if (sa->sa_family == AF_INET) {
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
    data = (unsigned char *)&sin->sin_addr.s_addr;
    len = 4;
  } else {
    return 0;
  }

  for (int i = 0; i < len; i++) {
    res = (res << 8) | data[i];
  }
  return res;
}

// If the socket address is ipv4-mapped ipv6, transmogrify it into plain ipv4.
const struct sockaddr *mapped_ipv4_to_real_ipv4(const struct sockaddr *sa, struct sockaddr_storage *buf) {
  // Check it's IPv6.
  if (sa->sa_family != AF_INET6) {
    return sa;
  }
  // Check it's mapped ipv4.
  u128 addr = sockaddr_to_uint128(sa);
  if ((addr & make_u128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF00000000)) != (u128)0xFFFF00000000) {
    return sa;
  }
  // transmogrify.
  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
  struct sockaddr_in sin4 = { .sin_family = AF_INET, .sin_port = sin6->sin6_port };
  memcpy(&sin4.sin_addr.s_addr, sin6->sin6_addr.s6_addr + 12, 4);
  memcpy(buf, &sin4, sizeof(sin4));
  return (struct sockaddr *)buf;
}

char *trim(char *s) {
  if (s == NULL) {
    return s;
  }
  while (isspace(*s)) {
    s++;
  }
  if (*s != 0) {
    char *e = s + strlen(s) - 1;
    while (e >= s && isspace(*e)) {
      *e-- = 0;
    }
  }
  return s;
}

void errorf(char *format, ...) {
  char buf[1024];
  strncpy(buf, "libtcpsocketflags: ", sizeof(buf));

  va_list args;
  va_start(args, format);
  vsnprintf(buf + 19, sizeof(buf) - 19, format, args);

  if (tcpsocketflags_debug()) {
    fprintf(stderr, "%s\n", buf);
  }
  syslog(LOG_WARNING, "%s", buf);
}
