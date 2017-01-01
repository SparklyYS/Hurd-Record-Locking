/*
   Copyright (C) 2001, 2014 Free Software Foundation

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
#include "trivfs_fs_S.h"

kern_return_t
trivfs_S_file_record_lock (struct trivfs_protid *cred,
			   mach_port_t reply,
			   mach_msg_type_name_t reply_type,
			   int cmd, struct flock64 *lock)
{
  return EOPNOTSUPP;
}
