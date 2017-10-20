# opc_server_for_temperature

1. Install open62541 library.
    git clone https://github.com/open62541/open62541.git
    cd open62541
    ./configure && make && sudo make install

2. Get opc_server_for_temperatur and run it.
    git clone https://github.com/songwenbin247/opc_server_for_temperature.git
    cd opc_server_for_temperatur 
    make

3. opc_server:
    a. opc_server run on the device, for example, ls1046rdb: 
        ./opc_server <modbus ip> <port>  // ip = 10.193.20.80 port = 26
    b. Run Prosys OPC UA Client on host to browse all kinds of nodes under Objects.

4. opc_client_updata mode.
    a. run opc_server_no_node on the host.
    	./opc_server_no_node // ip = 10.193.20.80 port = 26
    b. run opc_client_updata on device end.
    	./opc_client_updata <uri> <modbus ip> <port> // uri = opc.tcp://10.193.20.80:16666 
							ip = 10.193.20.80 port = 26
    	opc_client_updata will create new node on opc_server_no_node server and update the temperature date. 
    c. Run Prosys OPC UA Client on other host, right chick the Temperature-updata node under Ojects, select Minitor to check the temperature.


    	
								

