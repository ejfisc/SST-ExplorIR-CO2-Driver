# SST Sensing ExplorIR CO2 Sensor Driver

## Getting Started
Create an `explorir_handler_t` instance in main. Implement the `*explorir_tx` in main. Call `explorir_init()`.

## Communicating With The Sensor
The ExplorIR sensor uses a 9600 baudrate UART interface. The `*explorir_tx` function should use UART. The functions for retrieving data from the sensor are defined in the explorir.h file.

Recommended `*explorir_tx` implementation (this is psuedocode, modify to fit your specific uart implementation):
```
    void uart_write(unsigned char * tx, uint8_t size) {
        complete_uart_rx = false;
        uart_tx(tx, size);
        while(!complete_uart_rx) {} // set this flag to true in uart event handler
    }
```

If you are not using an interrupt based UART and cannot set that flag to true, get rid of that while true loop in the `uart_write()` implementation and uncomment the `explorir_wait_for_response()` function calls in each of the functions in the source code that talk with the sensor. The micro must wait and process each response before sending another request to the sensor because it can otherwise spam the UART bus with requests and the sensor will not recognize the command and respond with error. This might be solved with hardware flow control but I never tested this.

Add the following code to your super loop in order to process data when the sensor is in streaming mode.
```
    // process explorir data
	if(complete_uart_rx) {
	    explorir_process_response(&explorir);
	    complete_uart_rx = false;
	}
```

## Debug Output
A precompiler directive is used to turn debug output on and off. Currently all of the outputs are using `NRF_LOG_INFO` which is a Nordic nRF5 SDK specific function, change these to printf or whatever your micro environment uses. 
