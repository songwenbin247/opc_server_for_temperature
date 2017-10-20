#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "open62541.h"
#include "mbtcp_mstr.h"

UA_Boolean running = true;
char *ip, *port;

static void stopHandler(int sig)
{
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
		    "received ctrl-c");
	running = false;
	free(ip);
	free(port);
}

int main(int argc, char **argv)
{
	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);
	UA_ServerNetworkLayer nl;
	UA_ServerConfig config = UA_ServerConfig_standard;
        
	nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16666);
	
	config.networkLayers = &nl;
	config.networkLayersSize = 1;
	UA_Server *server = UA_Server_new(config);

	if (argc < 3){
		printf("  Usages: \n \topc_server <modbus_ip> <port>\n");
		return 0;
	}

	/*running the server*/
	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	return (int)retval;
}
