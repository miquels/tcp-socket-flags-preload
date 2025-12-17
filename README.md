# tcp-socket-flags-preload

A Linux-based experimental `LD_PRELOAD` library to tweak TCP flags and settings for a specific process,
system call, and IP address.

The library intercepts the `connect` and the `accept` systemcalls.

For connect, it first checks if the socket is a TCP socket, then if the destination address matches any networks defined in the configuration file.

For accept, if the accept succeeds, it checks if the remote address matches any networks defined in the configuration file.

On a match, the settings from the configuration file are applied to the socket. Right now, the only thing you can set is the TCP congestion control algorithm, `reno`, `cubic` or `bbr`.

## Example config file.
This config file enables BBR for all peers, with two exceptions where it's cubic.

```
# outgoing.
connect: 0.0.0.0/0: bbr
connect: [::]/0: bbr
connect: 10.200.0.0/16: cubic
connect: [2001:980:400::]/48: cubic
# incoming.
accept: 0.0.0.0/0: bbr
accept: [::]/0: bbr
accept: 10.200.0.0/16: cubic
accept: [2001:980:400::]/48: cubic
```

The default config file is `/etc/tcp_socket_flags.cfg`, but it can be overridden by an environment variable.

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
`LD_PRELOAD=./libtcpsocketflags.so TCPSOCKETFLAGS_CFG=./tcp_socket_flags.cfg curl https://gitlab.com/`

## Debug
`LD_PRELOAD=./libtcpsocketflags.so TCPSOCKETFLAGS_CFG=./tcp_socket_flags.cfg TCPSOCKETFLAGS_DEBUG=1 strace -s128 -e trace=network curl https://gitlab.com/`

```
socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP) = 3
socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) = 3
setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0
setsockopt(3, SOL_SOCKET, SO_KEEPALIVE, [1], 4) = 0
setsockopt(3, SOL_TCP, TCP_KEEPIDLE, [60], 4) = 0
setsockopt(3, SOL_TCP, TCP_KEEPINTVL, [60], 4) = 0
connect(3, {sa_family=AF_INET, sin_port=htons(443), sin_addr=inet_addr("172.65.251.78")}, 16) = -1 EINPROGRESS (Operation now in progress)
getsockopt(3, SOL_SOCKET, SO_TYPE, [1], [4]) = 0
setsockopt(3, SOL_TCP, TCP_CONGESTION, "bbr", 3) = 0
getsockopt(3, SOL_SOCKET, SO_ERROR, [0], [4]) = 0
getpeername(3, {sa_family=AF_INET, sin_port=htons(443), sin_addr=inet_addr("172.65.251.78")}, [128 => 16]) = 0
getsockname(3, {sa_family=AF_INET, sin_port=htons(45974), sin_addr=inet_addr("100.64.10.43")}, [128 => 16]) = 0
setsockopt(3, SOL_TCP, TCP_ULP, [7564404], 4) = 0
<html><body>You are being <a href="https://gitlab.com/users/sign_in">redirected</a>.</body></html>+++ exited with 0 +++
```

## Copyright and License
Copyright (C) Miquel van Smoorenburg.

Licensed under either of

- Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0
- MIT license, http://opensource.org/licenses/MIT

at your option.

## Contribution
Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.
