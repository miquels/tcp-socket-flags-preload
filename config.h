#ifndef LIB_CONFIG_H
#define LIB_CONFIG_H

#include <netdb.h>
#include "ipnetwork.h"

#define CFG_FILE "/etc/tcp_socket_flags.cfg"
#define CFG_ENV "TCPSOCKETFLAGS_CFG"

// One line in the config file.
struct Config {
  char *call;
  struct IPNetwork *net;
  char *cong;
  struct Config *next;
};

extern struct Config *config_get();
extern struct Config *config_match(char *call, const struct sockaddr *addr);
extern void config_print(struct Config *config);

#endif /* LIB_CONFIG_H */
