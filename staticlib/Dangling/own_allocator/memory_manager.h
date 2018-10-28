/*
 * memory_manager.h
 * 	Author - Vinod
 */

#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#define DANG_POOLBITS   34                      /* 16 MB initial pool size */                             
#define DANG_POOLSIZE   ((long)1 << DANG_POOLBITS)                                                              
//#define DANG_ALLOCBITS  6                       /* 64 bytes allocation unit */
//#define DANG_ALLOCSIZE  (1 << DANG_ALLOCBITS)
//#define DANG_ALLOCMASK  (~(DANG_ALLOCSIZE - 1))

/*                                                                                                        
 * Structure to hold memory pool control information.                                                     
 */ 
typedef struct dang_mempool     dang_mempool_t;                                                           
struct dang_mempool {                                                                                     
        void            	*start_addr;            /* Start address of the mem pool */       
        void            	*aligned_start_addr;    /* Start address aligned to DANG_ALLOCSIZE */
        size_t          	free_start_blk;         /* First free block */
        size_t          	blk_count;              /* Number of blocks */
        size_t          	free_list_blk;          /* Free list */
	pthread_spinlock_t 	lock; 			/* Spin lock */
        dang_mempool_t  	*next_mempool;          /* Next pool of fixed size unit */
        size_t                  alloc_bits;             /* Unit size of allocation, 2^alloc_bits */
};

//extern dang_mempool_t  *dang_mempool_info;
extern void dang_mempool_init(dang_mempool_t **dang_mempool_info, size_t alloc_bits);
extern void *dang_malloc(dang_mempool_t *dang_mempool_info);
extern void dang_free(dang_mempool_t *dang_mempool_info, void *ptr);
