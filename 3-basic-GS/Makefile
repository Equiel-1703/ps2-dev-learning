# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

BUILD_DIR = build

EE_BIN = basic-GS.elf
EE_OBJS = basic-GS.o
EE_LIBS = -ldebug -lc -lgskit -ldmakit

# For gsKit
EE_INCS = -I$(PS2DEV)/gsKit/include
EE_LDFLAGS = -L$(PS2DEV)/gsKit/lib

all: $(BUILD_DIR) $(EE_BIN)
	mv $(EE_BIN) $(BUILD_DIR)
	mv $(EE_OBJS) $(BUILD_DIR)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
