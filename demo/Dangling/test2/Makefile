CC=clang++
INCLUDES=-I.
CFLAGS= -flto -O0 -fsanitize=safe-stack
LDFLAGS=-flto -O0 -fsanitize=safe-stack -Wl,-plugin-opt=-largestack=false -Wl,-plugin-opt=-load=../../../llvm-plugins/libplugins.so -Wl,-plugin-opt=-debug-pass=Arguments -Wl,-plugin-opt=-stats -Wl,-plugin-opt=-byvalhandler -Wl,-plugin-opt=-stacktracker -Wl,-plugin-opt=-globaltracker -Wl,-plugin-opt=-pointertracker -umetaset_8 -umetaget_8 -uinitialize_global_metadata -L../../../staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive -L../../../gperftools-metalloc/.libs -ltcmalloc @../../../metapagetable/linker-options -Wl,-plugin-opt=-METALLOC_ONLYPOINTERWRITES=false -L../../../staticlib/Dangling/lib -ldang-malloc 
#-Wl,-rpath ../../staticlib/Dangling/lib  # It requires :  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/Metalloc/staticlib/Dangling/lib/
LDFLAGS_NONE=-flto -g3 -fsanitize=safe-stack -Wl,-plugin-opt=-load=../../../llvm-plugins/libplugins.so  -Wl,-plugin-opt=-byvalhandler -L../../../staticlib -Wl,-whole-archive -l:libmetadata.a -Wl,-no-whole-archive ../../../metapagetable/obj/metapagetable.o -L../../../autosetup.dir/src/gperftools/.libs -ltcmalloc

EXE=demo
EXE_NONE=demo-none

SRCS    := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,obj/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

all: directories $(EXE) $(EXE_DEEP) $(EXE_FIXED) $(EXE_NONE)

debug: CFLAGS += -g
debug: all

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(EXE)
	rm -f $(EXE_DEEP)
	rm -f $(EXE_FIXED)
	rm -f $(EXE_NONE)

$(EXE): $(OBJS) ../../../gperftools-metalloc/.libs/libtcmalloc.so ../../../metapagetable/linker-options ../../../staticlib/libmetadata.a ../../../llvm-plugins/libplugins.so
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(EXE_NONE): $(OBJS) ../../../llvm-plugins/libplugins.so
	$(CC) $(LDFLAGS_NONE) -o $@ $(OBJS)

-include $(DEPS)

obj/%.o: %.cpp
	$(CC) $(INCLUDES) -c $(CFLAGS) -MMD -o $@ $<

directories:
	mkdir -p obj
