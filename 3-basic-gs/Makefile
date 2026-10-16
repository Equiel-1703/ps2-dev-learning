# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = basic-gs.elf
EE_OBJS = basic-gs.o
EE_LIBS = -ldebug -lc -lgskit -ldmakit

# For gsKit
EE_INCS = -I$(GSKIT)/include
EE_LDFLAGS = -L$(GSKIT)/lib

.PHONY: all clean

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
