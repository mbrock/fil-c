#!/bin/bash

set -e
set -x

ulimit -c unlimited

test $EUID -eq 0
id -u lfs

export LFS=/mnt/lfs

test -d $LFS

export FILCSRC=..
test -d $FILCSRC/projects

test -e $LFS/sources/lfsbuildstate
lfsbuildstate=`cat $LFS/sources/lfsbuildstate`
test "x$lfsbuildstate" = "xpostlc7"

SRCDIR=$PWD

echo "postlc8-part" > $LFS/sources/lfsbuildstate

FILCOWNER=`stat -c %U $FILCSRC`
id -u $FILCOWNER
su $FILCOWNER ./build_postlc8_sub1_packaging.sh

./build_unmount.sh
./build_mount.sh

cp -v $FILCSRC/projects/*/pizlonated-*.tar.gz $LFS/sources
cp -v build_postlc8_sub2_chroot.sh $LFS/sources
cp -v build_postlc8_chroot_project_*.sh $LFS/sources

# Sprint 9 (live boot): the session glue must match the postlc8 packages.
# The postlc7 snapshot this stage recovers from predates the Sprint 5/6
# session wiring in the repo tree (its snapshot has the standalone-mutter
# session: RequiredComponents=pizlix-mutter, no /usr/bin/pizlix-shell), and
# sub2 only installs packages - without this step the recovered postlc8
# system would boot the superseded mutter session instead of GNOME Shell.
# Refresh the wiring the same way build_postlc7.sh does, then re-run the
# smoke test (it asserts the shell glue: org.gnome.Shell session component,
# pizlix-shell launcher, shell typelibs, no pizlix-mutter autostart).
cp -v build_postlc7_chroot_setup_scripts_for_gnome.sh \
      build_postlc7_chroot_gnome_smoke.sh $LFS/sources
mkdir -pv $LFS/sources/etc/init.d $LFS/sources/etc/gnome-session \
          $LFS/sources/etc/xdg/autostart $LFS/sources/etc/gnome
cp -v etc/init.d/dbus etc/init.d/elogind $LFS/sources/etc/init.d
cp -v etc/gnome-session/pizlix-gnome.session $LFS/sources/etc/gnome-session
cp -v etc/xdg/autostart/org.gnome.Shell.desktop \
      etc/xdg/autostart/pizlix-terminal.desktop $LFS/sources/etc/xdg/autostart
cp -v etc/gnome/pizlix-gnome.sh etc/gnome/pizlix-shell.sh \
      etc/gnome/pizlix-mutter.sh $LFS/sources/etc/gnome

./build_chroot_late.sh /sources/build_postlc8_sub2_chroot.sh

./build_chroot_late.sh /sources/build_postlc7_chroot_setup_scripts_for_gnome.sh
./build_chroot_late.sh /sources/build_postlc7_chroot_gnome_smoke.sh

echo "postlc8" > $LFS/sources/lfsbuildstate

./build_unmount.sh

cd $LFS
tar -czpf $SRCDIR/lfs-postlc8.tar.gz --exclude='var/coredumps/*' .

echo Post-libc part 8 OK
