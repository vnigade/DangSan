CC=clang
INCLUDES=-I. -I../metapagetable
#CFLAGS=-c -Werror -Wall -flto -O3 -fPIC -std=gnu11 -DADDITIVE_GROW -DNDEBUG -DDANG_THREAD_ENABLE
CFLAGS=-c -Werror -Wall -flto -O3 -fPIC -std=gnu11 -DTRACK_ALL_PTRS -DADDITIVE_GROW -DNDEBUG -DDANG_THREAD_ENABLE 
#-fno-omit-frame-pointer -ggdb
#-DHASHTABLE_GROW

EXE=libmetadata.a

SRCS    := $(wildcard *.c) $(wildcard **/*.c)
OBJS    := $(patsubst %.c,obj/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

# Decide which allocator to use for data structures #
# For DangSan we use TC_ALLOC=1 #
ifdef SANITIZER_ALLOC
	INCLUDES += -IDangling/sanitizer_allocator
	CFLAGS += -DSANITIZER_ALLOC
	ALLOC_DIR := Dangling/sanitizer_allocator
else
    ifdef TC_ALLOC
	CFLAGS += -DTC_ALLOC
    else
	INCLUDES += -IDangling/own_allocator
	ALLOC_DIR := Dangling/own_allocator
    endif
endif

# Stats collection is enabled
ifdef DANG_STATS
	CFLAGS += -DDANG_STATS
endif

# Set N-Lookbehind value
ifdef DANG_NLOOKBEHIND
	CFLAGS += -DDANG_NLOOKBEHIND=${DANG_NLOOKBEHIND}
endif

# Set Log size (Number of pointers per size)
# Size should be multiple of 2.
ifdef DANG_LOG_SIZE
	CFLAGS += -DOBJLOG_MAX_PTRS=${DANG_LOG_SIZE}
endif

all: directories $(EXE) $(EXE2)

debug: CFLAGS += -g3 -DDANG_DEBUG
debug: directories $(EXE) $(EXE2)

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(EXE)

$(EXE): $(OBJS)
	llvm-ar crv $@ $(OBJS)
	llvm-ranlib $@

-include $(DEPS)

obj/%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -o $@ $< 

directories:
	mkdir -p obj
	mkdir -p obj/Dangling
	mkdir -p obj/memleak
