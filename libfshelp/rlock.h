/*
   Copyright (C) 2001, 2014 Free Software Foundation

   Written by Neal H Walfield <neal@cs.uml.edu>

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   The GNU Hurd is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the GNU Hurd; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef FSHELP_RLOCK_H
#define FSHELP_RLOCK_H

//#include <pthread.h>
//#include <fcntl.h>

#include <string.h>
#//include "ports.h"

struct rlock_linked_list
{
  struct rlock_list *next;
  struct rlock_list **prevp;
};

/*
libthreads/cthreads.h:
typedef struct condition {
        spin_lock_t lock;
        struct cthread_queue queue;
        const char *name; <-- differs
        struct cond_imp *implications;
} *condition_t;
rlock.h:
  struct condition wait;
here replaced by
  struct pthread_cond_t wait;
/usr/include/pthread/pthreadtypes.h:typedef struct __pthread_cond pthread_cond_t;
/usr/include/i386-gnu/bits/condition.h:struct __pthread_cond
struct __pthread_cond
{
  __pthread_spinlock_t __lock;
  struct __pthread *__queue;
  struct __pthread_condattr *__attr; <-- differs
  struct __pthread_condimpl *__impl;
  void *__data; <-- differs
};
 */

struct rlock_list
{
  loff_t start;
  loff_t len;
  int type;

  struct rlock_linked_list node;
  struct rlock_linked_list po;

  pthread_cond_t wait;
  int waiting;

  void *po_id;
};

extern int pthread_cond_init (pthread_cond_t *__restrict cond,
                              const pthread_condattr_t *__restrict attr);

extern inline error_t
rlock_list_init (struct rlock_peropen *po, struct rlock_list *l)
{
  memset (l, 0, sizeof (struct rlock_list));
  pthread_cond_init (&l->wait, NULL);
  l->po_id = po->locks;
  return 0;
}

/* void list_list (X ={po,node}, struct rlock_list **head,
		   struct rlock_list *node)

   Insert a node in the given list, X, in sorted order.  */
#define list_link(X, head, node)				\
	do							\
	  {							\
	    struct rlock_list **e;				\
	    for (e = head;					\
		 *e && ((*e)->start < node->start);		\
		 e = &(*e)->X.next)				\
	      ;							\
	    node->X.next = *e;					\
	    if (node->X.next)					\
	      node->X.next->X.prevp = &node->X.next;		\
	    node->X.prevp = e;					\
	    *e = node;						\
	  }							\
	while (0)

/* void list_unlock (X = {po,node}, struct rlock_list *node)  */
#define list_unlink(X, node)					\
  	do							\
	  {							\
	    *node->X.prevp = node->X.next;			\
	    if (node->X.next)					\
	      node->X.next->X.prevp = node->X.prevp;		\
	  }							\
	while (0)

#endif /* FSHELP_RLOCK_H */
