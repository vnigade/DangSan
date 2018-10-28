/*
 * pointer_track.c
 * 	Implementss pointer registration and free list functions
 * 	including data structure operation. Design is similar to
 * 	FreeSentry.
 * 	TODO: Locking is commented out. Also, after DangSan implementation
 * 	have not built this. Thus, it may require some effort to compile.
 */
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include "metapagetable.h"
#include "pointer_track.h"
#include "metadata.h"
#include "dsan_common.h"
#include <pthread.h>

/* Stats */
#ifdef DANG_STATS
#include "pointer_stats.h"
#endif

#define unlikely(x)             __builtin_expect((x),0)

#define DANG_MAGIC 	        0x99109910U
#define ISVALID_HEADNODE(headnode) 	\
(headnode && DANG_MAGIC == (headnode->id ^ headnode->magic))
#define GLOBAL_SHIFTBITS 16 					/* Global number of objects 2 ^ 16 */
#define ISGLOBAL_OBJECT(headnode) 	\
(headnode && (headnode->id & ((1 << GLOBAL_SHIFTBITS) - 1))) 	/* Run time check for Global objects */

static uint64_t 		dang_objectID = 0;
//static pthread_spinlock_t 	dang_objectlock; 		/* Global lock */

/*
 * Hash table (pointer lookup table) is required only in FreeSentry implementation.
 */
//#define DANG_HASHTABLESIZE    8388608                         /* 64MB / sizeof(void *) ; 64MB to reduce O(N) search */
#define DANG_HASHTABLESIZE      67108864                        /* Total hash table size */

typedef struct {
	dang_objectnode_t 	**dang_hashtable; 
//	pthread_rwlock_t 	rwlock;
} dang_hashtable_info_t;

static dang_hashtable_info_t 	dang_hashtable_info;

static inline void dang_init_object(unsigned long obj_addr, unsigned long size, bool is_global);
static void dang_init_heapobj(unsigned long obj_addr, unsigned long size);
static void dang_freeptr(unsigned long obj_addr, unsigned long size);

void
dang_alloc_hashtable()
{
	size_t 	table_bytes = DANG_HASHTABLESIZE * sizeof(dang_objectnode_t *);
	//size_t          table_bytes = DANG_HASHTABLESIZE;

	if (unlikely(!dang_hashtable_info.dang_hashtable)) {
		dang_hashtable_info.dang_hashtable = mmap(NULL, table_bytes, PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (dang_hashtable_info.dang_hashtable == MAP_FAILED) {
			perror("Could not allocate memory for hash table");
			exit(EXIT_FAILURE);
		}
	}
	memset(dang_hashtable_info.dang_hashtable, 0, table_bytes);
	//pthread_rwlock_init(&dang_hashtable_info.rwlock, NULL); /* TODO: destroy in .fini */
	return;
}

/*__attribute__((section(".preinit_array"), used))
void (*init_dang)(void) = dang_initialize;*/

/*
 * Find hash bucket and travese to find the pointer info.
 * Linear search O(N) for doubly linked list.
 */
static inline dang_objectnode_t *
dang_getnode_hashtable(unsigned long ptr_addr)
{
	//size_t 			hash_index = (ptr_addr >> 4) % DANG_HASHTABLESIZE;
	size_t                          hash_index = ((ptr_addr >> 3) & (DANG_HASHTABLESIZE - 1));

	//pthread_rwlock_rdlock(&dang_hashtable_info.rwlock);
	dang_objectnode_t 	*ptr_info = dang_hashtable_info.dang_hashtable[hash_index];
	/* Hash table lock is required */
	while (ptr_info != NULL) {
		if ((unsigned long)ptr_info->ptr == ptr_addr)
			break;
		ptr_info = ptr_info->ptr_next;
	}
	//pthread_rwlock_unlock(&dang_hashtable_info.rwlock);
	return ptr_info;
}

/*
 * Allocate node and prepend it to the hash bucket.
 */
static inline dang_objectnode_t * 
dang_insertnode_hashtable(unsigned long ptr_addr)
{
	//size_t 		hash_index = (ptr_addr >> 4) % DANG_HASHTABLESIZE;
        size_t                  hash_index = ((ptr_addr >> 3) & (DANG_HASHTABLESIZE - 1));
	dang_objectnode_t 	*ptr_info;
	meta8 			metadata = metaget_8(ptr_addr);
	dang_headnode_t 	*obj_headnode = (dang_headnode_t *)metadata;
	
	LOG_MSG3("%s ptr_addr=%lx headnode=%lx\n", __func__, ptr_addr, obj_headnode);	
	ptr_info = (dang_objectnode_t *)DANG_MALLOC(sizeof(dang_objectnode_t));
	ptr_info->ptr = (void *)ptr_addr;
	ptr_info->object_next = ptr_info->object_prev = NULL;
	ptr_info->ptr_next = ptr_info->ptr_prev = NULL;
	ptr_info->id = obj_headnode->id; 

	/* TODO: Proper locking is required */
	//pthread_rwlock_wrlock(&dang_hashtable_info.rwlock);
	dang_objectnode_t       *root_info = dang_hashtable_info.dang_hashtable[hash_index];
	if (root_info != NULL) {
		root_info->ptr_prev = ptr_info;
		ptr_info->ptr_next = root_info;
	}
	dang_hashtable_info.dang_hashtable[hash_index] = ptr_info;
	//pthread_rwlock_unlock(&dang_hashtable_info.rwlock);
	return ptr_info;
}

/*
 * Remove node from the hash table
 * TODO: Looks similar to dang_rmnode_objlist
 */
static inline void
dang_rmnode_hashtable(dang_objectnode_t *ptr_info)
{
	//size_t        hash_index = ((unsigned long)ptr_info->ptr >> 4) % DANG_HASHTABLESIZE;
	size_t          hash_index = (((unsigned long)ptr_info->ptr >> 3) & (DANG_HASHTABLESIZE - 1));
	
	//pthread_rwlock_wrlock(&dang_hashtable_info.rwlock);
	dang_objectnode_t 	*root_info = dang_hashtable_info.dang_hashtable[hash_index];
	if (ptr_info == root_info) {
		dang_hashtable_info.dang_hashtable[hash_index] = ptr_info->ptr_next;
		if (ptr_info->ptr_next)
			dang_hashtable_info.dang_hashtable[hash_index]->ptr_prev = NULL;
	} else {
		ptr_info->ptr_prev->ptr_next = ptr_info->ptr_next;		
		if (ptr_info->ptr_next)
			ptr_info->ptr_next->ptr_prev = ptr_info->ptr_prev;
	}
	//pthread_rwlock_unlock(&dang_hashtable_info.rwlock);
	ptr_info->ptr_next = ptr_info->ptr_prev = NULL;
	return;
}

/*
 * Remove it only from the object list.
 * Get lock from obj_headnode.
 */
static inline void
dang_rmnode_objlist(dang_objectnode_t *ptr_info)
{
	meta8 			metadata = metaget_8(ptr_info->value);
	dang_headnode_t		*obj_headnode = (dang_headnode_t *)metadata;
	
	LOG_MSG3("%s ptr_info value %lx and metadata %lx\n", __func__, ptr_info->value, obj_headnode);	
	//fflush(stdout); /* TODO: remove this */

	//pthread_mutex_lock(&(obj_headnode->lock));
	dang_objectnode_t 	*root_node = obj_headnode->root_node;
	/* TODO: Under obj_headnode lock */
	if (root_node == ptr_info) { 	/* First node in the list */
		obj_headnode->root_node = ptr_info->object_next;
		if (ptr_info->object_next)
			(obj_headnode->root_node)->object_prev = NULL;
	} else {
		ptr_info->object_prev->object_next = ptr_info->object_next;
		if (ptr_info->object_next)
			ptr_info->object_next->object_prev = ptr_info->object_prev;
	}
	//pthread_mutex_unlock(&(obj_headnode->lock));

	ptr_info->object_next = ptr_info->object_prev = NULL;
	return;
}

static inline void
dang_insertnode_objlist(dang_objectnode_t *ptr_info, dang_headnode_t *obj_headnode)
{
	//pthread_mutex_lock(&(obj_headnode->lock));
	dang_objectnode_t *root_node = obj_headnode->root_node;
	if (root_node != NULL) {
		root_node->object_prev = ptr_info;
		ptr_info->object_next = root_node;
	}
	obj_headnode->root_node = ptr_info;
	//pthread_mutex_unlock(&(obj_headnode->lock));
	return;
}

/*
 * Try to register pointer with the object.
 */
static void
dang_registerptr(unsigned long ptr_addr, 
		unsigned long object_addr,
		dang_headnode_t *ptr_headnode, 
		dang_headnode_t *obj_headnode)
{
#ifdef DANG_STATS
        int     duplicate_ptr = 0;
#endif

	LOG_MSG3("%s ptr_addr=%lx object_addr=%lx\n", __func__, ptr_addr, object_addr);
	dang_objectnode_t *ptr_info = dang_getnode_hashtable(ptr_addr);
	if (ptr_info != NULL) { 	/* Pointer is poiting to valid object */
		/* TODO: Pointing to same object should be skipped */
		LOG_MSG2("%s remove ptr_info %p from object list \n", __func__, ptr_info);
#ifdef DANG_STATS  
                meta8 oldmetadata = metaget_8(ptr_info->value);
                dang_headnode_t *oldobj_headnode = (dang_headnode_t *)oldmetadata;
                if (obj_headnode == oldobj_headnode) {  /* Duplicate Pointer for the object */
                    ++obj_headnode->ptr_duplicate;
                    if ((obj_headnode->ptr_ctr - ptr_info->obj_ctr) <= DANG_NLOOKBEHIND) {
                        ++obj_headnode->lastptr_dup;
                    }
                    duplicate_ptr = 1;
                    //DANG_ASSERT("Duplicate Pointer " && 0);
                }
#endif
		dang_rmnode_objlist(ptr_info);
	} else if (obj_headnode != NULL) {			/* Allocate pointer information */
		LOG_MSG2("%s objnode %p not NULL\n", __func__, obj_headnode);
		ptr_info = dang_insertnode_hashtable(ptr_addr);
	}
	
	if (obj_headnode != NULL) { 	/* Valid object. Thus, insert pointer info */
		LOG_MSG1("%s obj_headnode not NULL insert\n", __func__);
		ptr_info->value = object_addr;
		dang_insertnode_objlist(ptr_info, obj_headnode);
#ifdef DANG_STATS
                if (!duplicate_ptr) {
                    ++obj_headnode->ptr_unique;
                }
                ptr_info->obj_ctr = obj_headnode->ptr_ctr++;
#endif
	} else if (ptr_info != NULL) { 	/* Invalid object. Thus, remove and delete pointer info */
		LOG_MSG2("%s free ptr_info %p\n", __func__, ptr_info);
		dang_rmnode_hashtable(ptr_info);
		DANG_FREE(ptr_info);
	}
	LOG_MSG1("%s END\n", __func__);
	return;
}

/*
 * Inline function to check whether pointer is valid or not. 
 */ 
void
inlinedang_registerptr(unsigned long ptr_addr, unsigned long object_addr)
{
        LOG_MSG3("%s track pointer %lx for object %lx\n", __func__,ptr_addr, object_addr);
        /* Check whether object_addr is already free'd */
        if (object_addr & 0xFFFF000000000000 || object_addr == 0) { /* TODO: Introduced for FreeSentry */
#ifdef DANG_STATS
            if (object_addr == 0) {
                ++_stats.object_null;
                DANG_ASSERT("Object is NULL" && 0);
            } else {
                ++_stats.object_freed;
            }
#endif
            return;
        }

        meta8   ptr_metadata = metaget_8(ptr_addr);
        dang_headnode_t *ptr_headnode = (dang_headnode_t *)ptr_metadata;
        if (ISVALID_HEADNODE(ptr_headnode)) {
                meta8   metadata = metaget_8(object_addr);
                dang_headnode_t *obj_headnode = (dang_headnode_t *)metadata;
                if (ISVALID_HEADNODE(obj_headnode) && !ISGLOBAL_OBJECT(obj_headnode)) {
                        dang_registerptr(ptr_addr, object_addr, ptr_headnode, obj_headnode);
#ifndef DANG_STATS
                }
#else
                } else {
                        if (ISGLOBAL_OBJECT(obj_headnode)) {
                            ++_stats.object_global;
                        } else {
                            ++_stats.object_invalid;
                        }
                }
#endif
        }

#ifdef DANG_STATS
        if (!ISVALID_HEADNODE(ptr_headnode)) {
            ++_stats.ptr_invalid;
        }
#endif
        return;
}

/*
 * Free object pointer list.
 * Called before free.
 */
static void
dang_freeptr(unsigned long obj_addr, unsigned long size)
{
	LOG_MSG2("%s START %lx\n", __func__, obj_addr);
	meta8 		metadata = metaget_8(obj_addr);
	dang_headnode_t *obj_headnode = (dang_headnode_t *)metadata;
#ifdef DANG_STATS
        unsigned long ptrlist_cnt = 0;
#endif

	LOG_MSG2("%s Object headnode %lx\n", __func__, obj_headnode);
	/* Take lock :
 	 * As object is getting freed no one should ask for the lock.
 	 * But if a pointer is pointing to this object and is beign 
 	 * assinged to new other object then register function might have to take
 	 * lock.
 	 */
	//pthread_mutex_lock(&(obj_headnode->lock));	
	dang_objectnode_t *ptr_info = obj_headnode->root_node;

	while (ptr_info != NULL) {
#ifdef DANG_STATS
                ++ptrlist_cnt;
#endif
		dang_rmnode_hashtable(ptr_info);
		meta8 		metadata1 = metaget_8((unsigned long)ptr_info->ptr);
		dang_headnode_t *headnode = (dang_headnode_t *)metadata1;
		if (ISVALID_HEADNODE(headnode) && headnode->id == ptr_info->id) {
			unsigned long current_value = *(unsigned long *)ptr_info->ptr;
			if (current_value == (unsigned long)ptr_info->value) {
				void  **ptr_addr = ptr_info->ptr;
				//*ptr_addr = DANG_NULL;
				*ptr_addr =  (void *)(ptr_info->value | DANG_NULL);
				LOG_MSG3("%s setting pointer %lx to DANG_NULL = %lx\n", __func__, ptr_info->ptr, ptr_info->value);
			}
		}
		dang_objectnode_t *temp_info = ptr_info;
		ptr_info = ptr_info->object_next;
		DANG_FREE(temp_info);
	}
	//pthread_mutex_unlock(&(obj_headnode->lock));    /* Quite a big critical region */

	//pthread_mutex_destroy(&(obj_headnode->lock));
	obj_headnode->magic = -1; 	                /* make object head node invalid */
#ifdef DANG_STATS
        dang_collect_avgstats(&_stats, ptrlist_cnt, obj_headnode->ptr_duplicate,
                                                    obj_headnode->ptr_unique,
                                                    obj_headnode->lastptr_dup);
#endif
	LOG_MSG1("%s END\n", __func__);
	return;
}

static void
dang_init_heapobj(unsigned long obj_addr, unsigned long size)
{
#ifdef DANG_STATS
        ++_stats.heap_alloc;
#endif
	dang_init_object(obj_addr, size, false);
	return;
}

/*
 * Get deepmetadata pointer and set ID and magic
 * So that object becomes valid.
 */
void
inlinedang_init_globalobj(unsigned long deepmeta_addr)
{
#ifdef DANG_STATS
        ++_stats.global_alloc;
#endif
	dang_headnode_t *head_node = (dang_headnode_t *)deepmeta_addr;
	/* TODO: Do we need lock here. */	
	head_node->id = dang_objectID;
	++dang_objectID;
	head_node->magic = DANG_MAGIC ^ head_node->id;
	return;
}

/*
 * Init object head node and return head node.
 * Called after malloc.
 */
__attribute__((always_inline))
static inline void
dang_init_object(unsigned long obj_addr, unsigned long size, bool is_global)
{
	meta8 	metadata = metaget_8(obj_addr);
	dang_headnode_t *head_node = (dang_headnode_t *)metadata;
	LOG_MSG3("%s START %lx headnode %lx\n", __func__, obj_addr, head_node);
	LOG_MSG2("%s allocated size %lu\n", __func__, size);
	//pthread_spin_lock(&dang_objectlock);
	head_node->id = is_global ? dang_objectID : (dang_objectID << GLOBAL_SHIFTBITS);
	++dang_objectID;
	//pthread_spin_unlock(&dang_objectlock);
	head_node->magic = DANG_MAGIC ^ head_node->id;
	head_node->root_node = NULL;
	//pthread_mutex_init(&(head_node->lock), NULL);
	
	LOG_MSG1("%s END\n", __func__);
	return;
}
