CC ?= clang
CWARNINGS := -Wall
CFLAGS += --std=c99 -g -O2 -MMD $(CWARNINGS)

all: dcc

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

DEPS = $(patsubst %.c,%.d,$(SRCS))

dcc: $(OBJS)
	$(CC) -o $@ $(filter %.o,$^)

%.d: %.o;

clean: .PHONY
	rm -f $(OBJS) ./dcc

-include $(DEPS)

.PHONY:
