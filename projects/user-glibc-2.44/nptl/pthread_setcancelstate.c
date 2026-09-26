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
#include <libc-lockP.h>

int
__pthread_setcancelstate (int state, int *oldstate)
{
  if (state < PTHREAD_CANCEL_ENABLE || state > PTHREAD_CANCEL_DISABLE)
    return EINVAL;
  int old = zthread_cancel_set (CANCELSTATE_BITMASK,
                                state == PTHREAD_CANCEL_DISABLE ? CANCELSTATE_BITMASK : 0);
  if (oldstate != NULL)
    *oldstate = old & CANCELSTATE_BITMASK ? PTHREAD_CANCEL_DISABLE : PTHREAD_CANCEL_ENABLE;
  if (cancel_enabled_and_canceled_and_async (zthread_cancel_get ()))
    __do_cancel (PTHREAD_CANCELED);
  return 0;
}
libc_hidden_def (__pthread_setcancelstate)
weak_alias (__pthread_setcancelstate, pthread_setcancelstate)
