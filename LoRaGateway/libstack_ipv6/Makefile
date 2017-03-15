### constant symbols

ARCH ?=
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

CFLAGS := -O2 -Wall -Wextra -std=c99 -Iinc -I.

OBJDIR = obj
OBJDIR_LoWPAN_IPHC = obj
OBJDIR_tun_tap = obj
OBJDIR_IPv6 = obj
OBJDIR_SCHC = obj
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

$(OBJDIR_LoWPAN_IPHC)/%.o: src/LoWPAN_IPHC/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR_tun_tap)/%.o: src/tun_tap/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR_IPv6)/%.o: src/IPv6/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@	

$(OBJDIR_SCHC)/%.o: src/SCHC/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@	


### static library

libstack_ipv6.a:  $(OBJDIR_LoWPAN_IPHC)/LoWPAN_IPHC.o $(OBJDIR_tun_tap)/tun_tap.o $(OBJDIR_IPv6)/IPv6.o $(OBJDIR_SCHC)/SCHC.o 
	$(AR) rcs $@ $^


### EOF
