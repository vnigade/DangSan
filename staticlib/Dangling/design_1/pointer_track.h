/*
 * pointer_track.h
 * 	Author - Vinod
 */
#ifndef POINTER_TRACK_H
#define POINTER_TRACK_H

#include <stdint.h>

typedef struct dang_headnode    dang_headnode_t;
typedef struct dang_objectnode  dang_objectnode_t;

/*
 * MetaAlloc Metadata will store this structure.
 * Sizeof this structure is equal to sizeof meta32.
 * There has to be way in MetaAlloc to allocate metadata
 * of any required size and return pointer to the metadata.
 */
struct dang_headnode {
        uint64_t                magic;          /* DANG_MAGIC ^ id */
        //void                    *lock;          /* Currently unsed */
        uint64_t                id;             /* Unique object ID */
        dang_objectnode_t       *root_node;     /* Pointer list of pointers */
	pthread_mutex_t 	lock;		/* Mutex per object. This increases memory requirement.
						 * Properly weigh option whether to use global lock
						 * Or have region wise lock(e.g. lock table) */
#ifdef DANG_STATS
        unsigned long           ptr_duplicate;  /* Number of duplicate pointers to the object */
        unsigned long           ptr_unique;     /* Number of unique pointers to the object */
        unsigned long           ptr_ctr;        /* Next sequential entry in the object log */
        unsigned long           lastptr_dup;    /* Number of duplicate pointers that were last */
#endif
};

/*
 * Every memory object will have list of pointers.
 * Pointer information will stored in this structure.
 */
struct dang_objectnode {
        void                    *ptr;           /* Ptr pointing to the object */
        dang_objectnode_t       *object_next;   /* Next pointer for the same object */
        dang_objectnode_t       *object_prev;   /* Prev pointer for the same object */
        dang_objectnode_t       *ptr_next;      /* Next pointer info in the same hash bucket */
        dang_objectnode_t       *ptr_prev;      /* Prev pointer info in the same hash bucket */
        uint64_t                value;          /* Value that pointer is currently pointing to */
        uint64_t                id;             /* ID of the object in which ptr resides */
/* XXX: own_allocator has only 64 bytes allocation unit */
#ifdef DANG_STATS
        unsigned long           obj_ctr;        /* Counter : Sequential entry in the object log */
#endif
};

//extern void dang_alloc_hashtable();
//extern void dang_registerptr(unsigned long ptr_addr, unsigned long object_addr);
//extern void dang_freeptr(unsigned long obj_addr);
//extern void dang_init_object(unsigned long obj_addr, unsigned long size);
#endif /* POINTER_TRACK_H */
