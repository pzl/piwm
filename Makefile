MAJOR_VERSION=0
VERSION=$(MAJOR_VERSION).0.1

TARGET=piwm

SRCDIR = src
OBJDIR = build

PLATFORM=$(shell uname -m)
CC = gcc
CFLAGS += -Wall -Wextra
CFLAGS += -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align
CFLAGS += -Wwrite-strings -ftrapv
#CFLAGS += -fsanitize=address
CFLAGS += -march=native
#CFLAGS += -pthread
SFLAGS = -std=gnu99 -pedantic
LDFLAGS += -L/opt/vc/lib
INCLUDES += -I. -isystem /opt/vc/include -isystem /opt/vc/include/interface/vcos/pthreads -isystem /opt/vc/include/interface/vmcs_host/linux
SRCS = $(wildcard $(SRCDIR)/*.c)
LIBS += -lbcm_host -lEGL -lGLESv2 -lpthread
OBJS=$(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

PREFIX ?= /usr
DEST = $(DESTDIR)$(PREFIX)
LIBDIR = $(DEST)/lib
INCDIR = $(DEST)/include


all: CFLAGS += -Os
all: LDFLAGS += -s
all: $(TARGET)

debug: CFLAGS += -O0 -g -DDEBUG
debug: $(TARGET)


$(OBJS): Makefile

dummy := $(shell test -d $(OBJDIR) || mkdir -p $(OBJDIR))


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SFLAGS) $(INCLUDES) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(SFLAGS) $(INCLUDES) -c -o $@ $< $(LDFLAGS) $(LIBS)


clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all debug clean install uninstall
