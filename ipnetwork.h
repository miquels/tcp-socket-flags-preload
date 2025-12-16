#ifndef LIB_IPNETWORK_H
#define LIB_IPNETWORK_H

#include <sys/socket.h>

struct IPNetwork {
  struct sockaddr_storage addr;
  int masklen;
};

extern struct IPNetwork *ipnetwork_parse(char *net);
extern int ipnetwork_match(struct IPNetwork *net, const struct sockaddr *sa);
extern char *ipnetwork_str(struct IPNetwork *net);
extern void ipnetwork_free(struct IPNetwork *net);

#endif /* LIB_IPNETWORK_H */
