#!/bin/bash

set -e
set -x

# upower 1.90.9 (power/battery D-Bus service + libupower-glib +
# UPowerGlib-1.0 gir for gnome-shell's power quick-settings).
#
# Flags (checked against meson_options.txt):
# -D introspection=enabled: UPowerGlib-1.0 typelib needed by gnome-shell.
# -D man=false -Dgtk-doc=false: no docs.
# -D idevice=disabled: no libimobiledevice in the chroot.
# -D systemdsystemunitdir=no: no systemd (elogind chroot); "no" disables.
# -D os_backend=auto → linux (libgudev is installed).
# -D polkit=auto → found (daemon policy actions).
# The daemon wants an 'upower' user? 1.90.x runs as root or via systemd user
# 'upower' only through the packaged unit file; no build-time user check.
cd /sources
rm -rf pizlonated-upower
tar -xf pizlonated-upower.tar.gz
cd pizlonated-upower
mkdir -v filc-build
cd filc-build
meson setup ..                   \
      --prefix=/usr              \
      --buildtype=debugoptimized \
      -Dintrospection=enabled    \
      -Dman=false                \
      -Dgtk-doc=false            \
      -Didevice=disabled         \
      -Dsystemdsystemunitdir=no
ninja
ninja install
cd ../..
rm -rf pizlonated-upower
hash -r
