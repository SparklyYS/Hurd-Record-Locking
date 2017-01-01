/* This test checks the return types of F_GETLK64
   Copyright (C) 2014 Free Software Foundation, Inc.

   Written by Svante Signell <svante.signell@gmail.com>

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

#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include "fs_U.h"
#include <hurd.h>

int main (int argc, char **argv)
{
  error_t err;
  struct flock64 lock;
  int fd;

  if (argc != 2)
    error (1, 0, "Usage: %s file", argv[0]);

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 10;
  lock.l_len = 20;
  lock.l_pid = 1;

  printf("GNU/Hurd\n");
  printf("F_RDLCK = 1, F_WRLCK = 2, F_UNLCK = 3\n");
  printf("SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2\n");

  printf("\nBefore calling file_record_lock\n");
  printf("{lock.l_type = %d lock.l_whence = %d lock.l_start = %ld lock.l_len = %ld lock.l_pid = %d}\n",
	 lock.l_type, lock.l_whence, (long)lock.l_start, (long)lock.l_len, lock.l_pid);
  fd = file_name_lookup (argv[1], O_READ | O_WRITE | O_CREAT, 0666);
  if (fd == MACH_PORT_NULL)
    error (1, errno, "file_name_lookup");

  err = file_record_lock (fd, F_GETLK64, &lock);
  if (err)
    error (1, err, "file_record_lock");

  printf("\nAfter calling file_record_lock\n");
  printf("{lock.l_type = %d lock.l_whence = %d lock.l_start = %ld lock.l_len = %ld lock.l_pid = %d}\n",
	 lock.l_type, lock.l_whence, (long)lock.l_start, (long)lock.l_len, lock.l_pid);

  mach_port_deallocate (mach_task_self (), fd);

  return 0;
}
