//===-- sanitizer_allocator_testlib.cc ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Malloc replacement library based on CombinedAllocator.
// The primary purpose of this file is an end-to-end integration test
// for CombinedAllocator.
//===----------------------------------------------------------------------===//
/* Usage:
clang++ -fno-exceptions  -g -fPIC -I. -I../include -Isanitizer \
 sanitizer_common/tests/sanitizer_allocator_testlib.cc \
 sanitizer_common/sanitizer_*.cc -shared -lpthread -o testmalloc.so
LD_PRELOAD=`pwd`/testmalloc.so /your/app
*/
#include "sanitizer_common/sanitizer_allocator.h"
#include "sanitizer_common/sanitizer_common.h"
#include "san_malloc.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#ifndef SANITIZER_MALLOC_HOOK
# define SANITIZER_MALLOC_HOOK(p, s)
#endif

#ifndef SANITIZER_FREE_HOOK
# define SANITIZER_FREE_HOOK(p)
#endif

namespace {
static const uptr kAllocatorSpace = 0x600000000000ULL;
static const uptr kAllocatorSize  =  0x10000000000ULL;  // 1T.

typedef SizeClassAllocator64<kAllocatorSpace, kAllocatorSize, 0,
  CompactSizeClassMap> PrimaryAllocator;
typedef SizeClassAllocatorLocalCache<PrimaryAllocator> AllocatorCache;
typedef LargeMmapAllocator<> SecondaryAllocator;
typedef CombinedAllocator<PrimaryAllocator, AllocatorCache,
          SecondaryAllocator> Allocator;

static Allocator allocator;
static bool global_inited;
static THREADLOCAL AllocatorCache cache;
static THREADLOCAL bool thread_inited;
static pthread_key_t pkey;

static void thread_dtor(void *v) {
  if ((uptr)v != 3) {
    pthread_setspecific(pkey, (void*)((uptr)v + 1));
    return;
  }
  allocator.SwallowCache(&cache);
}

static void NOINLINE thread_init() {
  if (!global_inited) {
    global_inited = true;
    allocator.Init(1);
    pthread_key_create(&pkey, thread_dtor);
  }
  thread_inited = true;
  pthread_setspecific(pkey, (void*)1);
  cache.Init(0);
}
}  // namespace

void *dang_malloc(size_t size) {
  if (UNLIKELY(!thread_inited))
    thread_init();
  void *p = allocator.Allocate(&cache, size, 8);
  SANITIZER_MALLOC_HOOK(p, size);
  return p;
}

void dang_free(void *p) {
  if (UNLIKELY(!thread_inited))
    thread_init();
  SANITIZER_FREE_HOOK(p);
  allocator.Deallocate(&cache, p);
}

namespace std {
  struct nothrow_t;
}
