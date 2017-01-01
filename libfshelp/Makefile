#   Copyright (C) 1994, 95, 96, 98, 1999, 2006, 2012 Free Software Foundation,
#   Inc.
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

dir := libfshelp
makemode := library

libname = libfshelp
SRCS =	lock-acquire.c lock-init.c \
	translator-list.c \
	start-translator-long.c start-translator.c \
	fetch-root.c transbox-init.c set-active.c fetch-control.c \
	drop-transbox.c translated.c \
	delegate.c \
	exec-reauth.c \
	set-options.c \
	get-identity.c \
	perms-isowner.c perms-iscontroller.c perms-access.c \
	perms-checkdirmod.c \
	touch.c \
	extern-inline.c \
	rlock-drop-peropen.c rlock-tweak.c rlock-status.c

installhdrs = fshelp.h locks.h trans.h rlock.h

HURDLIBS = shouldbeinlibc iohelp ports ihash
LDLIBS += -lpthread
OBJS = $(subst .c,.o,$(SRCS))

# Define the 64 bit versions of the second argument to fcntl()
# Can safely be removed when glibc is updated
EXTRA_CPP_FLAGS= -DF_GETLK64=10 -DF_SETLK64=11 -DF_SETLKW64=12
rlock-tweak-CPPFLAGS += $(EXTRA_CPP_FLAGS)

include ../Makeconf
