ROOTDIR=$(METALLOC_HOME)/autosetup.dir/src/llvm-svn/projects/compiler-rt/lib
SRCDIR=$(ROOTDIR)/sanitizer_common
CC=clang++
INCLUDES=-I$(ROOTDIR) -I$(ROOTDIR)/../include -I$(ROOTDIR)/../include/sanitizer
CFLAGS=-c -Werror -Wall -O3 -fno-exceptions -fPIC -std=c++1z
LDFLAGS=-shared -lpthread

EXE=../lib/libdang-malloc.so

NolibcSources := $(wildcard $(SRCDIR)/*_nolibc.cc)
SRCS := $(wildcard *.cc) $(filter-out $(NolibcSources), $(wildcard $(SRCDIR)/sanitizer_*.cc))
OBJS := $(patsubst %.cc,obj/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: directories $(EXE)

clean:
	rm obj/*.o
	rm obj/*.d
	rm $(EXE)

$(EXE): $(OBJS)
	$(CC) $(addprefix obj/, $(notdir $(OBJS))) $(LDFLAGS) -o $@

obj/%.o: %.cc
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -o obj/$(notdir $@) $<

directories:
	@mkdir -p obj
	@mkdir -p $(dir $(EXE))

-include $(DEPS)
