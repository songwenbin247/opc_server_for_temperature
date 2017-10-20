SRC_MASTER_TCP = opc_server.c mbtcp_mstr.c mbtcp_func.c
FLAG = -lopen62541

all: opc_server
CC = gcc
#CC = aarch64-linux-gnu-gcc
opc_server: $(SRC_MASTER_TCP)
	$(CC) -Wall -g -o $@ ${SRC_MASTER_TCP} ${FLAG}

clean:
	rm -f opc_server
	 
