CC=clang
INCLUDES=-I.
CFLAGS=-c -Werror -Wall -O3 -fPIC -std=gnu11
LDFLAGS=-shared

EXE=../lib/libdang-malloc.so

SRCS    := $(wildcard *.c)
OBJS    := $(patsubst %.c,obj/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

all: directories $(EXE)

clean:
	rm obj/*.o
	rm obj/*.d
	rm $(EXE)

$(EXE): $(OBJS)
	 $(CC) $(OBJS) $(LDFLAGS) -o $@ 

-include $(DEPS)

obj/%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -MMD -o $@ $<

directories:
	mkdir -p obj
	mkdir -p $(dir $(EXE))		
