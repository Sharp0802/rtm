#########################
#  BASIC CONFIGURATION  #
#########################

LIBS := pcap

CFLAGS := -std=gnu99 -Oz

CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -falign-functions -ftree-vectorize
CFLAGS += -march=native

CFLAGS += -W -Wall -Wextra
CFLAGS += -I./inc/

#########################
#  LD/CC CONFIGURATION  #
#########################

CCD := $(shell find /usr/lib/gcc/x86_64-*/*.*.* -maxdepth 0 | sort -r | head -1)/

LDFLAGS += -L$(CCD)
LDFLAGS += -lc -lgcc -lgcc_s $(addprefix -l,$(LIBS))
LDFLAGS += --gc-sections

CRT := $(addprefix /usr/lib/,crt1.o crti.o crtn.o)
CRT += $(addprefix $(CCD),crtbegin.o crtend.o)

C := $(wildcard src/*.c)
O := $(subst src,obj,$(C:.c=.o))
B := bin/rtm

#########################
# TARGETS CONFIGURATION #
#########################

all: $(B)
	strip -s -R .comment -R .eh_frame -R .gnu.version -R .note.gnu.property -R .gnu.hash $<
	@stat $<

$(B): $(O)
	@mkdir -p bin
	$(LD) $(LDFLAGS) $(CRT) $^ -o $@

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(O) $(B)

.PHONY: all clean

