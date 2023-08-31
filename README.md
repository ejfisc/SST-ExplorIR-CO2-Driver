# SST Sensing ExplorIR CO2 Sensor Driver

## Getting Started
Create an `explorir_handler_t` instance in main. Implement the `*explorir_tx` in main. Call `explorir_init()`.

## Communicating With The Sensor
The ExplorIR sensor uses a 9600 baudrate UART interface. The `*explorir_tx` function should use UART. The functions for retrieving data from the sensor are defined in the explorir.h file.

## Debug Output
A precompiler directive is used to turn debug output on and off. Currently all of the outputs are using `NRF_LOG_INFO` which is a nordic semi SDK specific function, change these to printf or whatever your micro environment uses. 
