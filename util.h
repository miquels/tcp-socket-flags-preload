#ifndef LIB_UTIL_H
#define LIB_UTIL_H

typedef unsigned __int128 u128;

extern u128 make_u128(unsigned long long part1, unsigned long long part2);
extern int tcpsocketflags_debug();
extern u128 sockaddr_to_uint128(const struct sockaddr *sa);
extern const struct sockaddr *mapped_ipv4_to_real_ipv4(const struct sockaddr *sa, struct sockaddr_storage *buf);
extern char *trim(char *s);
void errorf(char *format, ...);

#endif /* LIB_UTIL_H */
