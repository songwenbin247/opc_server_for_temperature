SRC_MASTER_TCP = opc_server.c mbtcp_mstr.c mbtcp_func.c
SRC_CLIENT_UPDATA = opc_client_updata.c mbtcp_mstr.c mbtcp_func.c
SRC_SERVER_NO_NODE= opc_server_no_node.c 
FLAG = -lopen62541

all: opc_server opc_client_updata opc_server_no_node
CC = gcc
#CC = aarch64-linux-gnu-gcc
opc_server: $(SRC_MASTER_TCP)
	$(CC) -Wall -g -o $@ ${SRC_MASTER_TCP} ${FLAG}

opc_client_updata: $(SRC_CLIENT_UPDATA)
	$(CC) -Wall -g -o $@ ${SRC_CLIENT_UPDATA} ${FLAG}

opc_server_no_node: $(SRC_SERVER_NO_NODE)
	$(CC) -Wall -g -o $@ ${SRC_SERVER_NO_NODE} ${FLAG}
clean:
	rm -f opc_server opc_client_updata
	 
