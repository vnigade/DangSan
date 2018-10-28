/*
 * pointer_stats.h
 *      Author - Vinod
 */
#ifndef POINTER_STATS_H
#define POINTER_STATS_H

#include <math.h>

#ifndef DANG_NLOOKBEHIND
#define DANG_NLOOKBEHIND        1
#endif

struct dang_stats {
    unsigned long ptr_invalid;      /* Object in which pointer resides is invalid */
    unsigned long object_null;      /* Object passed is NULL */
    unsigned long object_freed;     /* Object is freed already */
    unsigned long object_invalid;   /* Object is invalid */
    unsigned long object_global;    /* Object is global */
    unsigned long heap_alloc;       /* Number of Heap allocaion */
    unsigned long global_alloc;     /* Number of global objects */
    unsigned long heap_free;        /* Number of heap object free */
    unsigned long avg_ptrlist;      /* Average pointer list per object */
    unsigned long avg_dupptr;       /* Average duplicate ptr list per object */
    unsigned long avg_uniqptr;      /* Average unique ptr list per object */
    unsigned long avg_lastptr_dup;  /* Average last duplicate pointer */
} _stats;

void
dang_print_stats()
{
        fprintf(stderr, "Number of invalid pointers %lu\n"
            "Number of null objects %lu\n"
            "Number of invalid objetcs %lu\n"
            "Number of heap allocations %lu\n"
            "Number of global allocations %lu\n"
            "Number of heap objects freed %lu\n"
            "Number of pointers per memory object %lu\n"
            "Number of global invalid objects %lu\n"
            "Number of objects freed already %lu\n"
            "Number of duplicate pointers per object %lu\n"
            "Number of unique pointers per object %lu\n"
            "Number of duplicate pointers per object within %d-lookbehind %lu\n",
            _stats.ptr_invalid, _stats.object_null, _stats.object_invalid,
            _stats.heap_alloc, _stats.global_alloc, _stats.heap_free,
            (unsigned long)ceil(_stats.avg_ptrlist / (double)_stats.heap_free), _stats.object_global,
            _stats.object_freed, (unsigned long)ceil(_stats.avg_dupptr / (double)_stats.heap_free),
            (unsigned long)ceil(_stats.avg_uniqptr / (double)_stats.heap_free), DANG_NLOOKBEHIND,
            (unsigned long)ceil(_stats.avg_lastptr_dup / (double)_stats.heap_free));
        return;
}

void
dang_collect_avgstats(struct dang_stats *_stats,
                unsigned long ptrlist_cnt,
                unsigned long ptr_duplicate,
                unsigned long ptr_unique,
                unsigned long lastptr_dup)
{
//    double count;
    //if (_stats->heap_free == 0) {
        _stats->avg_ptrlist += ptrlist_cnt;
        _stats->avg_dupptr += ptr_duplicate;
        _stats->avg_uniqptr += ptr_unique;
        _stats->avg_lastptr_dup += lastptr_dup;
//    } else {
//        /* Calculate average number of pointers per object */
//        count = ptrlist_cnt / (double)(_stats->heap_free + 1);
//        count += (_stats->heap_free / (double)(_stats->heap_free + 1)) * (double)_stats->avg_ptrlist;
//        _stats->avg_ptrlist = ceil(count);
//
//        /* Calculate average number of duplicate pointers per object */
//        count = ptr_duplicate / (double)(_stats->heap_free + 1);
//        count += (_stats->heap_free / (double)(_stats->heap_free + 1)) * (double)_stats->avg_dupptr;
//        _stats->avg_dupptr = ceil(count);
//    
//        /* Calculate average number of unique pointers per object */
//        count =  ptr_unique / (double)(_stats->heap_free + 1);
//        count += (_stats->heap_free / (double)(_stats->heap_free + 1)) * (double)_stats->avg_uniqptr;
//        _stats->avg_uniqptr = ceil(count);
//    }
    ++_stats->heap_free;
    return;
}

__attribute__((section(".fini_array"), used))
void (*dang_exit)(void) = dang_print_stats;

#endif /* !POINTER_STATS_H */
