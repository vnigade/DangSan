#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <metadata.h>
#include <metapagetable_core.h>

/* DangSan: */
__thread long dang_stack_start;
__thread long dang_stack_end;

void unsafe_stack_alloc_meta(void *addr, unsigned long size, bool islarge) {
    /* DangSan */
    dang_stack_start = (long)addr;
    dang_stack_end = ((long)addr + size);

    if (!is_fixed_compression()) {
        unsigned long alignment = STACKALIGN;
        if (islarge)
            alignment = STACKALIGN_LARGE;
        void *metadata = allocate_metadata(size, alignment);
        set_metapagetable_entries(addr, size, metadata, alignment);
    }
}

void unsafe_stack_free_meta(void *unsafe_stack_start, unsigned long unsafe_stack_size, bool islarge) {
    if (!is_fixed_compression()) {
        unsigned long alignment = STACKALIGN;
        if (islarge)
            alignment = STACKALIGN_LARGE;
        deallocate_metadata(unsafe_stack_start, unsafe_stack_size, alignment);
    }
}
