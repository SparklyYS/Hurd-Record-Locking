/* Node state and file contents for tmpfs.
   Copyright (C) 2000,01,02 Free Software Foundation, Inc.

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

#include "tmpfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <hurd/hurd_types.h>
#include <hurd/store.h>
#include "default_pager_U.h"
#include "libdiskfs/fs_S.h"

unsigned int num_files;
static unsigned int gen;

struct node *all_nodes;
static size_t all_nodes_nr_items;

error_t
diskfs_alloc_node (struct node *dp, mode_t mode, struct node **npp)
{
  struct disknode *dn;

  dn = calloc (1, sizeof *dn);
  if (dn == 0)
    return ENOSPC;
  pthread_spin_lock (&diskfs_node_refcnt_lock);
  if (round_page (tmpfs_space_used + sizeof *dn) / vm_page_size
      > tmpfs_page_limit)
    {
      pthread_spin_unlock (&diskfs_node_refcnt_lock);
      free (dn);
      return ENOSPC;
    }
  dn->gen = gen++;
  ++num_files;
  tmpfs_space_used += sizeof *dn;
  pthread_spin_unlock (&diskfs_node_refcnt_lock);

  dn->type = IFTODT (mode & S_IFMT);
  return diskfs_cached_lookup ((ino_t) (uintptr_t) dn, npp);
}

void
diskfs_free_node (struct node *np, mode_t mode)
{
  switch (np->dn->type)
    {
    case DT_REG:
      if (np->dn->u.reg.memobj != MACH_PORT_NULL) {
	vm_deallocate (mach_task_self (), np->dn->u.reg.memref, 4096);
	mach_port_deallocate (mach_task_self (), np->dn->u.reg.memobj);
      }	
      break;
    case DT_DIR:
      assert (np->dn->u.dir.entries == 0);
      break;
    case DT_LNK:
      free (np->dn->u.lnk);
      break;
    }
  *np->dn->hprevp = np->dn->hnext;
  if (np->dn->hnext != 0)
    np->dn->hnext->dn->hprevp = np->dn->hprevp;
  all_nodes_nr_items -= 1;
  free (np->dn);
  np->dn = 0;

  --num_files;
  tmpfs_space_used -= sizeof *np->dn;
}

void
diskfs_node_norefs (struct node *np)
{
  if (np->dn != 0)
    {
      /* We don't bother to do this in diskfs_write_disknode, since it only
	 ever matters here.  The node state goes back into the `struct
	 disknode' while it has no associated diskfs node.  */

      np->dn->size = np->dn_stat.st_size;
      np->dn->mode = np->dn_stat.st_mode;
      np->dn->nlink = np->dn_stat.st_nlink;
      np->dn->uid = np->dn_stat.st_uid;
      np->dn->author = np->dn_stat.st_author;
      np->dn->gid = np->dn_stat.st_gid;
      np->dn->atime = np->dn_stat.st_atim;
      np->dn->mtime = np->dn_stat.st_mtim;
      np->dn->ctime = np->dn_stat.st_ctim;
      np->dn->flags = np->dn_stat.st_flags;

      switch (np->dn->type)
	{
	case DT_REG:
	  assert (np->allocsize % vm_page_size == 0);
	  np->dn->u.reg.allocpages = np->allocsize / vm_page_size;
	  break;
	case DT_CHR:
	case DT_BLK:
	  np->dn->u.chr = np->dn_stat.st_rdev;
	  break;
	}

      /* Remove this node from the cache list rooted at `all_nodes'.  */
      *np->dn->hprevp = np->dn->hnext;
      if (np->dn->hnext != 0)
	np->dn->hnext->dn->hprevp = np->dn->hprevp;
      all_nodes_nr_items -= 1;
      np->dn->hnext = 0;
      np->dn->hprevp = 0;
    }

  free (np);
}

static void
recompute_blocks (struct node *np)
{
  struct disknode *const dn = np->dn;
  struct stat *const st = &np->dn_stat;

  st->st_blocks = sizeof *dn + dn->translen;
  switch (dn->type)
    {
    case DT_REG:
      np->allocsize = dn->u.reg.allocpages * vm_page_size;
      st->st_blocks += np->allocsize;
      break;
    case DT_LNK:
      st->st_blocks += st->st_size + 1;
      break;
    case DT_CHR:
    case DT_BLK:
      st->st_rdev = dn->u.chr;
      break;
    case DT_DIR:
      st->st_blocks += dn->size;
      break;
    }
  st->st_blocks = (st->st_blocks + 511) / 512;
}

/* Fetch inode INUM, set *NPP to the node structure;
   gain one user reference and lock the node.  */
error_t
diskfs_cached_lookup (ino_t inum, struct node **npp)
{
  struct disknode *dn = (void *) (uintptr_t) inum;
  struct node *np;

  assert (npp);

  if (dn->hprevp != 0)		/* There is already a node.  */
    {
      np = *dn->hprevp;
      assert (np->dn == dn);
      assert (*dn->hprevp == np);

      diskfs_nref (np);
    }
  else
    /* Create the new node.  */
    {
      struct stat *st;

      np = diskfs_make_node (dn);
      np->cache_id = (ino_t) (uintptr_t) dn;

      pthread_spin_lock (&diskfs_node_refcnt_lock);
      dn->hnext = all_nodes;
      if (dn->hnext)
	dn->hnext->dn->hprevp = &dn->hnext;
      dn->hprevp = &all_nodes;
      all_nodes = np;
      all_nodes_nr_items += 1;
      pthread_spin_unlock (&diskfs_node_refcnt_lock);

      st = &np->dn_stat;
      memset (st, 0, sizeof *st);
      st->st_fstype = FSTYPE_MEMFS;
      st->st_fsid = getpid ();
      st->st_blksize = vm_page_size;

      st->st_ino = (ino_t) (uintptr_t) dn;
      st->st_gen = dn->gen;

      st->st_size = dn->size;
      st->st_mode = dn->mode;
      st->st_nlink = dn->nlink;
      st->st_uid = dn->uid;
      st->st_author = dn->author;
      st->st_gid = dn->gid;
      st->st_atim = dn->atime;
      st->st_mtim = dn->mtime;
      st->st_ctim = dn->ctime;
      st->st_flags = dn->flags;

      st->st_rdev = 0;
      np->allocsize = 0;
      recompute_blocks (np);
    }

  pthread_mutex_lock (&np->lock);
  *npp = np;
  return 0;
}

error_t
diskfs_node_iterate (error_t (*fun) (struct node *))
{
  error_t err = 0;
  size_t num_nodes;
  struct node *node, **node_list, **p;

  pthread_spin_lock (&diskfs_node_refcnt_lock);

  /* We must copy everything from the hash table into another data structure
     to avoid running into any problems with the hash-table being modified
     during processing (normally we delegate access to hash-table with
     diskfs_node_refcnt_lock, but we can't hold this while locking the
     individual node locks).  */

  num_nodes = all_nodes_nr_items;

  p = node_list = alloca (num_nodes * sizeof (struct node *));
  for (node = all_nodes; node != 0; node = node->dn->hnext)
    {
      *p++ = node;
      node->references++;
    }

  pthread_spin_unlock (&diskfs_node_refcnt_lock);

  p = node_list;
  while (num_nodes-- > 0)
    {
      node = *p++;
      if (!err)
	{
	  pthread_mutex_lock (&node->lock);
	  err = (*fun) (node);
	  pthread_mutex_unlock (&node->lock);
	}
      diskfs_nrele (node);
    }

  return err;
}

/* The user must define this function.  Node NP has some light
   references, but has just lost its last hard references.  Take steps
   so that if any light references can be freed, they are.  NP is locked
   as is the pager refcount lock.  This function will be called after
   diskfs_lost_hardrefs.  */
void
diskfs_try_dropping_softrefs (struct node *np)
{
}

/* The user must define this funcction.  Node NP has some light
   references but has just lost its last hard reference.  NP is locked. */
void
diskfs_lost_hardrefs (struct node *np)
{
}

/* The user must define this function.  Node NP has just acquired
   a hard reference where it had none previously.  It is thus now
   OK again to have light references without real users.  NP is
   locked. */
void
diskfs_new_hardrefs (struct node *np)
{
}



error_t
diskfs_get_translator (struct node *np, char **namep, u_int *namelen)
{
  *namelen = np->dn->translen;
  if (*namelen == 0)
    return 0;
  *namep = malloc (*namelen);
  if (*namep == 0)
    return ENOMEM;
  memcpy (*namep, np->dn->trans, *namelen);
  return 0;
}

error_t
diskfs_set_translator (struct node *np,
		       const char *name, u_int namelen,
		       struct protid *cred)
{
  char *new;
  if (namelen == 0)
    {
      free (np->dn->trans);
      new = 0;
      np->dn_stat.st_mode &= ~S_IPTRANS;
    }
  else
    {
      new = realloc (np->dn->trans, namelen);
      if (new == 0)
	return ENOSPC;
      memcpy (new, name, namelen);
      np->dn_stat.st_mode |= S_IPTRANS;
    }
  adjust_used (namelen - np->dn->translen);
  np->dn->trans = new;
  np->dn->translen = namelen;
  recompute_blocks (np);
  return 0;
}

static error_t
create_symlink_hook (struct node *np, const char *target)
{
  assert (np->dn->u.lnk == 0);
  np->dn_stat.st_size = strlen (target);
  if (np->dn_stat.st_size > 0)
    {
      const size_t size = np->dn_stat.st_size + 1;
      np->dn->u.lnk = malloc (size);
      if (np->dn->u.lnk == 0)
	return ENOSPC;
      memcpy (np->dn->u.lnk, target, size);
      np->dn->type = DT_LNK;
      adjust_used (size);
      recompute_blocks (np);
    }
  return 0;
}
error_t (*diskfs_create_symlink_hook)(struct node *np, const char *target)
     = create_symlink_hook;

static error_t
read_symlink_hook (struct node *np, char *target)
{
  memcpy (target, np->dn->u.lnk, np->dn_stat.st_size + 1);
  return 0;
}
error_t (*diskfs_read_symlink_hook)(struct node *np, char *target)
     = read_symlink_hook;

void
diskfs_write_disknode (struct node *np, int wait)
{
}

void
diskfs_file_update (struct node *np, int wait)
{
  diskfs_node_update (np, wait);
}

error_t
diskfs_node_reload (struct node *node)
{
  return 0;
}


/* The user must define this function.  Truncate locked node NP to be SIZE
   bytes long.  (If NP is already less than or equal to SIZE bytes
   long, do nothing.)  If this is a symlink (and diskfs_shortcut_symlink
   is set) then this should clear the symlink, even if
   diskfs_create_symlink_hook stores the link target elsewhere.  */
error_t
diskfs_truncate (struct node *np, off_t size)
{
  if (np->dn->type == DT_LNK)
    {
      free (np->dn->u.lnk);
      adjust_used (size - np->dn_stat.st_size);
      np->dn->u.lnk = 0;
      np->dn_stat.st_size = size;
      return 0;
    }

  if (np->allocsize <= size)
    return 0;

  assert (np->dn->type == DT_REG);

  if (default_pager == MACH_PORT_NULL)
    return EIO;

  np->dn_stat.st_size = size;

  off_t set_size = size;
  size = round_page (size);

  if (np->dn->u.reg.memobj != MACH_PORT_NULL)
    {
      error_t err = default_pager_object_set_size (np->dn->u.reg.memobj, set_size);
      if (err == MIG_BAD_ID)
	/* This is an old default pager.  We have no way to truncate the
	   memory object.  Note that the behavior here will be wrong in
	   two ways: user accesses past the end won't fault; and, more
	   importantly, later growing the file won't zero the contents
	   past the size we just supposedly truncated to.  For proper
	   behavior, use a new default pager.  */
	return 0;
      if (err)
	return err;
    }
  /* Otherwise it never had any real contents.  */

  adjust_used (size - np->allocsize);
  np->dn_stat.st_blocks += (size - np->allocsize) / 512;
  np->allocsize = size;

  return 0;
}

/* The user must define this function.  Grow the disk allocated to locked node
   NP to be at least SIZE bytes, and set NP->allocsize to the actual
   allocated size.  (If the allocated size is already SIZE bytes, do
   nothing.)  CRED identifies the user responsible for the call.  */
error_t
diskfs_grow (struct node *np, off_t size, struct protid *cred)
{
  assert (np->dn->type == DT_REG);

  if (np->allocsize >= size)
    return 0;

  off_t set_size = size;
  size = round_page (size);
  if (round_page (tmpfs_space_used + size - np->allocsize)
      / vm_page_size > tmpfs_page_limit)
    return ENOSPC;

  if (default_pager == MACH_PORT_NULL)
    return EIO;

  if (np->dn->u.reg.memobj != MACH_PORT_NULL)
    {
      /* Increase the limit the memory object will allow to be accessed.  */
      error_t err = default_pager_object_set_size (np->dn->u.reg.memobj, set_size);
      if (err == MIG_BAD_ID)	/* Old default pager, never limited it.  */
	err = 0;
      if (err)
	return err;
    }

  adjust_used (size - np->allocsize);
  np->dn_stat.st_blocks += (size - np->allocsize) / 512;
  np->allocsize = size;
  return 0;
}

mach_port_t
diskfs_get_filemap (struct node *np, vm_prot_t prot)
{
  error_t err;

  if (np->dn->type != DT_REG)
    {
      errno = EOPNOTSUPP;	/* ? */
      return MACH_PORT_NULL;
    }

  if (default_pager == MACH_PORT_NULL)
    {
      errno = EIO;
      return MACH_PORT_NULL;
    }

  /* We don't bother to create the memory object until the first time we
     need it (i.e. first mapping or i/o).  This way we might have a clue
     what size it's going to be beforehand, so we can tell the default
     pager how big to make its bitmaps.  This is just an optimization for
     the default pager; the memory object can be expanded at any time just
     by accessing more of it.  (It also optimizes the case of empty files
     so we might never make a memory object at all.) */
  if (np->dn->u.reg.memobj == MACH_PORT_NULL)
    {
      error_t err = default_pager_object_create (default_pager,
						 &np->dn->u.reg.memobj,
						 np->allocsize);
      if (err)
	{
	  errno = err;
	  return MACH_PORT_NULL;
	}
      assert (np->dn->u.reg.memobj != MACH_PORT_NULL);
      
      /* XXX we need to keep a reference to the object, or GNU Mach
	 will terminate it when we release the map. */
      np->dn->u.reg.memref = 0;
      vm_map (mach_task_self (), &np->dn->u.reg.memref, 4096, 0, 1,
	      np->dn->u.reg.memobj, 0, 0, VM_PROT_NONE, VM_PROT_NONE,
	      VM_INHERIT_NONE);
      assert_perror (err);
    }

  /* XXX always writable */

  /* Add a reference for each call, the caller will deallocate it.  */
  err = mach_port_mod_refs (mach_task_self (), np->dn->u.reg.memobj,
			    MACH_PORT_RIGHT_SEND, +1);
  assert_perror (err);

  return np->dn->u.reg.memobj;
}

/* The user must define this function.  Return a `struct pager *' suitable
   for use as an argument to diskfs_register_memory_fault_area that
   refers to the pager returned by diskfs_get_filemap for node NP.
   NP is locked.  */
struct pager *
diskfs_get_filemap_pager_struct (struct node *np)
{
  return 0;
}

/* We have no pager of our own, so there is no need to worry about
   users of it, or to shut it down.  */
int
diskfs_pager_users ()
{
  return 0;
}
void
diskfs_shutdown_pager ()
{
}

/* The purpose of this is to decide that it's ok to make the fs read-only.
   Turning a temporary filesystem read-only seem pretty useless.  */
vm_prot_t
diskfs_max_user_pager_prot ()
{
  return VM_PROT_READ;		/* Probable lie that lets us go read-only.  */
}

error_t
diskfs_S_file_get_storage_info (struct protid *cred,
				mach_port_t **ports,
				mach_msg_type_name_t *ports_type,
				mach_msg_type_number_t *num_ports,
				int **ints, mach_msg_type_number_t *num_ints,
				off_t **offsets,
				mach_msg_type_number_t *num_offsets,
				char **data, mach_msg_type_number_t *data_len)
{
  mach_port_t memobj = diskfs_get_filemap (cred->po->np, VM_PROT_ALL);
  if (memobj == MACH_PORT_NULL)
    return errno;

  assert (*num_ports >= 1);	/* mig always gives us some */
  *num_ports = 1;
  *ports_type = MACH_MSG_TYPE_MOVE_SEND;
  (*ports)[0]
    = (cred->po->openstat & O_RDWR) == O_RDWR ? memobj : MACH_PORT_NULL;

  assert (*num_offsets >= 2);	/* mig always gives us some */
  *num_offsets = 2;
  (*offsets)[0] = 0;
  (*offsets)[1] = cred->po->np->dn_stat.st_size;

  assert (*num_ints >= 6);	/* mig always gives us some */
  *num_ints = 6;
  (*ints)[0] = STORAGE_MEMORY;
  (*ints)[1] = (cred->po->openstat & O_WRITE) ? 0 : STORE_READONLY;
  (*ints)[2] = 1;		/* block size */
  (*ints)[3] = 1;		/* 1 run in offsets list */
  (*ints)[4] = 0;		/* name len */
  (*ints)[5] = 0;		/* misc len */

  *data_len = 0;

  return 0;
}
