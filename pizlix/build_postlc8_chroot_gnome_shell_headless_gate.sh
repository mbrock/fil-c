#!/bin/bash
# Headless gnome-shell gate (final form): system bus + private session bus,
# shell runs headless with a virtual monitor, must stay alive and own
# org.gnome.Shell + org.gnome.Shell.Extensions.
export GI_TYPELIB_PATH=/usr/lib/gnome-shell:/usr/lib/mutter-14
export LD_LIBRARY_PATH=/usr/lib/gnome-shell:/usr/lib/mutter-14
export NO_AT_BRIDGE=1
export LC_ALL=C
export XDG_RUNTIME_DIR=/tmp/xdgrt
mkdir -p $XDG_RUNTIME_DIR && chmod 700 $XDG_RUNTIME_DIR
mkdir -p /run/dbus
rm -f /run/dbus/pid /run/dbus/system_bus_socket

dbus-daemon --system --fork 2>/dev/null
dbus-run-session -- bash -c '
  gnome-shell --wayland --headless --virtual-monitor 1024x768 > /tmp/gshell.out 2>&1 &
  SHELL_PID=$!
  sleep 40
  echo "=== is gnome-shell alive after 40s? ==="
  if kill -0 $SHELL_PID 2>/dev/null; then echo "ALIVE (pid $SHELL_PID)"; else echo "DIED"; fi
  echo "=== session bus names (shell-owned) ==="
  dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null | grep string: | sed "s/^ *string *//"
  echo "=== org.gnome.Shell owner ==="
  dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.Shell 2>&1 | grep string:
  echo "=== org.gnome.Shell.Extensions owner ==="
  dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.Shell.Extensions 2>&1 | grep string:
  echo "=== org.gnome.Shell.Extensions introspection (methods present) ==="
  dbus-send --session --print-reply --dest=org.gnome.Shell.Extensions /org/gnome/Shell/Extensions org.freedesktop.DBus.Introspectable.Introspect 2>/dev/null | grep -oE "method name=\"[A-Za-z]+\"" | sort -u | head -8
  echo "=== Shell.Eval responds (false = upstream unsafe-mode default) ==="
  dbus-send --session --print-reply --dest=org.gnome.Shell /org/gnome/Shell org.gnome.Shell.Eval string:"1+2" 2>&1 | grep -E "boolean|string"
  kill $SHELL_PID 2>/dev/null
  sleep 2
  kill -9 $SHELL_PID 2>/dev/null
'
kill $(cat /run/dbus/pid 2>/dev/null) 2>/dev/null
echo "=== gnome-shell stderr (expected non-fatal notices) ==="
grep -E "CRITICAL|WARNING|ERROR" /tmp/gshell.out | grep -v "Stack trace\|^  \|^$" | head -8
