# vpn-server-cpp
## _Experimental VPN server written in C++ using boost.asio with coroutines_

## Building
⚠️ At the moment only Arch Linux is supported. ⚠️

Compiling:
```bash

pacman -S gcc meson boost

meson build

cd build
ninja all

```

Testing:
```bash

ninja test

```

The executable should be present at `vpn-server-cpp/build/src/vpn`

## Running
Before running make sure that you have masquarade and packet forwarding set-up
on your server.
