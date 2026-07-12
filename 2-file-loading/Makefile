# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

BUILD_DIR = build

EE_BIN = file-loading.elf
EE_OBJS = file-loading.o
EE_LIBS = -ldebug -lc

FILE = message.txt

all: $(BUILD_DIR) $(EE_BIN)
	mv $(EE_BIN) $(BUILD_DIR)
	mv $(EE_OBJS) $(BUILD_DIR)
	cp $(FILE) $(BUILD_DIR)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
