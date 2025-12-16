/*
 * =========================
 * Configuration file reader.
 * =========================
 */
#define _GNU_SOURCE
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "config.h"
#include "ipnetwork.h"

// Like strsep, but skips everything between [ and ].
static char *strsep_net(char **line, char sep) {
  if (*line == NULL) {
    return NULL;
  }
  char *l = *line;
  char *l2 = *line;
  while (*l) {
    if (*l == '[') {
      while (*l && *l != ']') {
        l++;
      }
      continue;
    }
    if (*l == sep) {
      *l = 0;
      *line = l + 1;
      return l2;
    }
    l++;
  }
  *line = NULL;
  return NULL;
}

// Read one config line.
static struct Config *config_parse(char *line) {
  char *orig_line = strdup(line);
  struct IPNetwork *ipnet = NULL;

  // Parse.
  char *call = trim(strsep(&line, ":"));
  char *net = trim(strsep_net(&line, ':'));
  char *cong = trim(strsep(&line, "\n"));

  // Parse network.
  if (line) {
    ipnet = ipnetwork_parse(net);
  }

  if (
       line == NULL ||
       (strcmp(call, "connect") != 0 && strcmp(call, "accept") != 0) ||
       ipnet == NULL ||
       (strcmp(cong, "bbr") != 0 && strcmp(cong, "cubic") != 0 && strcmp(cong, "reno") != 0)
    ) {
    errorf("cannot parse config: %s", trim(orig_line));
    free(orig_line);
    return NULL;
  }
  free(orig_line);

  // Fill out struct and return.
  struct Config *cfgline = malloc(sizeof(struct Config));
  cfgline->call = strdup(call);
  cfgline->net = ipnet;
  cfgline->cong = strdup(cong);
  cfgline->next = NULL;
  return cfgline;
}

static struct Config *config_reader(char *filename) {
  // Open file.
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    errorf("%s: %s", filename, strerror(errno));
    return NULL;
  }

  struct Config *config = NULL;
  struct Config **next = NULL;
  char buf[1024];

  // Read file line by line.
  while (fgets(buf, sizeof(buf), fp)) {
    if (buf[0] == '\n' || buf[0] == '#') {
      continue;
    }
    struct Config *c = config_parse(buf);
    if (c) {
      if (!config) {
        config = c;
      }
      if (next) {
        *next = c;
      }
      next = &(c->next);
    }
  }

  return config;
}

// Free configuration.
static void config_free(struct Config *cfgline) {
  while (cfgline) {
    struct Config *next = cfgline->next;
    free(cfgline->call);
    free(cfgline->cong);
    ipnetwork_free(cfgline->net);
    free(cfgline);
    cfgline = next;
  }
}

// Get the config.
struct Config *config_get() {
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  static time_t mtime = 0;
  static struct Config *config = NULL;
  static char *envfile = NULL;
  struct stat st;

  pthread_mutex_lock(&mutex);

  if (envfile == NULL) {
    envfile = getenv(CFG_ENV);
  }
  char *cfgfile = envfile ? envfile : CFG_FILE;

  if (stat(cfgfile, &st) < 0) {
    if (mtime != -1) {
      errorf("%s: %s", CFG_FILE, strerror(errno));
      mtime = -1;
    }
    goto out;
  }

  if (mtime == st.st_mtime) {
    goto out;
  }
  mtime = st.st_mtime;

  struct Config *newconfig = config_reader(cfgfile);
  if (newconfig) {
    if (config) {
      config_free(config);
    }
    config = newconfig;
  }

out:
  pthread_mutex_unlock(&mutex);
  return config;
}

// Match the config. Last match is returned.
struct Config *config_match(char *call, const struct sockaddr *addr) {
  struct Config *config = config_get();
  if (!config) {
    return NULL;
  }

  // If it's a mapped v4 address we got, transform it into ipv4.
  struct sockaddr_storage buf;
  addr = mapped_ipv4_to_real_ipv4(addr, &buf);

  struct Config *result = NULL;
  while (config) {
    if (strcmp(config->call, call) == 0 && ipnetwork_match(config->net, addr)) {
      result = config;
    }
    config = config->next;
  }

  return result;
}

void config_print(struct Config *config) {
  while (config) {
    printf("%s: %s: %s\n", config->call, ipnetwork_str(config->net), config->cong);
    config = config->next;
  }
}
