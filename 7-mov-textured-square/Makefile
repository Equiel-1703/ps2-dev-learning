# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = mov-textured-square.elf
EE_OBJS = mov-textured-square.o
EE_LIBS = -ldebug -lpng -lz -lc -lpad -lgskit_toolkit -lgskit -ldmakit

# gsKit headers and PS2SDK ports headers
EE_INCS = -I$(GSKIT)/include -I$(PS2SDK)/ports/include

# gsKit libraries and PS2SDK ports libraries (containing the PNG libs)
EE_LDFLAGS = -L$(GSKIT)/lib -L$(PS2SDK)/ports/lib

.PHONY: all clean run

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	run-elf.sh $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
