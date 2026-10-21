# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = mov-textured-square.elf
EE_OBJS = mov-textured-square.o
EE_LIBS = -ldebug -lc -lgskit_toolkit -lgskit -ldmakit -lpad

# For gsKit
EE_INCS = -I$(GSKIT)/include
EE_LDFLAGS = -L$(GSKIT)/lib

.PHONY: all clean run

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	run-elf.sh $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
