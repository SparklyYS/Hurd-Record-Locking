/* Copyright (C) 2001, 2014 Free Software Foundation, Inc.

   Written by Neal H Walfield <neal@cs.uml.edu>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "priv.h"
#include "fs_S.h"

#include <fcntl.h>
#include <sys/file.h>

kern_return_t
diskfs_S_file_lock (struct protid *cred, int flags)
{
  error_t err;
  struct flock64 lock;
  struct node *node;

  if (! cred)
    return EOPNOTSUPP;

  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  if (flags & LOCK_UN)
    lock.l_type = F_UNLCK;
  else if (flags & LOCK_SH)
    lock.l_type = F_RDLCK;
  else if (flags & LOCK_EX)
    lock.l_type = F_WRLCK;
  else
    return EINVAL;

  node = cred->po->np;
  pthread_mutex_lock (&node->lock);
  err = fshelp_rlock_tweak (&node->userlock, &node->lock,
			    &cred->po->lock_status, cred->po->openstat,
			    0, 0, flags & LOCK_NB ? F_SETLK64 : F_SETLKW64,
			    &lock);
  pthread_mutex_unlock (&node->lock);
  return err;
}
