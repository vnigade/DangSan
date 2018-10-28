#ifndef DSAN_STATS_H
#define DSAN_STATS_H

#define DANG_STATS_FILE  "DANG_STATS_FILE" 
#define DANG_STATS_OBJS  1000
static FILE *fp = NULL;
static int counter = 0;
 
__attribute__((always_inline))
void
dang_stats_printlog(unsigned long *logbuf, unsigned long index)
{
    fprintf(stderr, "Printing log %p..\n", logbuf);
    for (unsigned int i = 0; i < index; ++i) {
        fprintf(stderr, "Ptr = %lx\n", logbuf[i]);
    }
    return;
}

static inline bool
dang_stats_inserthash(dang_hashnode_t **hash_table, unsigned long ptr)
{
    unsigned int index = ((ptr >> 3) & (OBJLOG_MAX_PTRS - 1));
    dang_hashnode_t *ptr_node = hash_table[index];
    dang_hashnode_t *ptr_nodeprev = NULL;

    while (ptr_node) {
        ptr_nodeprev = ptr_node;
        if (ptr_node->ptr == ptr) {
            return false;
        }
        ptr_node = ptr_node->next;
    }
    
    DANG_MALLOC(ptr_node, dang_hashnode_t *, sizeof(dang_hashnode_t));
    ptr_node->ptr = ptr;
    ptr_node->next = NULL;
    if (ptr_nodeprev) {
        ptr_nodeprev->next = ptr_node;
    } else {
        hash_table[index] = ptr_node;
    }
    return true;
}

static inline void
dang_stats_freehash(dang_hashnode_t **hash_table)
{
    for (unsigned long i = 0; i < OBJLOG_MAX_PTRS; i++) {
        dang_hashnode_t *ptr_node = hash_table[i];
        while (ptr_node) {
            dang_hashnode_t *ptr_nodetemp = ptr_node;
            ptr_node = ptr_node->next;
            DANG_FREE(ptr_nodetemp);
        }
    }
    DANG_FREE(hash_table);
    return;
}
 
void
dang_stats_collect(unsigned long *logbuf, unsigned long obj_size, unsigned long obj_addr)
{
    if (counter >= DANG_STATS_OBJS) {
        return;
    }
    ++counter;

    if (!fp) {
        char *file_path = getenv(DANG_STATS_FILE);
        fp = fopen(file_path, "w"); 
        if (fp == NULL) {
            fprintf(stdout, "Cannot open stats file! \n");
            abort();
        }
        fprintf(fp, "Object_size, Total_Ptrs, Uniq_Ptrs, Duplicate_Ptrs\n");
    }
    unsigned long total_ptrs = 0, uniq_ptrs = 0, valid_ptrs = 0;
    unsigned long obj_end = obj_addr + obj_size;
   
    /* Using hash table to find unique and duplicate pointers
     * Algorithm is O(N) with some memory overhead.
     */ 
    //dang_stats_printlog(logbuf, OBJLOG_MAX_PTRS);
    dang_hashnode_t **hash_table;
    DANG_MALLOC(hash_table, dang_hashnode_t **, OBJLOG_SIZE); 
    memset(hash_table, 0, OBJLOG_SIZE); 
    for (unsigned int i = OBJLOG_START_IDX; i < OBJLOG_MAX_PTRS; ++i) {
        unsigned long ptr_prefix = logbuf[i] >> 24;
        unsigned long byte = (logbuf[i] >> 16) & 0xFF; 
        unsigned long ptr = (ptr_prefix << 8 | byte);
        unsigned long ptr_value = *(unsigned long*)ptr;

        ++total_ptrs;
        if (dang_stats_inserthash(hash_table, ptr)) { /* Unique pointer */
            ++uniq_ptrs;
        }
        if (ptr_value >= obj_addr && ptr_value < obj_end) {
            ++valid_ptrs;
        }
        
        byte = (logbuf[i] >> 8) & 0xFF;
        if (!byte)
            continue;
        ++total_ptrs;
        ptr = (ptr_prefix << 8 | byte);
        if (dang_stats_inserthash(hash_table, ptr)) { /* Unique pointer */
            ++uniq_ptrs;
        }
        ptr_value = *(unsigned long*)ptr;
        if (ptr_value >= obj_addr && ptr_value < obj_end) {
            ++valid_ptrs;
        }
        
        byte = logbuf[i] & 0xFF;
        if (!byte)
            continue;
        ++total_ptrs;
        ptr = (ptr_prefix << 8 | byte);
        if (dang_stats_inserthash(hash_table, ptr)) { /* Unique pointer */
            ++uniq_ptrs;
        }
        ptr_value = *(unsigned long*)ptr;
        if (ptr_value >= obj_addr && ptr_value < obj_end) {
            ++valid_ptrs;
        }
    }
   
    /* Free hash_table and print object specific stats */ 
    dang_stats_freehash(hash_table); 
    fprintf(fp, "%lu, %lu, %lu, %lu, %lu\n",
            obj_size, total_ptrs, uniq_ptrs, (total_ptrs - uniq_ptrs), valid_ptrs);
    fflush(fp);
    //dang_print_strace();
    return; 
}

static inline
void dang_get_pointers(unsigned long ptr_slot, unsigned long *ptr1, unsigned long *ptr2, unsigned long *ptr3)
{
    unsigned long ptr_prefix = ptr_slot >> 24;
    if (!ptr_prefix) {
        *ptr1 = 0;
        *ptr2 = 0;
        *ptr3 = 0;
    } else {
        unsigned long byte = (ptr_slot >> 16) & 0xFF;
        *ptr1 = (ptr_prefix << 8 | byte);
        
        byte = (ptr_slot >> 8) & 0xFF;
        if (!byte) {
            *ptr2 = 0;
            *ptr3 = 0;
        } else {
            *ptr2 = (ptr_prefix << 8 | byte);
            byte = ptr_slot & 0xFF;
            if (!byte) {
                *ptr3 = 0;
            } else {
                *ptr3 = (ptr_prefix << 8 | byte);
            }
        }
    }
    return;
}

#define LOG_STRIDE(ptr1, ptr2, ptr_count)                                       \
    unsigned long stride = ptr1 > ptr2 ? (ptr1 - ptr2) : (ptr2 - ptr1);         \
    fprintf(fp, ",%lu", stride);                                                \
    ++ptr_count;
     
/*
 * Collect pointer clearance pattern. This function is called when some slots
 * are cleared from log_buffer.
 */
void 
dang_stats_ptr_pattern(unsigned long *logbuf, unsigned long size, unsigned long start, unsigned long end)
{
    static unsigned int max_ptrs = 0;
    /* Track only for limited number of objects */
    if (counter >= DANG_STATS_OBJS) {
        if (counter == DANG_STATS_OBJS) {
            fprintf(fp, "Objects");
            for (unsigned int i = 0; i < max_ptrs; ++i) {
                fprintf(fp, ",P%u", i);
            }
            ++counter;
        }
        return;
    }
    ++counter;
   
    /* Open stats file, if it is not already open */ 
    if (!fp) {
        char *file_path = getenv(DANG_STATS_FILE);
        fp = fopen(file_path, "w"); 
        if (fp == NULL) {
            fprintf(stdout, "Cannot open stats file! \n");
            abort();
        }
        fprintf(fp, "Objects\n");
    }
    
    fprintf(fp, "%u", counter);
    unsigned long last_ptr = 0;
    unsigned int ptr_count = 0;
    /* Iterate through start to end of the cleared slots */ 
    while (start != end) {
        unsigned long ptr[3];
        dang_get_pointers(logbuf[start], &ptr[0], &ptr[1], &ptr[2]);
        for (unsigned long i = 0; i < 3; i++) {
            if (ptr[i]) {
                if (last_ptr) {
                    LOG_STRIDE(last_ptr, ptr[i], ptr_count);
                } else {
                    LOG_STRIDE(ptr[i], ptr[i], ptr_count);
                }
                last_ptr = ptr[i];
            } else {
                break;
            }
        }
        start = (start + 1) & (size - 1);
    }

    if (max_ptrs < ptr_count) {
        max_ptrs = ptr_count;
    }
    fprintf(fp, "\n");
    return;
}

unsigned long last_metadata;
unsigned long ptr_regcount = 0;
unsigned long ptr_lastcount = 0; 
unsigned long dang_log_full = 0;
unsigned long dang_ptr_reg = 0;
unsigned long dang_ptr_insert = 0; 
unsigned long dang_obj_alloc = 0;
unsigned long dang_obj_free = 0;
unsigned long dang_dup_ptr = 0;

void
dang_stats_last_object(unsigned long metadata)
{
    if (metadata == last_metadata) {
        ptr_lastcount++;
    }
    ++ptr_regcount;
    last_metadata = metadata;
    return;
}

void
dang_print_stats(void)
{
    fprintf(stderr, "Total pointer register = %lu, last_count = %lu, percentage = %lf\n", 
        ptr_regcount, ptr_lastcount, ptr_lastcount / (double)ptr_regcount);
    fprintf(stdout, "Number of times Log full = %lu\n"
                    "Number of pointer registration = %lu\n"
                    "Number of pointers inserted = %lu\n"
                    "Number of objects allocated = %lu\n"
                    "Number of objects free'd = %lu\n"
                    "Number of duplicate pointers = %lu\n",
                    dang_log_full, dang_ptr_reg, dang_ptr_insert,
                    dang_obj_alloc, dang_obj_free, dang_dup_ptr);
}

__attribute__((section(".fini_array"), used))
void (*dang_exit)(void) = dang_print_stats;

#endif
