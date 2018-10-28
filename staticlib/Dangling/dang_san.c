/*
 * dang_san.c
 * Design is based on per-Thread log model.
 */
#include "metadata.h"
#include "dang_san.h"
#include "dsan_common.h"
#include <setjmp.h>
#include <signal.h>

#ifdef DANG_STATS
#include "dsan_stats.h"
#endif

#ifdef DANG_THREAD_ENABLE                               /* Multi-threading support */
#include "dsan_atomics.h"
static unsigned long threadglobal_id = 0;
static __thread unsigned long threadlocal_id = 0;       /* TODO: Check attribute initial-exec */
#define DANG_GET_THREADID(log_id) log_id = threadlocal_id;
#define DSAN_ATOMIC_CMPXCHNG_ONCE(ptr, old_value, new_value) \
            dsan_atomic_cmpxchng_once(ptr, old_value, new_value)

#else                                                   /* Single thread support */
#define DANG_GET_THREADID(log_id)
#define DSAN_ATOMIC_CMPXCHNG_ONCE(ptr, old_value, new_value)  \
            *ptr = new_value
#endif

#ifdef  DANG_NLOOKBEHIND
#define N_LOOKBEHIND    DANG_NLOOKBEHIND
#else
#define N_LOOKBEHIND    4
#endif

#define unlikely(x)     __builtin_expect((x),0)

/* Declare extern stack/global start and end */
extern long dang_global_start;
extern long dang_global_end;
extern __thread long dang_stack_start;
extern __thread long dang_stack_end;

/*
 * Shamelessly using per-thread
 * exception handling code snippet by : Drew Eckhardt
 */
struct thread_siginfo {
    jmp_buf jmp;
    unsigned int initialized : 1;
};

static __thread struct thread_siginfo thread_state;
struct sigaction sa_old;

static void
dang_segv_handler(int sig, siginfo_t *info, void *where)
{
    sigset_t set;
 
    sigemptyset(&set);
    sigaddset(&set, SIGSEGV);
    /* sigproc_mask for single-threaded programs */
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
 
    if (thread_state.initialized) {
        LOG_MSG("SIGSEGV during dang_freeptr\n");
        longjmp(thread_state.jmp, 1);
    } else {
        /* old handler may be SIG_DFL which precludes a direct
         * call
         */
        sigaction(SIGSEGV, &sa_old, NULL);
        raise(SIGSEGV);
    }
}
 
static inline
dang_objlog_t *dang_alloc_threadlog(dang_objlog_t *prev_log, unsigned long thread_id)
{
    dang_objlog_t       *obj_loglocal;

    /* Allocate log buffer and append it.
     */
    //obj_loglocal = (dang_objlog_t *)DANG_MALLOC(OBJLOG_SIZE);
    DANG_MALLOC(obj_loglocal, dang_objlog_t *, OBJLOG_SIZE);
    DANG_GET_THREADID(obj_loglocal->thread_id);
    //obj_loglocal->thread_id = thread_id;
    obj_loglocal->next_log = NULL;
    obj_loglocal->index = 0;
    obj_loglocal->end = 0;
    obj_loglocal->count = 0;
    obj_loglocal->size = OBJLOG_MAX_PTRS;
    obj_loglocal->next_growlog = NULL;
  
#ifdef DANG_THREAD_ENABLE 
    if (prev_log) {
        obj_loglocal->next_log = (dang_objlog_t *)
                    dsan_atomic_cmpxchng((unsigned long*)&prev_log->next_log, 
                    (unsigned long)obj_loglocal);
    }
#endif

    return obj_loglocal;
}

/* 
 * Consider following pointer storage format to insert pointer. 
 * N-Lookbehind may match N*3 pointers and thereby, reducing total size by 3.
 *  ________________________________________________
 * | 40 bits Pointer Prefix | byte1 | byte2 | byte3 |
 *  ------------------------------------------------
 */
__attribute__((always_inline))
static inline int
dang_insertlogptr(unsigned long ptr_addr, dang_objlog_t *obj_loglocal, unsigned long index)
{
    unsigned long ptr_prefix = ptr_addr >> 8;
    unsigned long ptr_byte = ptr_addr & 0xFF;
    unsigned long n_index = index, n_byte1 = ptr_byte, n_byte2 = 0x00, n_byte3 = 0x00;
    unsigned long *log_buf = &((unsigned long*)obj_loglocal)[OBJLOG_START_IDX];
    
    /* Iterate for last N_LOOKBEHIND entries */
    for (int i = 1; i <= N_LOOKBEHIND && ((int)index - i) >= 0; ++i) {
        unsigned long log_prefix = log_buf[index - i] >> 24;
        if (ptr_prefix == log_prefix) {
            /* Prefixed matched. Match the byte */
            unsigned long byte1 = (log_buf[index - i] >> 16) & 0xFF;
            unsigned long byte2 = (log_buf[index - i] >> 8) & 0xFF;
            unsigned long byte3 = log_buf[index - i] & 0xFF;
            if (ptr_byte == byte1) {
#ifdef DANG_STATS
                ++dang_dup_ptr;
#endif
                return 0;
            } else if (ptr_byte == byte2) {
                if (ptr_byte) {
#ifdef DANG_STATS
                    ++dang_dup_ptr;
#endif
                    return 0;
                }
                n_byte1 = ptr_byte;
                n_byte2 = byte1;
                n_byte3 = byte3;
                n_index = index - i;
            } else if (!byte2) {
                n_byte1 = byte1;
                n_byte2 = ptr_byte;
                n_byte3 = byte3;
                n_index = index - i;
            } else if (ptr_byte == byte3) {
                if (ptr_byte) {
#ifdef DANG_STATS
                    ++dang_dup_ptr;
#endif
                    return 0;
                }
                n_byte1 = ptr_byte;
                n_byte2 = byte2;
                n_byte3 = byte1;
                n_index = index - i;
            } else if (!byte3) {
                n_byte1 = byte1;
                n_byte2 = byte2;
                n_byte3 = ptr_byte;
                n_index = index - i;
            }
        }
    }
 
    //obj_loglocal->count += 1;
    log_buf[n_index] = ((ptr_prefix << 24) | (n_byte1 << 16) | (n_byte2 << 8) | n_byte3);
    return n_index;
}

#ifdef HASHTABLE_GROW
/*
 * Consider following pointer storage to store pointer in the hashtable.
 *  ________________________________________________
 * | 32 bits Pointer prefix | byte1 | byte2 | byte3 |
 *  ------------------------------------------------
 */
__attribute__((always_inline))
static inline void
dang_inserthashptr(unsigned long ptr_addr, dang_hashnode_t **hash_node)
{
    unsigned long ptr_prefix = ptr_addr >> 16;
    unsigned long ptr_suffix = ptr_addr & 0xFFFF;
    unsigned long n_suffix1 = ptr_suffix, n_suffix2 = 0x0000;
    dang_hashnode_t *ptr_prev = NULL, *n_ptrnode = NULL;
    dang_hashnode_t *ptr_tempnode = *hash_node;
    while (ptr_tempnode) {
        ptr_prev = ptr_tempnode;
        unsigned long hashptr_prefix = ptr_tempnode->ptr >> 32;
        unsigned long hashptr_suffix1 = (ptr_tempnode->ptr >> 16) & 0xFFFF;
        unsigned long hashptr_suffix2 = (ptr_tempnode->ptr & 0xFFFF);
        //printf("stored_ptr = %lx, ptr_prefix = %lx, ptr_suffix = %lx, hashptr_prefix = %lx," 
        //              " hashptr_suffix1 = %lx, hashptr_suffix2 = %lx\n", 
        //               ptr_tempnode->ptr, ptr_prefix, ptr_suffix, hashptr_prefix, 
        //               hashptr_suffix1, hashptr_suffix2);
        if (ptr_prefix == hashptr_prefix) {
            if (ptr_suffix == hashptr_suffix1) {
                return;
            } else if (ptr_suffix == hashptr_suffix2) {
                if (ptr_suffix)
                    return;
                n_ptrnode = ptr_tempnode;
                n_suffix1 = ptr_suffix;
                n_suffix2 = hashptr_suffix1;
                break;
            } else if (!hashptr_suffix2) {
                n_ptrnode = ptr_tempnode;
                n_suffix1 = hashptr_suffix1;
                n_suffix2 = ptr_suffix;
                break;
            }
        }
        ptr_tempnode = ptr_tempnode->next;
    }
    
    if (!n_ptrnode) {
        DANG_MALLOC(n_ptrnode, dang_hashnode_t *, sizeof(dang_hashnode_t));
        assert(n_ptrnode != NULL && "Pointer node is NULL!!");
        n_ptrnode->next = NULL;
        if (ptr_prev) {
            ptr_prev->next = n_ptrnode;
        } else {
            *hash_node = n_ptrnode;
        }
    }
    n_ptrnode->ptr = (ptr_prefix << 32) | (n_suffix1 << 16) | (n_suffix2);
    return;
}
#endif

#define dang_checkptr_value(ptr, obj_start, obj_end)            \
{                                                               \
    if (!setjmp(thread_state.jmp)) {                            \
        unsigned long ptr_value = *ptr;                         \
        if (ptr_value >= obj_start && ptr_value < obj_end) {    \
            break;                                              \
        }                                                       \
    }                                                           \
}

/*
 * Clear pointers that do not point the object anymore. 
 * Every log slot can have 1-3 pointers. All sequential slots of 
 * pointers that do not point to the objects are cleared.
 */
__attribute__((always_inline))
static void inline
dang_clear_objlog(dang_objlog_t *obj_loglocal, unsigned long obj_start, unsigned long obj_end)
{
    unsigned long end = obj_loglocal->end;
    long count = obj_loglocal->count;
    unsigned long *log_buf = (unsigned long*)&((unsigned long*)obj_loglocal)[OBJLOG_START_IDX];
    struct sigaction sa_new;
#ifdef DANG_STATS
    ++dang_log_full;
    unsigned long old_end = end;
#endif
    
    sa_new.sa_sigaction = dang_segv_handler;
    sa_new.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa_new, &sa_old);
    
    thread_state.initialized = 1;
    while (count > 0) {
        unsigned long ptr_prefix = log_buf[end] >> 24;
        if (!ptr_prefix) {      /* Empty slot left during reallocation */
            end =  (end + 1) & (obj_loglocal->size - 1);
            --count;
            continue;
        }
        unsigned long byte = (log_buf[end] >> 16) & 0xFF;
        unsigned long *ptr = (unsigned long *)(ptr_prefix << 8 | byte);

        dang_checkptr_value(ptr, obj_start, obj_end);   /* It has break */
        
        byte = (log_buf[end] >> 8) & 0xFF;
        if (!byte) {
            end =  (end + 1) & (obj_loglocal->size - 1);
            --count;            /* Only one pointer in the slot */
            continue;
        }
        ptr = (unsigned long *) (ptr_prefix << 8 | byte);
        dang_checkptr_value(ptr, obj_start, obj_end);

        byte = log_buf[end] & 0xFF;
        if (byte) {
            ptr = (unsigned long *) (ptr_prefix << 8 | byte);
            dang_checkptr_value(ptr, obj_start, obj_end);
            //count -= 3;         /* Three pointers in the slot */
        } else {
            //count -= 2;         /* Two pointers in the slot */
        }
        end =  (end + 1) & (obj_loglocal->size - 1);
        --count;
    }
    thread_state.initialized = 0;
    sigaction(SIGSEGV, &sa_old, NULL);
     
    //printf("Cleared %lx pointers\n", obj_loglocal->count - count);
    obj_loglocal->count = count;
    obj_loglocal->end = end;
#ifdef DANG_STATS
    if (end != old_end) {
        dang_stats_ptr_pattern(log_buf, obj_loglocal->size, old_end, end);
    }
#endif
    return;
}

static void
dang_register(unsigned long ptr_addr, unsigned long obj_addr, dang_objlog_t *obj_log, unsigned long obj_bounds)
{
#ifdef DANG_STATS
    ++dang_ptr_insert;
#endif

    dang_objlog_t *obj_loglocal;
    dang_objlog_t *obj_logtemp = obj_log;

#ifdef DANG_THREAD_ENABLE
    dang_objlog_t *obj_logprev = NULL;
    /* Retrieve thread ID. Initialize thread ID, if not initialized */
    if (unlikely(!threadlocal_id)) {
        /* Give thread ID atomically */
        threadlocal_id = dsan_atomic_add(&threadglobal_id, 1);
    }
    LOG_MSG2("%s thread %lx entered \n", __func__, threadlocal_id);
    
    while (obj_logtemp && obj_logtemp->thread_id != threadlocal_id) { /* TODO: Thread Init can be made efficient */
        obj_logprev = obj_logtemp;
        obj_logtemp = obj_logtemp->next_log;
    }
    
    if (unlikely(obj_logtemp == NULL)) {
        obj_loglocal = dang_alloc_threadlog(obj_logprev, threadlocal_id);
        LOG_MSG3("%s:%lx log buffer allocated %p\n", __func__, threadlocal_id, obj_loglocal);
    } else {
        obj_loglocal = obj_logtemp;
        LOG_MSG3("%s:%lx found log bufer %p\n", __func__, threadlocal_id, obj_loglocal);
    }

#else
    obj_loglocal = obj_logtemp;
#endif

    dang_objlog_t *obj_logbase = obj_loglocal;
    if (obj_logbase->next_growlog) {
        obj_loglocal = obj_logbase->next_growlog;
    }

    unsigned long threshold = (unsigned long)(OBJLOG_THRESHOLD_PER * obj_loglocal->size);
    if (unlikely(obj_loglocal->count >= threshold)) {   /* TODO: Weigh unlikely properly */
       //printf("HEre count %lx, %lx\n", slots, threshold);
        unsigned long obj_start = (obj_addr & 0xFFFFFFFF00000000) | (obj_bounds >> 32);
        unsigned long obj_size = obj_bounds & 0xFFFFFFFF;
        unsigned long obj_end = obj_start + obj_size; 
        //unsigned long slots = obj_loglocal->count;
        dang_clear_objlog(obj_loglocal, obj_start, obj_end);
        //printf("Slots freed %lu\n", slots - obj_loglocal->count);
        //assert("Log Full" && 0);
        if (obj_loglocal->count >= obj_loglocal->size) {
#ifdef DANG_STATS
        /*
        if (!obj_loglocal->hash_table) {
            unsigned long obj_size = 1 << (metabaseget(obj_addr) & 0xFF);
            dang_stats_collect((unsigned long *)obj_loglocal, obj_size, obj_addr);
        }
        */
#endif
            if (!obj_logbase->next_growlog) {
#ifdef DANG_STATS
                /* Collect stats for the perlbench */
                unsigned long obj_size = 1 << (metabaseget(obj_addr) & 0xFF);
                dang_stats_collect((unsigned long *)obj_loglocal, obj_size, obj_addr);
#endif

#ifdef  DANG_THREAD_ENABLE
                obj_loglocal = dang_alloc_threadlog(NULL, threadlocal_id);
#else 
                obj_loglocal = dang_alloc_threadlog(NULL, 0); 
#endif
            } else {
                /* XXX: Highly unsyrnchornized buggy code. But did it for
                 * SPEC2006. Mistalkenly below code got deleted. Thus rewrote it
                 * but not tested.
                 */
                dang_objlog_t *obj_logold = obj_loglocal;
                DANG_REALLOC(obj_loglocal, obj_logold, dang_objlog_t *, 
                                ((OBJLOG_START_IDX + (obj_loglocal->size << 1)) << 3));
                assert(obj_loglocal != NULL && "Realloc object is NULL!!");
#ifdef TC_ALLOC
                if (obj_loglocal != obj_logold) {         /* Pointers are different */
                    assert(malloc_flag == false && "Malloc flag is still true!!");
                } else {
                    /* Pointers are same. Thus, TCMalloc has not called malloc and
                     * free. Thus, remove flags.
                     */
                    malloc_flag = false;
                    free_flag = false;
                }
#endif
                //unsigned long *log_buf = (unsigned long*)&((unsigned long*)obj_loglocal)[OBJLOG_START_IDX];
                //log_buf[obj_loglocal->index] = 0; /* This is required to skip count update during free */
                obj_loglocal->index = obj_loglocal->size;
                obj_loglocal->end = 0;
                obj_loglocal->size = obj_loglocal->size << 1;
            }

            obj_logbase->next_growlog = obj_loglocal;
        } /* Log full */
    }

    if (dang_insertlogptr(ptr_addr, obj_loglocal, obj_loglocal->index) ==
                            obj_loglocal->index) {
        obj_loglocal->index = (obj_loglocal->index + 1) & (obj_loglocal->size - 1);
        obj_loglocal->count += 1;
    }

    LOG_MSG3("%s:%lx pointer stored in buffer %lx\n", __func__, threadlocal_id, ptr_addr); 
    return;
}

/*
 * Inline function which will first check the obj_addr validity. 
 * Globals will have 0 metadata. TODO: stack variables currently are not tracked. 
 * Thus, their metadata will be 0. We are tracking all pointers (stored in globals,
 * heap and stack).
 */
void
inlinedang_registerptr(unsigned long ptr_addr, unsigned long obj_addr)
{
#ifdef DANG_STATS
    ++dang_ptr_reg;
#endif

    LOG_MSG3("%s tracking pointer %lx and object %lx\n", __func__, ptr_addr, obj_addr);
    if (obj_addr == 0 || obj_addr & DANG_NULL) {
        LOG_MSG1("%s object is 0 or freed already \n", __func__);
        return;
    }
    
    /* Check for stack and global objects */
    if ((obj_addr >= dang_stack_start && obj_addr <= dang_stack_end)  ||
            (obj_addr >= dang_global_start && obj_addr <= dang_global_end)) {
        LOG_MSG1("%s object is either global or stack \n", __func__);
        return;
    }

    meta16 obj_metadata = metaget_16(obj_addr);
    if (!obj_metadata.a) {
        LOG_MSG2("%s obj_addr %lx metadata is 0\n", __func__, obj_addr);
        return;
    }

#ifndef TRACK_ALL_PTRS
    if ((ptr_addr >= dang_stack_start && ptr_addr <= dang_stack_end)  ||
            (ptr_addr >= dang_global_start && ptr_addr <= dang_global_end)) {
        LOG_MSG1("%s pointer is either global or stack \n", __func__);
        return;
    }

    /*
    meta16 ptr_metadata = metaget_16(ptr_addr);
    if (!ptr_metadata.a) {
        return;
    }
    */
#endif

    /* Skip pointers that were pointing to the same object */
    unsigned long obj_start = (obj_addr & 0xFFFFFFFF00000000) | (obj_metadata.b >> 32);
    unsigned long obj_size = obj_metadata.b & 0xFFFFFFFF;
    unsigned long obj_end = obj_start + obj_size;
    unsigned long obj_oldaddr = *(unsigned long *)ptr_addr;
    if (obj_oldaddr >= obj_start && obj_oldaddr < obj_end) {
        LOG_MSG3("%s pointer %lx and obj %lx seems to be registered already \n",
            __func__, ptr_addr, obj_addr);
#ifdef DANG_STATS
        ++dang_dup_ptr;
#endif
        return;
    }

    dang_register(ptr_addr, obj_addr, (dang_objlog_t *)obj_metadata.a, obj_metadata.b);
    return;
}

#define dang_nullifyptr(ptr, obj_addr, obj_end)                                         \
{                                                                                       \
    /* TODO: Check how come this pointer is DANG_NULL */                                \
    if (((unsigned long) ptr & DANG_NULL) == 0 && !setjmp(thread_state.jmp)) {          \
        unsigned long ptr_value = *ptr;                                                 \
        if (ptr_value >= obj_addr && ptr_value < obj_end) {  /* Optimize this check */  \
            DSAN_ATOMIC_CMPXCHNG_ONCE(ptr, ptr_value, ptr_value | DANG_NULL);           \
        }                                                                               \
    }                                                                                   \
}

void
dang_freeptr(unsigned long obj_addr, unsigned long size)
{

#ifdef TC_ALLOC
    if (free_flag) {
        free_flag = false;
        return;
    }
#endif

#ifdef DANG_STATS
    ++dang_obj_free;
#endif

    meta16 metadata = metaget_16(obj_addr);
    dang_objlog_t *obj_loglocal = (dang_objlog_t *)metadata.a;
    dang_objlog_t *obj_logtemp;
    unsigned long *log_buffer;
    unsigned long obj_end = obj_addr + size;
    struct sigaction sa_new;

    sa_new.sa_sigaction = dang_segv_handler;
    sa_new.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa_new, &sa_old);
 
    thread_state.initialized = 1;
    while (obj_loglocal) {
        dang_objlog_t *obj_threadlocal = obj_loglocal->next_log;
        while (obj_loglocal) {
            log_buffer = (unsigned long *)&((unsigned long *)obj_loglocal)[OBJLOG_START_IDX];
#ifdef DANG_STATS
        /* Collect stats only for the objects which has log overflow */
        /*if (obj_loglocal->hash_table) {
            dang_stats_collect(log_buffer, size);
        }*/
#endif
            /* First handle static log buffer */
            unsigned long end = obj_loglocal->end;
            long count = obj_loglocal->count;
            //assert("Count is greater than expected!" && count <= obj_loglocal->size);
            //printf("Count %d %lu\n", count > obj_loglocal->size, count);
            while (count > 0) {
                unsigned long ptr_prefix = log_buffer[end] >> 24;
                if (!ptr_prefix) {  /* Empty slot left during reallocation */
                    end = (end + 1) & (obj_loglocal->size - 1);
                    --count;
                    continue;
                }
                unsigned long byte = (log_buffer[end] >> 16) & 0xFF;
                unsigned long *ptr = (unsigned long *)(ptr_prefix << 8 | byte); 
                dang_nullifyptr(ptr, obj_addr, obj_end);

                byte = (log_buffer[end] >> 8) & 0xFF;
                if (!byte) {
                    end = (end + 1) & (obj_loglocal->size - 1);    
                    --count;
                    continue;
                }
                ptr = (unsigned long *) (ptr_prefix << 8 | byte);
                dang_nullifyptr(ptr, obj_addr, obj_end);

                byte = log_buffer[end] & 0xFF;
                if (byte) {
                    ptr = (unsigned long *) (ptr_prefix << 8 | byte);
                    dang_nullifyptr(ptr, obj_addr, obj_end); 
                    //count -= 3;
                } else {
                    //count -= 2;
                }
                end = (end + 1) & (obj_loglocal->size - 1);
                --count;
            }
            obj_logtemp = obj_loglocal;
            obj_loglocal = obj_loglocal->next_growlog;    
            DANG_FREE(obj_logtemp);
        }
        //obj_logtemp = obj_loglocal;
        obj_loglocal = obj_threadlocal;
        //DANG_FREE(obj_logtemp);
    }
    thread_state.initialized = 0;
    sigaction(SIGSEGV, &sa_old, NULL);
    
    /* Set metadata to NULL. PerlBEnch has issue. See gperftools
     * tcmalloc.cc for more information.
     */
    metadata.a = 0;
    metadata.b = 0; 
    metaset_16(obj_addr, size, metadata);
    /* TODO: Efficient zero out.
     * Freed object may get reallocated. And another thread might be writing to this
     * uniitialized pointer location. which will race with the free of other object
     */
    return;
}

void
dang_init_heapobj(unsigned long obj_addr, unsigned long size)
{
    //printf("Allocated Object %lu : Size %lu\n", obj_addr, size);
#ifdef TC_ALLOC
    /* DSAN uses TCmalloc to allocate internal log buffers.
     * Avoid recursion tracking for internal allocations.
     * TODO: This can be done in TCmalloc which will save one internal
     * function call.
     */
    if (malloc_flag) {
        malloc_flag = false;
        return;
    }
#endif

#ifdef DANG_THREAD_ENABLE
    if (unlikely(!threadlocal_id)) {
        threadlocal_id = dsan_atomic_add(&threadglobal_id, 1);
    }
#endif
    
    /* Allocate thread log */
    dang_objlog_t       *obj_loglocal;
    DANG_MALLOC(obj_loglocal, dang_objlog_t *, OBJLOG_SIZE);
    DANG_GET_THREADID(obj_loglocal->thread_id);
    //obj_loglocal->thread_id = threadlocal_id;
    obj_loglocal->next_log = NULL;
    obj_loglocal->index = 0;    /* TODO:index, end can go in one entry and 
                                 * count, size in another entry */
    obj_loglocal->end = 0;
    obj_loglocal->count = 0;
    obj_loglocal->size = OBJLOG_MAX_PTRS;
    obj_loglocal->next_growlog = NULL;
    /* Set metadata value TODO: TCmalloc hook init's it with zero. Skip that hook */
    meta16 metadata;
    metadata.a = (uint64_t)obj_loglocal;
    metadata.b = (obj_addr & 0xFFFFFFFF) << 32;
    metadata.b |= size;
    metaset_16(obj_addr, size, metadata);

#ifdef DANG_STATS
//    fprintf(stderr, "Object allocated : %lx, size %lu, logbuffer %p\n", 
  //                          obj_addr, size, obj_loglocal);
    ++dang_obj_alloc;
    //dang_print_strace();
#endif
    return;
}
