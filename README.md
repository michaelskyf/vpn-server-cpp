# vpn-server-cpp
## _Experimental VPN server written in C++ using boost.asio with coroutines_

## Build
⚠️ At the moment only Arch Linux is supported. ⚠️

Compiling:
```console

user@archlinux:~/vpn-server-cpp$ pacman -S gcc meson boost

user@archlinux:~/vpn-server-cpp$ meson build

user@archlinux:~/vpn-server-cpp/build$ cd build
user@archlinux:~/vpn-server-cpp/build$ ninja all

```

Testing:
```console

user@archlinux:~/vpn-server-cpp/build$ ninja test

```

The executable should be present at `vpn-server-cpp/build/src/vpn`