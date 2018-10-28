/*
 * memory_manager.c
 * 	Implements fixed-size memory pool manager.
 * 	Author - Vinod
 */
#include <sys/mman.h>
#include <stdlib.h>
#include "memory_manager.h"

#define unlikely(x)     __builtin_expect((x),0)

//dang_mempool_t 	*dang_mempool_info = NULL;

/*
 * Allocate memory pages and aligned it on the DANG_ALLOCSIZE
 * boundary. Initialize dang_mempool_t structure.
 * TODO: Pass max DANG_POOLSIZE also.
 */
void
dang_mempool_init(dang_mempool_t **dang_mempoolinfo, size_t alloc_bits)
{
	dang_mempool_t 	*dang_mempool_info = *dang_mempoolinfo;

	if (unlikely(!dang_mempool_info)) {
		dang_mempool_info = mmap(NULL, DANG_POOLSIZE, PROT_READ | PROT_WRITE, 
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (dang_mempool_info == MAP_FAILED) {
			perror("Could not allocate memory pool");
			exit(EXIT_FAILURE);
		}
	}
        size_t alloc_size = 1 << alloc_bits;
        size_t alloc_mask = ~(alloc_size - 1);	

	/* Init mempool info at the start of the mempool */
        dang_mempool_info->alloc_bits = alloc_bits;
	dang_mempool_info->start_addr = dang_mempool_info;
	dang_mempool_info->aligned_start_addr = (void *)(((unsigned long)dang_mempool_info +
				sizeof(dang_mempool_t) + alloc_size - 1) & alloc_mask);
	dang_mempool_info->free_start_blk = 0;
	unsigned long aligned_end_addr = (((unsigned long)dang_mempool_info +
					DANG_POOLSIZE) & alloc_mask);
	dang_mempool_info->blk_count = ((size_t)(aligned_end_addr -
			(unsigned long)dang_mempool_info->aligned_start_addr)) >> alloc_bits;
	dang_mempool_info->free_list_blk = -1;
	pthread_spin_init(&(dang_mempool_info->lock), PTHREAD_PROCESS_PRIVATE); /* TODO: Destroy should in .fini .*/
	dang_mempool_info->next_mempool = NULL;
	*dang_mempoolinfo = dang_mempool_info;
	return;
}

/*
 * Chunk allocation routine.
 * TODO: We do not need size info here.
 * dang_mempool_t *dang_mempool_info : This corresponds to particular
 * size. Wrapper above can be written to pass particular mempool info.
 */
void *
dang_malloc(dang_mempool_t *dang_mempool_info)
{
	void 	*addr;
	
	pthread_spin_lock(&(dang_mempool_info->lock));
	if (dang_mempool_info->free_start_blk < dang_mempool_info->blk_count) {
		addr = dang_mempool_info->aligned_start_addr +
				(dang_mempool_info->free_start_blk << dang_mempool_info->alloc_bits);
		dang_mempool_info->free_start_blk += 1;
	} else if (dang_mempool_info->free_list_blk == -1) {
		/* TODO: Mempool empty. Grow pool */
		addr = NULL;
	} else {
		addr = dang_mempool_info->aligned_start_addr +
				(dang_mempool_info->free_list_blk << dang_mempool_info->alloc_bits);
		dang_mempool_info->free_list_blk = *(size_t *)addr;
	}
	pthread_spin_unlock(&(dang_mempool_info->lock));
	return addr;
}

/*
 * Free allocation routine.
 * dang_mempool_t *dang_mempool_info : This corresponds to particular
 * size. Wrapper above can be written to pass particular mempool info.
 */
void
dang_free(dang_mempool_t *dang_mempool_info, void *ptr)
{
	size_t 	*temp_ptr = ptr;
	
	/* Biggest assumption : ptr is poiting correctly in the correct mempool */
	pthread_spin_lock(&(dang_mempool_info->lock));
	*temp_ptr = dang_mempool_info->free_list_blk;
	size_t blk = (size_t)((ptr - dang_mempool_info->aligned_start_addr) >> dang_mempool_info->alloc_bits);
	dang_mempool_info->free_list_blk = blk;
	pthread_spin_unlock(&(dang_mempool_info->lock));
	
	/* TODO : Statistic to release Grown memory pool */
	return;
}
