### constant symbols

ARCH ?=
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

CFLAGS := -O2 -Wall -Wextra -std=c99 -Iinc -I.

OBJDIR = obj
OBJDIR_LoWPAN_HC = obj
OBJDIR_tun_tap = obj
OBJDIR_IPv6_checksum = obj
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

$(OBJDIR_LoWPAN_HC)/%.o: src/LoWPAN_HC/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR_tun_tap)/%.o: src/tun_tap/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR_IPv6_checksum)/%.o: src/IPv6_checksum/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@	
### static library

libstack_ipv6.a:  $(OBJDIR_LoWPAN_HC)/LoWPAN_HC.o $(OBJDIR_tun_tap)/tun_tap.o $(OBJDIR_IPv6_checksum)/IPv6_checksum.o 
	$(AR) rcs $@ $^


### EOF
