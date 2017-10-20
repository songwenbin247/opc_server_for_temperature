# opc_server_for_temperature

1. Install open62541 library.
    git clone https://github.com/open62541/open62541.git
    cd open62541
    ./configure && make && sudo make install
 2. Get opc_server_for_temperatur and run it.
    git clone https://github.com/songwenbin247/opc_server_for_temperature.git
    cd opc_server_for_temperatur 
    make
    ./opc_server <modbus ip> <port>
    
