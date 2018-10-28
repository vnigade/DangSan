#ifndef SAN_MALLOC_H
#define SAN_MALLOC_H
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void *dang_malloc(size_t);
void dang_free(void *);

#ifdef __cplusplus
}
#endif

#endif

