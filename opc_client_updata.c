#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "open62541.h"
#include "mbtcp_mstr.h"

int main(int argc, char **argv)
{
	UA_Client *client = UA_Client_new(UA_ClientConfig_standard);

	UA_EndpointDescription *endpointArray = NULL;
	size_t endpointArraySize = 0;
	UA_StatusCode retval = UA_Client_getEndpoints(client, argv[1], &endpointArraySize, &endpointArray);
	if (retval != UA_STATUSCODE_GOOD){
		UA_Array_delete(endpointArray, endpointArraySize,
				&UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
		UA_Client_delete(client);
		return (int)retval;
	}

	if (argc < 4){
		printf("  Usages: \n \topc_client_updata <uri> <modbus_ip> <port>\n");
		return 0;
		
	}
	
	retval = UA_Client_connect(client, argv[1]);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_Client_delete(client);
		return (int)retval;
	}

	UA_Int32 value = get_temperature(argv[2], argv[3]);
	UA_NodeId var_id;
	UA_VariableAttributes var_attr;
	UA_VariableAttributes_init(&var_attr);
	var_attr.displayName = 
		UA_LOCALIZEDTEXT("en_US", "temperature-updata");
	var_attr.description =
		UA_LOCALIZEDTEXT("en_US", "updata from client");
	UA_Variant_setScalar(&var_attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
	var_attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	var_attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	retval = UA_Client_addVariableNode(client,
				       UA_NODEID_STRING(1, "temperature-updata"), 
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(0, "temperature-updata"),
                                       UA_NODEID_NULL, // no variable type
                                       var_attr, &var_id);
	
	if(retval != UA_STATUSCODE_GOOD){
		printf("Create node \"temperature-updata\" fail.\n");
		return 0;
	}

	UA_Variant *myVariant = UA_Variant_new();
	UA_Int32 old_value = value;
 	while (1){
		usleep(10000);
		value = get_temperature(argv[2], argv[3]);
		if (value == old_value)
			continue;	
		old_value = value;
		UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
		UA_Client_writeValueAttribute(client,
				UA_NODEID_STRING(1, "temperature-updata"), myVariant);
	}

}

