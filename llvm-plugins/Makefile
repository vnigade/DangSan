CC=g++
INCLUDES=-I. -I../staticlib -I../metapagetable
CFLAGS=-c -Werror -Wall -std=c++0x -g `llvm-config --cxxflags` -O0
LDFLAGS=-g -shared
GOLDINSTDIR=$(METALLOC_HOME)/autosetup.dir/install
LINKWITHGOLDFLAGS=-Wl,--no-undefined -L $(GOLDINSTDIR)/lib/ -l:LLVMgold.so

EXE=libplugins.so
EXE2=libplugins-opt.so

SRCS    := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,obj/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

all: directories $(EXE) $(EXE2)

debug: CFLAGS += -DDANG_DEBUG
debug: all

clean:
	rm obj/*.o
	rm obj/*.d
	rm $(EXE)
	rm $(EXE2)

$(EXE): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LINKWITHGOLDFLAGS) -o $@

$(EXE2): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

-include $(DEPS)

obj/%.o: %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -o $@ $<

directories:
	mkdir -p obj
