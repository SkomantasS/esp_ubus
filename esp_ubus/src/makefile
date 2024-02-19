BIN:=esp_ubus
LDFLAGS:=-lubus\
-lubox\
-lblobmsg_json\
-ljson-c\
-lserialport

CPPFLAGS:=-I$(CURDIR)
SRC:=$(wildcard *.c)
OBJS:=$(SRC:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN) $(OBJS)
