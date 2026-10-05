#!/bin/sh
# Read-only diagnostics. Run inside the same PortMaster environment as the game.
# This intentionally avoids dumping environment variables or personal save names.
printf 'Architecture: '; uname -m
printf 'Kernel: '; uname -r
printf 'C library: '; getconf GNU_LIBC_VERSION 2>/dev/null || true
if command -v ldd >/dev/null 2>&1; then ldd --version 2>&1 | head -n 1; fi
if [ -r /sys/class/graphics/fb0/virtual_size ]; then
  printf 'Framebuffer: '; cat /sys/class/graphics/fb0/virtual_size
fi
if [ -r /proc/bus/input/devices ]; then cat /proc/bus/input/devices; fi
if [ -r /etc/os-release ]; then cat /etc/os-release; fi
if [ -x ./openrct2 ]; then
  ./openrct2 --version
  ldd ./openrct2 2>&1 || true
fi
