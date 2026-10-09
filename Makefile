ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC


TARGET  := scarytest
NAME    := Scary Test
SHORT   := Scary Test
AUTHOR  := Ugjh
ICON    := assets/icon.png
TV_IMG  := assets/splash_tv.png
DRC_IMG := assets/splash_drc.png


WUT      := $(DEVKITPRO)/wut
PORTLIBS := $(DEVKITPRO)/portlibs/wiiu
PPCLIBS  := $(DEVKITPRO)/portlibs/ppc
CC       := $(DEVKITPPC)/bin/powerpc-eabi-gcc
ARCH     := -mcpu=750 -meabi -mhard-float
CFLAGS   := -O2 -Wall -ffunction-sections -fdata-sections $(ARCH) -D__WIIU__ -D__WUT__ \
            -I$(WUT)/include -I$(PORTLIBS)/include -I$(PORTLIBS)/include/SDL2 -I$(PPCLIBS)/include
LDFLAGS  := $(ARCH) -specs=$(WUT)/share/wut.specs -Wl,--gc-sections \
            -L$(WUT)/lib -L$(PORTLIBS)/lib -L$(PPCLIBS)/lib
PCLIBS   := $(shell $(PORTLIBS)/bin/powerpc-eabi-pkg-config --static --libs SDL2_mixer SDL2_image 2>/dev/null)
   LIBS     := -lSDL2_mixer -lSDL2_image -lSDL2 $(PCLIBS) -lmodplug -lwebp -lstdc++ -lvorbisfile -lvorbis -logg -lmpg123 -ljpeg -lpng -lz -lwut -lm

all: $(TARGET).wuhb

$(TARGET).elf: src/main.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@

$(TARGET).rpx: $(TARGET).elf
	$(DEVKITPRO)/tools/bin/elf2rpl $< $@

$(TARGET).wuhb: $(TARGET).rpx
	$(DEVKITPRO)/tools/bin/wuhbtool $< $@ --name="$(NAME)" --short-name="$(SHORT)" --author="$(AUTHOR)" --icon=$(ICON) --tv-image=$(TV_IMG) --drc-image=$(DRC_IMG)

clean:
	rm -f $(TARGET).elf $(TARGET).rpx $(TARGET).wuhb
