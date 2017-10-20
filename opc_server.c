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
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
	running = false;
	free(ip);
	free(port);
}


static UA_StatusCode 
incremental(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *value)
{
	if(range){
		value->hasStatus = true;
		value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
		return UA_STATUSCODE_GOOD;
	}
	UA_Int32 temperature = get_temperature(ip, port);
	UA_Variant_setScalarCopy(&(value->value), &temperature, &UA_TYPES[UA_TYPES_INT32]);

	value->hasValue = true;
	if(sourceTimeStamp){
		value->sourceTimestamp = true;
		value->sourceTimestamp = UA_DateTime_now();
	}
	return UA_STATUSCODE_GOOD;

}

int main(int argc, char **argv)
{
	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);
	UA_ServerNetworkLayer nl =                                         UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16666);
	UA_ServerConfig config = UA_ServerConfig_standard;
	config.networkLayers = &nl;
	config.networkLayersSize = 1;
	UA_Server *server = UA_Server_new(config);

	if (argc < 3){
		printf("  Usages: \n \topc_server <modbus_ip> <port>\n");
		return 0;
	}
	
	ip = strndup(argv[1],strlen("255.255.255.255"));
	port = strndup(argv[2],strlen("65536"));
	
	/*Add my node*/
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "my displayname");
	UA_Int32 myInteger = 56;
	UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
	attr.accessLevel = UA_ACCESSLEVELMASK_HISTORYREAD | UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "the.answer");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "my qualified name");
	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId, parentReferenceNodeId,browseName ,variableType, attr, NULL, NULL);
	UA_Variant_deleteMembers(&attr.value);

	/*Add a variable with the a incremental source*/
	UA_DataSource myDataSource = (UA_DataSource){.handle = NULL, .read = incremental, .write = NULL};
	UA_VariableAttributes v_attr;
	UA_VariableAttributes_init(&v_attr);
	v_attr.description = UA_LOCALIZEDTEXT("en-US", "get temperature via modbus");
	v_attr.displayName = UA_LOCALIZEDTEXT("en-US", "temperature");
	v_attr.accessLevel = UA_ACCESSLEVELMASK_READ ;
	const UA_QualifiedName dateName = UA_QUALIFIEDNAME(1, "temperature");
	UA_NodeId dataSourceId;
	UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
				UA_NODEID_NUMERIC(0,UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), 
				dateName, UA_NODEID_NULL, v_attr,
			       	myDataSource, &dataSourceId);
	/*running the server*/
	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	return (int)retval;
}
