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

#include "pthreadP.h"
#include <tls.h>
#include <libc-lock.h>

void
__libc_cleanup_push_defer (struct _pthread_cleanup_buffer *buffer)
{
  struct pthread *self = THREAD_SELF;
  buffer->__prev = THREAD_GETMEM (self, cleanup);
  buffer->__filc_frame = zget_call_frame (1);
  unsigned old = zthread_cancel_set (CANCELTYPE_BITMASK, 0);
  buffer->__canceltype = old & CANCELTYPE_BITMASK
    ? PTHREAD_CANCEL_ASYNCHRONOUS : PTHREAD_CANCEL_DEFERRED;
  THREAD_SETMEM (self, cleanup, buffer);
}
libc_hidden_def (__libc_cleanup_push_defer)

void
__libc_cleanup_pop_restore (struct _pthread_cleanup_buffer *buffer)
{
  THREAD_SETMEM (THREAD_SELF, cleanup, buffer->__prev);
  if (buffer->__canceltype == PTHREAD_CANCEL_ASYNCHRONOUS)
    {
      zthread_cancel_set (CANCELTYPE_BITMASK, CANCELTYPE_BITMASK);
      if (cancel_enabled_and_canceled (zthread_cancel_get ()))
        __do_cancel (PTHREAD_CANCELED);
    }
}
libc_hidden_def (__libc_cleanup_pop_restore)
