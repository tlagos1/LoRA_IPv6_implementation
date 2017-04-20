### Application-specific constants

APP_NAME := project_ipv6

### Environment constants 

LGW_PATH ?= ../libloragw
LGW_IPV6_PATH ?= ../libstack_ipv6
ARCH ?=
CROSS_COMPILE ?=

### External constant definitions

include $(LGW_PATH)/library.cfg

### Constant symbols

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar

CFLAGS=-O2 -Wall -Wextra -std=c99 -Iinc -I.

OBJDIR = obj

### Constants for LoRa concentrator HAL library
# List the library sub-modules that are used by the application

LGW_INC = $(LGW_PATH)/inc/config.h
LGW_INC += $(LGW_PATH)/inc/loragw_hal.h
LGW_INC += $(LGW_IPV6_PATH)/inc/tun_tap.h
LGW_INC += $(LGW_IPV6_PATH)/inc/SCHC.h


### Linking options

LIBS := -lloragw -lstack_ipv6 -lrt -lm -lpthread

### General build targets

all: $(APP_NAME)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(APP_NAME)

### HAL library (do no force multiple library rebuild even with 'make -B')

$(LGW_PATH)/inc/config.h:
	@if test ! -f $@; then \
	$(MAKE) all -C $(LGW_PATH); \
	fi

$(LGW_PATH)/libloragw.a: $(LGW_INC)
	@if test ! -f $@; then \
	$(MAKE) all -C $(LGW_PATH); \
	fi

### Sub-modules compilation

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/parson.o: src/parson.c inc/parson.h | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

### Main program compilation and assembly

$(OBJDIR)/$(APP_NAME).o: src/$(APP_NAME).c $(LGW_INC) inc/parson.h | $(OBJDIR)
	$(CC) -c $(CFLAGS) -I$(LGW_PATH)/inc -I$(LGW_IPV6_PATH)/inc $< -o $@

$(APP_NAME): $(OBJDIR)/$(APP_NAME).o $(LGW_PATH)/libloragw.a $(LGW_IPV6_PATH)/libstack_ipv6.a $(OBJDIR)/parson.o
	$(CC) -L$(LGW_PATH) -L$(LGW_IPV6_PATH) $< $(OBJDIR)/parson.o -o $@ $(LIBS)

### EOF
