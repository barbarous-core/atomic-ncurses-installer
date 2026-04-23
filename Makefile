CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g \
            -I src \
            $(shell pkg-config --cflags ncursesw || pkg-config --cflags ncurses)
LDFLAGS := $(shell pkg-config --libs ncursesw || pkg-config --libs ncurses)

TARGET  := barbarous-install-tui
SRCDIR  := src
SRCS    := $(shell find $(SRCDIR) -name '*.c')
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	find $(SRCDIR) -name '*.o' -delete
	rm -f $(TARGET)
