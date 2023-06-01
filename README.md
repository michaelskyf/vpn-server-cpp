# vpn-server-cpp
## _Experimental VPN server written in C++ using boost.asio with coroutines_

## Build
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
