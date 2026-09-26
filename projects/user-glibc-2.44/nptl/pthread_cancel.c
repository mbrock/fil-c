/* Copyright (C) 2002-2026 Free Software Foundation, Inc.
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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "pthreadP.h"
#include <atomic.h>
#include <sysdep.h>
#include <unistd.h>
#include <unwind-link.h>
#include <cancellation-pc-check.h>
#include <stdio.h>
#include <gnu/lib-names.h>
#include <sys/single_threaded.h>

/* Native notification and request state are owned by libpizlo. */

int
__pthread_cancel (pthread_t th)
{
  struct pthread *pd = (struct pthread *) th;
  if (atomic_load_relaxed (&pd->joinstate) == THREAD_STATE_EXITED)
    return 0;
  if (!zthread_cancel_request (pd->zthread))
    return errno;
  if (pd == THREAD_SELF
      && cancel_enabled_and_canceled_and_async (zthread_cancel_get ()))
    __do_cancel (PTHREAD_CANCELED);
  return 0;
}
versioned_symbol (libc, __pthread_cancel, pthread_cancel, GLIBC_2_34);

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libpthread, __pthread_cancel, pthread_cancel, GLIBC_2_0);
#endif

/* Ensure that the unwinder is always linked in (the __pthread_unwind
   reference from __do_cancel is weak).  Use ___pthread_unwind_next
   (three underscores) to produce a strong reference to the same
   file.  */
PTHREAD_STATIC_FN_REQUIRE (___pthread_unwind_next)
