CC ?= clang
CFLAGS += --std=c11 -g

all: dcc

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

dcc: $(OBJS)
	$(CC) -o $@ $^

clean:
	rm $(OBJS)
