/* Linux openat syscall implementation, LFS.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <fcntl.h>
#include <stdarg.h>

#include <sysdep-cancel.h>
#include <pizlonated_syscalls.h>
#include <pthreadP.h>

/* Open FILE with access OFLAG.  Interpret relative paths relative to
   the directory associated with FD.  If OFLAG includes O_CREAT or
   O_TMPFILE, a fourth argument is the file protection.  */
int
__libc_openat64 (int fd, const char *file, int oflag, ...)
{
  __pthread_testcancel ();
  mode_t mode = 0;
  if (__OPEN_NEEDS_MODE (oflag))
    {
      va_list ap;
      va_start (ap, oflag);
      mode = va_arg (ap, int);
      va_end (ap);
    }
  int canceled;
  int result = zsys_openat_cancel (fd, file, oflag, mode, &canceled);
  if (canceled)
    __do_cancel (PTHREAD_CANCELED);
  return result;
}

strong_alias (__libc_openat64, __openat64)
libc_hidden_weak (__openat64)
weak_alias (__libc_openat64, openat64)

#ifdef __OFF_T_MATCHES_OFF64_T
strong_alias (__libc_openat64, __openat)
libc_hidden_weak (__openat)
weak_alias (__libc_openat64, openat)
#endif
