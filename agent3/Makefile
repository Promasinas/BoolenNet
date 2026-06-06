# Agent 3 Makefile - BoolNet Test Automation
# Target: Windows MinGW/GCC

CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11
LDFLAGS  =
TARGET   = agent3.exe

# All source files
SRCS = agent3.c utils.c config.c module_tracker.c heartbeat.c \
       detector.c interact.c testdir.c makefile_gen.c \
       compiler.c runner.c parallel.c reporter.c gitmgr.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
