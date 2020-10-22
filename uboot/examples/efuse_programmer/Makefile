#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#
TOPDIR  := $(shell /bin/pwd)
include $(TOPDIR)/../../config.mk

LOAD_ADDR = 0x01500000
STATIC_EFUSE_LIB = ../../static_lib/libefuse.programmer.a

STATIC_LIB =	../../lib/div64.o				\
		../../lib/time.o				\
		../../$(CPUDIR)/$(SOC)/mcp.o		\
		../../$(CPUDIR)/$(SOC)/timer.o	\
		../../$(CPUDIR)/$(SOC)/big_int.o	\
		../../$(CPUDIR)/$(SOC)/big_util.o	\
		../../$(CPUDIR)/cache_v7.o

# ********************************************************************
# Detect compiling environment.
# ********************************************************************
UNAME = $(shell uname)
LINUX_ENV = "LINUX"
CYGWIN_ENV = "CYGWIN"
ENV = $(CYGWIN_ENV)	# default

ifneq "$(filter Linux%,$(UNAME))" ""
ENV = $(LINUX_ENV)
endif

ifneq "$(filter CYGWIN%,$(UNAME))" ""
ENV = $(CYGWIN_ENV)
endif

# ********************************************************************
# Directories specifics.
# ********************************************************************


PERL    = perl
RM      = rm -rf
CD      = cd
CAT     = cat
CP      = cp

TOP_DIR         = $(shell cd .; pwd)
INCLUDE_DIR     = $(TOP_DIR)/include

OBFUSECATOR = $(TOP_DIR)/obfuse
RTSSL_EXE = $(TOP_DIR)/../../examples/flash_writer_u/tools/RtSSL/RtSSL
STR2BIN_PL = $(TOP_DIR)/../../examples/flash_writer_u/tools/script/str2bin.pl
REVERSE_PL = $(TOP_DIR)/../../examples/flash_writer_u/tools/script/reverse.pl
RSA_KEY_FILE = $(TOP_DIR)/../../examples/flash_writer_u/image/rsa_key_2048.pem
RSA_PUB_BIN = $(TOP_DIR)/secure_key/rsa_pub.bin
RSA_PUB_REV_BIN = $(TOP_DIR)/secure_key/rsa_pub_rev.bin
AES_KEY_TARGET = $(TOP_DIR)/../../examples/flash_writer_u/image/aes_128bit_key.bin
AES_SEED_TARGET = $(TOP_DIR)/../../examples/flash_writer_u/image/aes_128bit_seed.bin
AES_KEY_ENC = $(TOP_DIR)/secure_key/aes_128bit_key.bin
AES_SEED_ENC = $(TOP_DIR)/secure_key/aes_128bit_seed.bin
POCONO_KEY_TARGET = $(TOP_DIR)/../../examples/flash_writer_u/image/pocono_key.bin
POCONO_KEY = $(TOP_DIR)/secure_key/pocono_key.bin
USER_PART_KEY_TARGET = $(TOP_DIR)/../../examples/flash_writer_u/image/user_part_key.bin
USER_PART_KEY = $(TOP_DIR)/secure_key/user_part_key.bin

BIN_ARGS      	 = "rsa_pub_key="$(RSA_PUB_REV_BIN)
BIN_ARGS         += "aes_key="$(AES_KEY_ENC)
BIN_ARGS         += "aes_seed="$(AES_SEED_ENC)
BIN_ARGS	 += "pocono_key="$(POCONO_KEY)
BIN_ARGS	 += "user_part_key="$(USER_PART_KEY)

#-------------------------------------------------
TARGET_CASE = 

SOBJS	:= temp_target_array.o
SOBJS	+= start.o
COBJS	:= efusemain.o util.o ctype.o
LIB	= $(obj)

LIBCOBJS = 

LIBOBJS	= $(addprefix $(obj),$(LIBAOBJS) $(LIBCOBJS))

SRCS	:= $(COBJS:.o=.c) $(LIBCOBJS:.o=.c) $(LIBAOBJS:.o=.S) $(SOBJS:.o=.S)
OBJS	:= $(addprefix $(obj),$(COBJS))
SSOBJS	:= $(addprefix $(obj),$(SOBJS))


gcclibdir := $(shell dirname `$(CC) -print-libgcc-file-name`)

CPPFLAGS += -I.. -I$(INCLUDE_DIR) -I$(TOP_DIR)/include
INCDIRS =  -I$(TOP_DIR)/../../include -I.. -I$(INCLUDE_DIR) 

# For PowerPC there's no need to compile standalone applications as a
# relocatable executable.  The relocation data is not needed, and
# also causes the entry point of the standalone application to be
# inconsistent.
ifeq ($(ARCH),powerpc)
AFLAGS := $(filter-out $(RELFLAGS),$(AFLAGS))
CFLAGS := $(filter-out $(RELFLAGS),$(CFLAGS))
CPPFLAGS := $(filter-out $(RELFLAGS),$(CPPFLAGS))
endif

# We don't want gcc reordering functions if possible.  This ensures that an
# application's entry point will be the first function in the application's
# source file.
CFLAGS = -g -Os -fno-common -fzero-initialized-in-bss -mfloat-abi=hard -mthumb-interwork -D__KERNEL__ -fno-builtin -ffreestanding -nostdinc -isystem $(gcclibdir)/../../../include -pipe -DCONFIG_ARM -D__ARM__ -marm -mabi=aapcs-linux -march=armv7-a -D__EL__
CFLAGS += $(INCDIRS)

#.PHONY: clean

all: clean gen_key b2a $(OBJS) $(SSOBJS) $(LIB) build_key build_finalize build_mp

efusemain_def_key: efusemain.c
	-rm -rf efusemain.o
	$(CC) $(CFLAGS) -c $< -o efusemain.o 

efusemain_def_finalize: efusemain.c
	-rm -rf efusemain.o
	$(CC) $(CFLAGS) -c $< -o efusemain.o

efusemain_def_mp: efusemain.c
	-rm -rf efusemain.o
	$(CC) $(CFLAGS) -c $< -o efusemain.o

key: key_burn_def efusemain_def_key
	$(LD) $(LDFLAGS) -g -Ttext $(LOAD_ADDR) \
	-o $(TARGET_CASE) -e start $(SOBJS) $(COBJS) $(LIB) $(STATIC_EFUSE_LIB) \
	-L$(gcclibdir) -lgcc -Map $(TARGET_CASE).map

build_key: key
	$(OBJCOPY) -O binary $(TARGET_CASE) $(TARGET_CASE).bin 2>/dev/null

finalize: finalize_burn_def efusemain_def_finalize
	$(LD) $(LDFLAGS) -g -Ttext $(LOAD_ADDR) \
	-o $(TARGET_CASE) -e start $(SOBJS) $(COBJS) $(LIB) $(STATIC_EFUSE_LIB) \
	-L$(gcclibdir) -lgcc -Map $(TARGET_CASE).map

build_finalize: finalize
	$(OBJCOPY) -O binary $(TARGET_CASE) $(TARGET_CASE).bin 2>/dev/null

mp: mp_burn_def efusemain_def_mp
	$(LD) $(LDFLAGS) -g -Ttext $(LOAD_ADDR) \
	-o $(TARGET_CASE) -e start $(SOBJS) $(COBJS) $(LIB) $(STATIC_EFUSE_LIB) \
	-L$(gcclibdir) -lgcc -Map $(TARGET_CASE).map

build_mp: mp
	$(OBJCOPY) -O binary $(TARGET_CASE) $(TARGET_CASE).bin 2>/dev/null

key_burn_def:
	$(eval CFLAGS := $(filter-out -DFINALIZE_BURN,$(CFLAGS)))
	$(eval CFLAGS += -DKEY_BURN)
	$(eval TARGET_CASE := efuse_programmer_key)
	$(eval BIN := $(addsuffix .bin,$(TARGET_CASE)))

finalize_burn_def:
	$(eval CFLAGS := $(filter-out -DKEY_BURN,$(CFLAGS)))
	$(eval CFLAGS += -DFINALIZE_BURN)
	$(eval TARGET_CASE := efuse_programmer_finalize)
	$(eval BIN := $(addsuffix .bin,$(TARGET_CASE)))

mp_burn_def:
	$(eval CFLAGS += -DKEY_BURN)
	$(eval TARGET_CASE := efuse_programmer_mp)
	$(eval BIN := $(addsuffix .bin,$(TARGET_CASE)))

gen_key:
	$(STR2BIN_PL) `$(RTSSL_EXE) rsa -print -modulous -i $(RSA_KEY_FILE)` > $(RSA_PUB_BIN)
	$(REVERSE_PL) $(RSA_PUB_BIN) > $(RSA_PUB_REV_BIN)
	$(OBFUSECATOR) 2 $(AES_KEY_TARGET) $(AES_SEED_TARGET) $(AES_KEY_ENC) $(AES_SEED_ENC)
	$(OBFUSECATOR) 3 $(POCONO_KEY_TARGET) $(USER_PART_KEY_TARGET) $(POCONO_KEY) $(USER_PART_KEY)

b2a:
	./bin2ascii $(BIN_ARGS) > temp_target_array.h

clean:
	-rm -f efuse_programmer* *.o temp_target_array.h *.bin *.map *.dis *.srec

$(LIB):	$(obj).depend $(LIBOBJS)
	$(call cmd_link_o_target, $(LIBOBJS))
