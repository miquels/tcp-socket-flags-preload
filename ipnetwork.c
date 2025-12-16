/*
 * ================================
 * Network range parser and matcher.
 * ================================
 *
 */
#define _GNU_SOURCE
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "util.h"
#include "ipnetwork.h"

// Parse network range.
struct IPNetwork *ipnetwork_parse(char *net) {
  struct addrinfo          hints;
  struct addrinfo          *result;
  char                     *slash;

  // Find slash and set it to 0.
  if ((slash = strrchr(net, '/')) != NULL) {
    *slash = 0;
  }

  // Strip [ and ]
  if (*net == '[' && net[strlen(net) - 1] == ']') {
    net[strlen(net) - 1] = 0;
    net += 1;
  }

  // Get the address.
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_NUMERICHOST;

  int s = getaddrinfo(net, NULL, &hints, &result);
  if (s != 0) {
    if (tcpsocketflags_debug()) {
      if (slash) {
        *slash = '/';
      }
      errorf("ipnetwork: %s: %s%s\n", net, slash ? slash : "", gai_strerror(s));
    }
    return NULL;
  }

  // Get masklen.
  int maxmasklen = result->ai_family == AF_INET ? 32 : 128;
  int masklen = maxmasklen;
  if (slash) {
    char *e;
    masklen = strtol(slash + 1, &e, 10);
    if (*(slash + 1) == 0 || *e != 0 || masklen < 0 || masklen > maxmasklen) {
      if (tcpsocketflags_debug()) {
	*slash = '/';
        errorf("ipnetwork: %s: invalid mask\n", net);
      }
      return NULL;
    }
  }

  // Return new IPNetwork.
  struct IPNetwork *n = (struct IPNetwork *)malloc(sizeof(struct IPNetwork));
  if (result->ai_family == AF_INET) {
    memcpy(&n->addr, result->ai_addr, sizeof(struct sockaddr_in));
  } else {
    memcpy(&n->addr, result->ai_addr, sizeof(struct sockaddr_in6));
  }
  n->masklen = masklen;
  return n;
}

// See if the IP address is part of the IP network.
int ipnetwork_match(struct IPNetwork *net, const struct sockaddr *sa) {
  if (net->addr.ss_family != sa->sa_family) {
    return 0;
  }
  if (net->masklen == 0) {
    return 1;
  }
  u128 n = sockaddr_to_uint128((const struct sockaddr *)&net->addr);
  u128 a = sockaddr_to_uint128(sa);
  int bit = (sa->sa_family == AF_INET6 ? 128 : 32) - net->masklen;
  u128 mask = ~(((u128)1 << bit) - 1);
  return (n & mask) == (a & mask);
}

char *ipnetwork_str(struct IPNetwork *net) {
  char name[INET6_ADDRSTRLEN + 4], *res;
  getnameinfo((struct sockaddr *)&net->addr, sizeof(net->addr), name, sizeof(name), NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
  if (net->addr.ss_family == AF_INET6) {
    asprintf(&res, "[%s]/%d", name, net->masklen);
  } else {
    asprintf(&res, "%s/%d", name, net->masklen);
  }
  return res;
}

// Free the object.
void ipnetwork_free(struct IPNetwork *net) {
  free(net);
}
