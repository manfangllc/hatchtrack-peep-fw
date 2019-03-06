CC = gcc

PROTOBUF_DIR = ../../protobuf

SRC := $(wildcard $(PROTOBUF_DIR)/*.c) peep_uart_config.c

CFLAGS = -I$(PROTOBUF_DIR) -O0 -ggdb3 -DPB_BUFFER_ONLY -DPB_FIELD_32BIT -Wall
EXEC = peep_uart_config

.PHONY: clean
all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EXEC)
