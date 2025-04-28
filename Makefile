# Makefile

CC=gcc
CFLAGS=-Wall -Wextra -std=c11
LDFLAGS=-lrt -lpthread
TARGET=proj2

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
