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

static void 
beforeReadTemperatue(void *handle, const UA_NodeId nodeId,
		     const UA_Variant *data, const UA_NumericRange *range)
{
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 temperature = get_temperature(ip, port);
	UA_Variant value;
	UA_Variant_setScalarCopy(&value, &temperature,
				 &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "temperatureCall");
	UA_Server_writeValue(server, tempNodeId, value);

} 

static void 
afterUpdateTemperatue(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
		    "The temperature has been updated");
}

static UA_StatusCode 
getTemperature(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
	      const UA_NumericRange *range, UA_DataValue *value)
{
	if(range){
		value->hasStatus = true;
		value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
		return UA_STATUSCODE_GOOD;
	}

	UA_Int32 temperature = get_temperature(ip, port);
	UA_Variant_setScalarCopy(&(value->value), &temperature, 
				 &UA_TYPES[UA_TYPES_INT32]);

	value->hasValue = true;
	if(sourceTimeStamp){
		value->sourceTimestamp = true;
		value->sourceTimestamp = UA_DateTime_now();
	}

	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode 
getTemperatureMethod(void *handle, const UA_NodeId nodeId, size_t inputSize,
		       const UA_Variant *input, size_t optputSize, UA_Variant *output)
{
	UA_Int32 temperature = get_temperature(ip, port);
	UA_Variant_setScalarCopy(output, &temperature, &UA_TYPES[UA_TYPES_INT32]);
	return UA_STATUSCODE_GOOD;
}

static void
addGetTemperatureCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "TemperatureCall");
	UA_Int32 temperature = get_temperature(ip, port);
	UA_Variant_setScalar(&attr.value, &temperature, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "temperatureCall");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "temperatureCall");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = beforeReadTemperatue;
	callback.onWrite = afterUpdateTemperatue;
	UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
}

static void
addGetTemperatureDataSource(UA_Server *server)
{
	/*Add a variable with the a incremental source*/
	UA_DataSource myDataSource = (UA_DataSource){
		.handle = NULL, 
		.read = getTemperature, 
		.write = NULL
	};
	
	UA_VariableAttributes v_attr;
	UA_VariableAttributes_init(&v_attr);
	v_attr.description = UA_LOCALIZEDTEXT("en-US", "Temperature Datesource");
	v_attr.displayName = UA_LOCALIZEDTEXT("en-US", "temperature-Datesource");
	v_attr.accessLevel = UA_ACCESSLEVELMASK_READ ;
	const UA_QualifiedName dateName = UA_QUALIFIEDNAME(1, "temperature-Datesource");
	UA_NodeId dataSourceId;
	UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
				UA_NODEID_NUMERIC(0,UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), 
				dateName, UA_NODEID_NULL, v_attr,
			       	myDataSource, &dataSourceId);
}	

static void
addGetTemperatureMethod(UA_Server *server)
{
	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en_US", "temperature");
	outputArgument.name = UA_STRING("Temperature");
	outputArgument.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	outputArgument.valueRank = -1;

	UA_MethodAttributes get_temp;
	UA_MethodAttributes_init(&get_temp);

	get_temp.description = UA_LOCALIZEDTEXT("en_US","Get temperature");
	get_temp.displayName = UA_LOCALIZEDTEXT("en_US", "Get temperature");
	get_temp.executable = true;
	get_temp.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_STRING(1,"temperature method\n"),
				UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
				UA_QUALIFIEDNAME(1, "get_temperature"), get_temp, 
				&getTemperatureMethod, NULL, 0, NULL, 1, &outputArgument, NULL);
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
	
	ip = strndup(argv[1],strlen("255.255.255.255"));
	port = strndup(argv[2],strlen("65536"));

	addGetTemperatureDataSource(server);
	addGetTemperatureCallback(server);
	addGetTemperatureMethod(server);	

	/*running the server*/
	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	return (int)retval;
}
