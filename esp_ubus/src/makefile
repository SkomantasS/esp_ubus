BIN:=esp_ubus
# -llink_core\
# -lmiddleware_implementation\
# -lplatform_port\
# -lutils_modules
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
