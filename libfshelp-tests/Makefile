# Makefile libfshelp test cases
#
#   Copyright (C) 2001 Free Software Foundation, Inc.
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2, or (at
#   your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

dir := libfshelp-tests
makemode := utilities

targets = race locks fork checklock
SRCS = race.c locks.c fork.c checklock.c

MIGSTUBS = fsUser.o ioUser.o
OBJS = $(SRCS:.c=.o) $(MIGSTUBS)
HURDLIBS = fshelp ports
LDLIBS += -lpthread

race: race.o fsUser.o ioUser.o
fork: fork.o fsUser.o
locks: locks.o
checklock: checklock.o fsUser.o

race locks: ../libfshelp/libfshelp.a ../libports/libports.a \
	    ../libthreads/libthreads.a

# Define the 64 bit versions of the second argument to fcntl()
# Can safely be removed when glibc is updated
EXTRA_CPP_FLAGS= -DF_GETLK64=10 -DF_SETLK64=11 -DF_SETLKW64=12
fork-CPPFLAGS += $(EXTRA_CPP_FLAGS)
locks-CPPFLAGS += $(EXTRA_CPP_FLAGS)
race-CPPFLAGS += $(EXTRA_CPP_FLAGS)
checklock-CPPFLAGS += $(EXTRA_CPP_FLAGS)

include ../Makeconf

