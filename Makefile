# Makefile for xrick TI-Nspire CX CAS II port
# Requires: ndless-sdk (nspire-gcc, genzehn, make-prg)

# Output
TARGET  = xrick
ELF     = $(TARGET).elf
ZEHN    = $(TARGET).zehn
TNS     = $(TARGET).tns

# Toolchain
CC      = nspire-gcc
LD      = nspire-gcc
GENZEHN = genzehn
MAKEPRG = make-prg

# Directories
SRCDIR  = src
INCDIR  = include
NSPDIR  = nspire
PRODDIR = prod

# Compiler flags
GCCFLAGS = -Wall -W -marm -mno-unaligned-access \
           -ffunction-sections -fdata-sections \
           -fno-unwind-tables -fno-asynchronous-unwind-tables \
           -std=gnu99 -Os -flto

CFLAGS  = $(GCCFLAGS) \
          -I$(NSPDIR) -I$(INCDIR) -I$(SRCDIR) \
          -DNSPIRE -DNDEBUG -DLSB_FIRST=1 \
          -DNOZLIB

LDFLAGS = -Wl,--gc-sections \
          -Wl,--wrap=fopen \
          -Wl,--wrap=open \
          -Wl,--wrap=stat \
          -Wl,--wrap=clock \
          -Wl,--wrap=access \
          -Os -flto -marm -mno-unaligned-access

LIBS    = -lSDL -lm

# Source files - xrick core
CORE_SRC = \
	$(SRCDIR)/control.c \
	$(SRCDIR)/dat_ents.c \
	$(SRCDIR)/dat_maps.c \
	$(SRCDIR)/dat_picsST.c \
	$(SRCDIR)/dat_screens.c \
	$(SRCDIR)/dat_snd.c \
	$(SRCDIR)/dat_spritesST.c \
	$(SRCDIR)/dat_tilesST.c \
	$(SRCDIR)/data.c \
	$(SRCDIR)/draw.c \
	$(SRCDIR)/e_bomb.c \
	$(SRCDIR)/e_bonus.c \
	$(SRCDIR)/e_box.c \
	$(SRCDIR)/e_bullet.c \
	$(SRCDIR)/e_rick.c \
	$(SRCDIR)/e_sbonus.c \
	$(SRCDIR)/e_them.c \
	$(SRCDIR)/ents.c \
	$(SRCDIR)/env.c \
	$(SRCDIR)/fb.c \
	$(SRCDIR)/game.c \
	$(SRCDIR)/img.c \
	$(SRCDIR)/maps.c \
	$(SRCDIR)/rects.c \
	$(SRCDIR)/scr_gameover.c \
	$(SRCDIR)/scr_getname.c \
	$(SRCDIR)/scr_imain.c \
	$(SRCDIR)/scr_imap.c \
	$(SRCDIR)/scr_pause.c \
	$(SRCDIR)/scr_xrick.c \
	$(SRCDIR)/scroller.c \
	$(SRCDIR)/sounds.c \
	$(SRCDIR)/sprites.c \
	$(SRCDIR)/sysjoy.c \
	$(SRCDIR)/system.c \
	$(SRCDIR)/tiles.c \
	$(SRCDIR)/util.c

# Nspire platform files (replace sysvid.c, sysevt.c, syskbd.c, syssnd.c, xrick.c)
NSPIRE_SRC = \
	$(NSPDIR)/sysvid_nspire.c \
	$(NSPDIR)/sysevt_nspire.c \
	$(NSPDIR)/syskbd_nspire.c \
	$(NSPDIR)/syssnd_nspire.c \
	$(NSPDIR)/sysarg_nspire.c \
	$(NSPDIR)/xrick_nspire.c \
	$(NSPDIR)/saveload.c

# nspire_fopen.c must be compiled WITHOUT -flto for --wrap to work
FOPEN_SRC = $(NSPDIR)/nspire_fopen.c

# Objects
CORE_OBJ   = $(CORE_SRC:.c=.o)
NSPIRE_OBJ = $(NSPIRE_SRC:.c=.o)
FOPEN_OBJ  = $(FOPEN_SRC:.c=.o)
ALL_OBJ    = $(CORE_OBJ) $(NSPIRE_OBJ) $(FOPEN_OBJ)

# Rules
.PHONY: all clean prod

all: $(PRODDIR)/$(TNS)

# nspire_fopen.o: compile without -flto so --wrap symbols are visible
$(FOPEN_OBJ): $(FOPEN_SRC)
	$(CC) $(filter-out -flto,$(CFLAGS)) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(ALL_OBJ)
	$(LD) $(LDFLAGS) -Wl,-Map=$(TARGET).map -o $@ $^ $(LIBS)

$(ZEHN): $(ELF)
	$(GENZEHN) --input $< --output $@

$(TNS): $(ZEHN)
	$(MAKEPRG) $< $@

$(PRODDIR)/$(TNS): $(TNS) | $(PRODDIR)
	cp $< $(PRODDIR)/$(TNS)

$(PRODDIR):
	mkdir -p $(PRODDIR)

clean:
	rm -f $(ALL_OBJ) $(ELF) $(ZEHN) $(TNS) $(TARGET).map
	rm -f $(PRODDIR)/$(TNS)
