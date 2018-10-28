#ifndef DSAN_COMMON_H
#define DSAN_COMMON_H

#include "metapagetable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DANG_DEBUG
#define LOG_MSG(f)                      fprintf(stderr, f); fflush(stderr)
#define LOG_MSG1(f, arg1)               fprintf(stderr, arg1); fflush(stderr)
#define LOG_MSG2(f, arg1, arg2)         fprintf(stderr, f, arg1, arg2); fflush(stderr)
#define LOG_MSG3(f, arg1, arg2, arg3)   fprintf(stderr, arg1, arg2, arg3); fflush(stderr)
#define PRINT_STRACE                    dang_print_strace(); fflush(stdout)
#define DANG_ASSERT(exp)                assert(exp)
#else

#include <execinfo.h>
#define BT_BUF_SIZE 100
static int dang_temp = 0;
void
dang_print_strace()
{
    int j, nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;

    /* TODO: remove this. This is to avoid recurssion in tcmalloc do_free */
    dang_temp = 1;

    nptrs = backtrace(buffer, BT_BUF_SIZE);
    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        printf("No Stack trace generated\n");
        return;
    }
   
    printf("Stack Trace: \n");
    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);
    free(strings);
    dang_temp = 0;
    return;
}

//#else
#define LOG_MSG(f)
#define LOG_MSG1(f, arg1) 
#define LOG_MSG2(f, arg1, arg2)
#define LOG_MSG3(f, arg1, arg2, arg3)
#define PRINT_STRACE
#define DANG_ASSERT(exp)
#endif

/* dang_malloc and dang_free interface is not consistent across different
 * memory allocator.
 * TODO: make it consistent
 */
#if defined(SANITIZER_ALLOC)
#include "san_malloc.h"
#define DANG_MALLOC(ptr, type, size)       \
        ptr = (type) dang_malloc(size)
#define DANG_FREE(ptr)          dang_free(ptr)
#define DANG_REALLOC(new_ptr, old_ptr, type, size)

#elif  defined(TC_ALLOC)
static __thread bool malloc_flag;
static __thread bool free_flag;
#define DANG_MALLOC(ptr, type, size)       \
        malloc_flag = true;                \
        ptr = (type) malloc(size)        
/* TODO: Add assert after malloc call to chech the malloc_flag = false */
#define DANG_FREE(ptr)          \
        free_flag = true, free(ptr)
#define DANG_REALLOC(new_ptr, old_ptr, type, size) \
        malloc_flag = true;                            \
        free_flag = true;                               \
        new_ptr = (type) realloc(old_ptr, size)

#else   /* MemPool */
#include "memory_manager.h"
static dang_mempool_t           *dang_mempool_info = NULL;      /* Pointer info mempool */
#define DANG_ALLOCBITS          14 
#define DANG_MALLOC(ptr, type, size)       \
        ptr = (type)dang_malloc(dang_mempool_info)
#define DANG_FREE(ptr)          dang_free(dang_mempool_info, ptr)
#define DANG_REALLOC(new_ptr, old_ptr, type, size)

static dang_mempool_t           *dang_ptrpool_info = NULL;      /* Memory pool for small objects */
#define DANG_ALLOCPTR_BITS      4
#define DANG_PTRMALLOC(ptr, type, size)       \
        ptr = (type)dang_malloc(dang_ptrpool_info)
#define DANG_PTRFREE(ptr)       dang_free(dang_ptrpool_info, ptr)

#endif

#define DANG_NULL               (unsigned long)0x8000000000000000

extern void dang_freeptr(unsigned long obj_adr, unsigned long size);
extern void dang_init_heapobj(unsigned long obj_addr, unsigned long size);

int
dang_get_stack() {
    int i;
    printf("Address in proper stack %p", &i);
    return 0;
}

/* 
 * Initialize function to allocatee mempool, hashtable or any other memory requirement.
 */
__attribute__((visibility ("hidden")))
void
dang_initialize(void)
{
        /* Init Global lock */
        //pthread_spin_init(&dang_objectlock, PTHREAD_PROCESS_PRIVATE);
        
        //dang_get_stack();
        //printf("Address in init array %d", 0);
        
        /* Set malloc post hook */
        metalloc_malloc_posthook = dang_init_heapobj;

        /* Set free pre hook */
        metalloc_free_prehook = dang_freeptr;

#if !(defined(SANITIZER_ALLOC) || defined(TC_ALLOC))
        /* Init fixed sized mempool */
        if (dang_mempool_info == NULL) {
                dang_mempool_init(&dang_mempool_info, DANG_ALLOCBITS);
        }
        if (dang_ptrpool_info == NULL) {
            dang_mempool_init(&dang_ptrpool_info, DANG_ALLOCPTR_BITS);
        }
#endif

        /* Allocate hash table */
        //dang_alloc_hashtable();
        return;
}

#endif /* !DSAN_COMMON_H */
