### constant symbols

ARCH ?=
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

CFLAGS := -O2 -Wall -Wextra -std=c99 -Iinc -I.

OBJDIR = obj
INCLUDES = $(wildcard inc/*.h)

### linking options

LIBS := -lloragw -lstack_ipv6 -lrt -lm

### general build targets

all: libstack_ipv6.a

clean:
	rm -f libstack_ipv6.a
	rm -f $(OBJDIR)/*.o

### library module target

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: src/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

### static library

libstack_ipv6.a:  $(OBJDIR)/IPv6.o $(OBJDIR)/LoRaMacCrypto.o $(OBJDIR)/aes.o $(OBJDIR)/cmac.o $(OBJDIR)/utilities.o
	$(AR) rcs $@ $^


### EOF
