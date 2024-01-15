#########################
#  BASIC CONFIGURATION  #
#########################

LIBS := pcap

CFLAGS := -std=gnu89 -Oz
CFLAGS += -march=native
CFLAGS += -W -Wall -Wextra
CFLAGS += -I./inc/
CFLAGS += -fpic

#########################
#  LD/CC CONFIGURATION  #
#########################

LDFLAGS := -shared -fpic $(addprefix -l,$(LIBS))
LDFLAGS += -march=native

C := $(shell find src/ -type f -name '*.c')
O := $(subst src,obj,$(C:.c=.o))
B := bin/librtm.so

#########################
# TARGETS CONFIGURATION #
#########################

all: cfg bin man

cfg:
	@chmod 0777 cfg/version.sh
	@cfg/version.sh

bin: $(B)
	@echo ""
	@stat $<

man:
	@echo ""
	dotnet build src/man/RTM.sln

$(B): $(O)
	@mkdir -p bin
	$(CC) $(LDFLAGS) $^ -o $@
	$(CC) -march=native -lpcap $^ -o bin/rtm

obj/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(O) $(B)

.PHONY: all cfg bin clean

