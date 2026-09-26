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

#include <stdlib.h>
#include "pthreadP.h"
#include <shlib-compat.h>

void
__cleanup_fct_attribute
___pthread_register_cancel_defer (__pthread_unwind_buf_t *buf)
{
  struct pthread_unwind_buf *ibuf = (struct pthread_unwind_buf *) buf;
  struct pthread *self = THREAD_SELF;
  ibuf->priv.data.prev = THREAD_GETMEM (self, cleanup_jmp_buf);
  ibuf->priv.data.cleanup = THREAD_GETMEM (self, cleanup);
  unsigned old = zthread_cancel_set (CANCELTYPE_BITMASK, 0);
  ibuf->priv.data.canceltype = old & CANCELTYPE_BITMASK
    ? PTHREAD_CANCEL_ASYNCHRONOUS : PTHREAD_CANCEL_DEFERRED;
  THREAD_SETMEM (self, cleanup_jmp_buf, (struct pthread_unwind_buf *) buf);
}
versioned_symbol (libc, ___pthread_register_cancel_defer,
		  __pthread_register_cancel_defer, GLIBC_2_34);

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_3_3, GLIBC_2_34)
compat_symbol (libpthread, ___pthread_register_cancel_defer,
	       __pthread_register_cancel_defer, GLIBC_2_3_3);
#endif

void
__cleanup_fct_attribute
___pthread_unregister_cancel_restore (__pthread_unwind_buf_t *buf)
{
  struct pthread_unwind_buf *ibuf = (struct pthread_unwind_buf *) buf;
  THREAD_SETMEM (THREAD_SELF, cleanup_jmp_buf, ibuf->priv.data.prev);
  if (ibuf->priv.data.canceltype == PTHREAD_CANCEL_ASYNCHRONOUS)
    {
      zthread_cancel_set (CANCELTYPE_BITMASK, CANCELTYPE_BITMASK);
      if (cancel_enabled_and_canceled (zthread_cancel_get ()))
        __do_cancel (PTHREAD_CANCELED);
    }
}
versioned_symbol (libc, ___pthread_unregister_cancel_restore,
		  __pthread_unregister_cancel_restore, GLIBC_2_34);

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_3_3, GLIBC_2_34)
compat_symbol (libpthread, ___pthread_unregister_cancel_restore,
	       __pthread_unregister_cancel_restore, GLIBC_2_3_3);
#endif
