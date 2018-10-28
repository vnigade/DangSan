CC=clang++
INCLUDES=-I.
CFLAGS=-g -flto -O3 -fno-strict-aliasing -fsanitize=safe-stack
LDFLAGS=-flto -O3 -fno-strict-aliasing -fsanitize=safe-stack -Wl,-plugin-opt=-largestack=true -Wl,-plugin-opt=-load=../llvm-plugins/libplugins.so -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-custominline -umetaset_1 -umetaget_1 -uinitialize_global_metadata -L../staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L../gperftools-metalloc/.libs -ltcmalloc @../metapagetable/linker-options -Wl,-plugin-opt=-METALLOC_ONLYPOINTERWRITES=false
LDFLAGS_NONE=-flto -O3 -fno-strict-aliasing -fsanitize=safe-stack -Wl,-plugin-opt=-load=../llvm-plugins/libplugins.so -Wl,-plugin-opt=-byvalhandler

EXE=demo
EXE_NONE=demo-none

SRCS    := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,obj/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

all: directories $(EXE) $(EXE_DEEP) $(EXE_FIXED) $(EXE_NONE)

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(EXE)
	rm -f $(EXE_DEEP)
	rm -f $(EXE_FIXED)
	rm -f $(EXE_NONE)

$(EXE): $(OBJS) ../gperftools-metalloc/.libs/libtcmalloc.so ../metapagetable/linker-options ../staticlib/libmetadata.a ../llvm-plugins/libplugins.so
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(EXE_NONE): $(OBJS) ../llvm-plugins/libplugins.so
	$(CC) $(LDFLAGS_NONE) -o $@ $(OBJS)

-include $(DEPS)

obj/%.o: %.cpp
	$(CC) $(INCLUDES) -c $(CFLAGS) -MMD -o $@ $<

directories:
	mkdir -p obj

