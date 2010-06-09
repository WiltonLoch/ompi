/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009-2010 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Los Alamos National Security, LLC.  
 *                         All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#ifndef _COMMON_SM_H_
#define _COMMON_SM_H_

#include "ompi_config.h"

#include "opal/mca/mca.h"
#include "opal/class/opal_object.h"
#include "opal/class/opal_list.h"
#include "opal/sys/atomic.h"
#include "ompi/mca/mpool/mpool.h" 
#include "ompi/proc/proc.h"
#include "ompi/group/group.h"

BEGIN_C_DECLS

struct mca_mpool_base_module_t;

typedef struct mca_common_sm_seg_header_t 
{
    /* lock to control atomic access */
    opal_atomic_lock_t seg_lock;
    /* is the segment ready for use */
    volatile int32_t seg_inited;
    /* offset to next available memory location available for allocation */
    size_t seg_offset;
    /* total size of the segment */
    size_t seg_size;
} mca_common_sm_seg_header_t;

typedef struct mca_common_sm_module_t 
{
    /* double link list element */
    opal_list_item_t module_item;
    /* pointer to header embedded in the shared memory segment */
    mca_common_sm_seg_header_t *module_seg;
    /* base address of the segment */
    unsigned char *module_seg_addr;
    /* base address of data segment */
    unsigned char *module_data_addr;
    /* how big it is (in bytes) */
    size_t module_size;
    char module_seg_path[OPAL_PATH_MAX];
#if defined(__WINDOWS__)
    /* handle to the object */
    HANDLE hMappedObject;
#endif  /* defined(__WINDOWS__) */
} mca_common_sm_module_t;

OBJ_CLASS_DECLARATION(mca_common_sm_module_t);

OMPI_DECLSPEC extern int
mca_common_sm_param_register(mca_base_component_t *c);

/**
 * Register the MCA parameters for common sm.
 */
int
mca_common_sm_param_register(mca_base_component_t *c);

/**
 *  This routine is used to set up a shared memory segment (whether
 *  it's an mmaped file or a SYSV IPC segment).  It is assumed that
 *  the shared memory segment does not exist before any of the current
 *  set of processes try and open it.
 *
 *  @param procs - array of (ompi_proc_t*)'s to create this shared
 *  memory segment for.  This array must be writable; it may be edited
 *  (in undefined ways) if the array contains procs that are not on
 *  this host.  It is assumed that the caller will simply free this
 *  array upon return.  (INOUT)
 *
 *  @param num_procs - length of the procs array (IN)
 *
 *  @param size - size of the segment, in bytes (IN)
 *
 *  @param name - unique string identifier of this segment (IN)
 *
 *  @param size_ctl_structure  size of the control structure at
 *                             the head of the segment. The control structure
 *                             is assumed to have mca_common_sm_seg_header_t
 *                             as its first segment (IN)
 *
 *  @param data_set_alignment  alignment of the data segment.  this
 *                             follows the control structure.  If this 
 *                             value if 0, then assume that there will 
 *                             be no data segment following the control 
 *                             structure. (IN)
 *
 *  @returnvalue pointer to control structure at head of shared memory segment.
 */
OMPI_DECLSPEC extern mca_common_sm_module_t *
mca_common_sm_init(ompi_proc_t **procs,
                   size_t num_procs,
                   size_t size, 
                   char *file_name,
                   size_t size_ctl_structure, 
                   size_t data_seg_alignment);

typedef mca_common_sm_module_t *
(*mca_common_sm_init_fn_t)(ompi_proc_t **procs,
                           size_t num_procs,
                           size_t size, 
                           char *file_name,
                           size_t size_ctl_structure, 
                           size_t data_seg_alignment);

/**
 *  This routine is used to set up a shared memory segment (whether
 *  it's an mmaped file or a SYSV IPC segment).  It is assumed that
 *  the shared memory segment does not exist before any of the current
 *  set of processes try and open it.
 *
 * This routine is the same as mca_common_sm_mmap_init() except that
 * it takes an (ompi_group_t*) parameter to specify the peers rather
 * than an array of procs.  Unlike mca_common_sm_mmap_init(), the
 * group must contain *only* local peers, or this function will return
 * NULL and not create any shared memory segment.
 */
OMPI_DECLSPEC extern mca_common_sm_module_t *
mca_common_sm_init_group(ompi_group_t *group,
                         size_t size, 
                         char *file_name,
                         size_t size_ctl_structure, 
                         size_t data_seg_alignment);

typedef mca_common_sm_module_t *
(*mca_common_sm_init_group_fn_t)(ompi_group_t *group,
                                 size_t size, 
                                 char *file_name,
                                 size_t size_ctl_structure, 
                                 size_t data_seg_alignment);

/**
 * callback from the sm mpool
 */
OMPI_DECLSPEC extern void *
mca_common_sm_seg_alloc(struct mca_mpool_base_module_t *mpool, 
                        size_t* size, 
                        mca_mpool_base_registration_t **registration);

typedef void *
(*mca_common_sm_seg_alloc_fn_t)(struct mca_mpool_base_module_t *mpool, 
                                size_t* size, 
                                mca_mpool_base_registration_t **registration);

/**
 * This function will release all local resources attached to the
 * shared memory segment. We assume that the operating system will 
 * release the memory resources when the last process release it.
 *
 * @param mca_common_sm_module - instance that is shared between 
 *                               components that use shared memory.
 *
 * @returnvalue 0 if everything was OK, otherwise a negative value.
 */

OMPI_DECLSPEC extern int 
mca_common_sm_fini(mca_common_sm_module_t *mca_common_sm_module);

typedef int 
(*mca_common_sm_fini_fn_t)(mca_common_sm_module_t *mca_common_sm_module);

/*
 * instance that is shared between components that use shared memory
 */
OMPI_DECLSPEC extern mca_common_sm_module_t *mca_common_sm_module;

END_C_DECLS

#endif /* _COMMON_SM_H_ */

