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
#include "pthreadP.h"
#include <atomic.h>


int
__pthread_setcanceltype (int type, int *oldtype)
{
  if (type < PTHREAD_CANCEL_DEFERRED || type > PTHREAD_CANCEL_ASYNCHRONOUS)
    return EINVAL;
  int old = zthread_cancel_set (CANCELTYPE_BITMASK,
                                type == PTHREAD_CANCEL_ASYNCHRONOUS ? CANCELTYPE_BITMASK : 0);
  if (oldtype != NULL)
    *oldtype = old & CANCELTYPE_BITMASK ? PTHREAD_CANCEL_ASYNCHRONOUS : PTHREAD_CANCEL_DEFERRED;
  if (cancel_enabled_and_canceled_and_async (zthread_cancel_get ()))
    __do_cancel (PTHREAD_CANCELED);
  return 0;
}
libc_hidden_def (__pthread_setcanceltype)
weak_alias (__pthread_setcanceltype, pthread_setcanceltype)
