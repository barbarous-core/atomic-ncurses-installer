CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g \
            -I src/common -I src/installer -I src/setup \
            $(shell pkg-config --cflags ncursesw || pkg-config --cflags ncurses)
LDFLAGS := $(shell pkg-config --libs ncursesw || pkg-config --libs ncurses)

INSTALLER_TARGET := barbarous-install-tui
SETUP_TARGET     := barbarous-setup-tui

SRCDIR  := src
COMMON_DIR    := $(SRCDIR)/common
INSTALLER_DIR := $(SRCDIR)/installer
SETUP_DIR     := $(SRCDIR)/setup

# Common files used by both
COMMON_SRCS := $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

# Installer specific
INSTALLER_SRCS := $(wildcard $(INSTALLER_DIR)/*.c)
INSTALLER_OBJS := $(INSTALLER_SRCS:.c=.o) $(COMMON_OBJS)

# Setup specific
SETUP_SRCS := $(wildcard $(SETUP_DIR)/*.c)
SETUP_OBJS     := $(SETUP_SRCS:.c=.o) $(COMMON_OBJS)

.PHONY: all clean run-install run-setup

all: $(INSTALLER_TARGET) $(SETUP_TARGET)

$(INSTALLER_TARGET): $(INSTALLER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(INSTALLER_OBJS) $(LDFLAGS)

$(SETUP_TARGET): $(SETUP_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SETUP_OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run-install: $(INSTALLER_TARGET)
	./$(INSTALLER_TARGET)

run-setup: $(SETUP_TARGET)
	./$(SETUP_TARGET)

clean:
	find $(SRCDIR) -name '*.o' -delete
	rm -f $(INSTALLER_TARGET) $(SETUP_TARGET)
