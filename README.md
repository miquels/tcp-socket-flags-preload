# tcp-socket-flags-preload

A Linux-based experimental `LD_PRELOAD` library to tweak TCP flags and settings for a specific process,
system call, and IP address.

The library intercepts the `connect` and the `accept` systemcalls.

For connect, it first checks if the socket is a TCP socket, then if the destination address matches any networks defined in the configuration file.

For accept, if the accept succeeds, it checks if the remote address matches any networks defined in the configuration file.

On a match, the settings from the configuration file are applied to the socket. Right now, the only thing you can set is the TCP congestion control algorithm, `reno`, `cubic` or `bbr`.

## Example config file.
This config file enables BBR on all destinations, with two exceptions where it's cubic.

```
# config file.
connect: 0.0.0.0/0: bbr
connect: [::]/0: bbr
connect: 10.200.0.0/16: cubic
connect: [2001:980:400::]/48: cubic
```

The default config file is `/etc/tcp_socket_flags.cfg`, the environment variable `TCPSOCKETFLAGS_CFG`
can be set to use read it from a different location.

## Warnings and errors.
Are logged to syslog, and also to stdout if an environment variable is set.

## Environment variables

`TCPSOCKETFLAGS_CFG`: override default configuration file location
`TCPSOCKETFLAGS_DEBUG`: if set (to anything) logs any warnings/errors to stdout.

## Build Instructions
```
make
```

## Usage
```
LD_PRELOAD=./libtcpsocketflags.so TCPSOCKETFLAGS_CFG=./tcp_socket_flags.cfg curl www.google.com`
```

## Debug
```
LD_PRELOAD=./libtcpsocketflags.so TCPSOCKETFLAGS_CFG=./tcp_socket_flags.cfg strace -e trace=network curl -I www.google.com
```

```
[root@foo bbr-preload]# LD_PRELOAD=./libtcpsocketflags.so strace -e trace=network curl -I www.google.com
socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP) = 3
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 3
setsockopt(3, SOL_TCP, TCP_CONGESTION, [7496290], 4) = 0
setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0
setsockopt(3, SOL_SOCKET, SO_KEEPALIVE, [1], 4) = 0
setsockopt(3, SOL_TCP, TCP_KEEPIDLE, [60], 4) = 0
setsockopt(3, SOL_TCP, TCP_KEEPINTVL, [60], 4) = 0
connect(3, {sa_family=AF_INET, sin_port=htons(80), sin_addr=inet_addr("142.250.188.228")}, 16) = -1 EINPROGRESS (Operation now in progress)
getsockopt(3, SOL_SOCKET, SO_ERROR, [0], [4]) = 0
getpeername(3, {sa_family=AF_INET, sin_port=htons(80), sin_addr=inet_addr("142.250.188.228")}, [128 => 16]) = 0
getsockname(3, {sa_family=AF_INET, sin_port=htons(47818), sin_addr=inet_addr("10.0.0.14")}, [128 => 16]) = 0
sendto(3, "HEAD / HTTP/1.1\r\nHost: www.googl"..., 79, MSG_NOSIGNAL, NULL, 0) = 79
recvfrom(3, "HTTP/1.1 200 OK\r\nContent-Type: t"..., 102400, 0, NULL, NULL) = 1125
HTTP/1.1 200 OK
```
