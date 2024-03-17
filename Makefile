include ../nanopb/extra/nanopb.mk

CFLAGS = -ansi -Wall -g -00
CFLAGS = -I$(NANOPB_DIR)

all: server client

.SUFFIXES:

clean:
	rm -f server client message.pb.c message.pb.h

client: client.c common.c message.pb.c 
	$(CC) $(CFLAGS) -o $@ $^ $(NANOPB_CORE)

server: server.c common.c message.pb.c helper.c 
	$(CC) $(CFLAGS) -o $@ $^ $(NANOPB_CORE)

# %: %.c common.c message.pb.c 
# 	$(CC) $(CFLAGS) -o $@ $^ $(NANOPB_CORE)
