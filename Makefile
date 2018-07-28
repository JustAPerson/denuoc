CC ?= gcc

all: denuoc

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

denuoc: $(OBJS)
	$(CC) -o $@ $<
