#ifndef DANG_SAN_H
#define DANG_SAN_H
#include <assert.h>

#ifndef OBJLOG_MAX_PTRS
#define OBJLOG_MAX_PTRS         128           /* Number of pointers to track */
#endif

#define OBJLOG_HT_MAX           OBJLOG_MAX_PTRS

//#define OBJLOG_MAX_PTRS       2             /* Only for debugging */
#define OBJLOG_SIZE             ((OBJLOG_MAX_PTRS+OBJLOG_START_IDX) << 3)
#define OBJLOG_START_IDX        7
//#define OBJLOG_HT_MAX         2             /* Only for debugging */
#define OBJLOG_HT_SIZE          (OBJLOG_HT_MAX << 3)
#define OBJLOG_THRESHOLD_PER    0.95          /* Pointer percentage allowed before reclaimation */

typedef struct dang_hashnode   dang_hashnode_t;
struct dang_hashnode {
    unsigned long ptr;
    dang_hashnode_t *next;
};

typedef struct dang_objlog dang_objlog_t;       /* Log buffer per-thread and per-object */
struct dang_objlog {
    unsigned long thread_id;                    /* Per-thread ID */
    dang_objlog_t *next_log;                    /* Next thread log for the object */
    unsigned long index;                        /* Next empty entry in the static log */
    unsigned long end;                          /* Next pointer entry to remove */
    unsigned long count;                        /* Number of slots in the log */
    unsigned long size;                         /* Required for variable sized log */
    dang_objlog_t *next_growlog;               /* Use Log after overflow */
};

#endif /* !DANG_SAN_H */
