/* ****************************************************************************/
/** ExplorIr CO2 Sensor Function Library

  @Company
    Myriad Sensors

  @File Name
    explorir.h

  @Summary
    Driver Library for ExplorIr CO2 Sensor

  @Description
    Defines functions that allow the user to interact with ExplorIr
******************************************************************************/

#ifndef EXPLORIR_H
#define EXPLORIR_H

#include <stdint.h>

//#define DEBUG_OUTPUT // comment this line out to turn off printf statements
#ifdef DEBUG_OUTPUT
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#endif

/* 
    Size of the uart receive and transmit buffer in bytes
    Use these to configure your UART protocol
*/
#define UART_TX_BUF_SIZE 128
#define UART_RX_BUF_SIZE 128

/*
    Set to the largest value that a 32 bit integer can represent, 
    UART is slow but this was a sufficient max wait time in testing.
    If the micro is waiting this long for a response from the sensor it
    is reasonable to assume that something is wrong. 
*/
#define RESPONSE_TIMEOUT 0x989680

// ascii codes for commands/keywords
#define SET_DIGITAL_FILTER 'A'
#define GET_DIGITAL_FILTER 'a'
#define FINE_TUNE_ZERO_POINT 'F'
#define SET_ZERO_POINT_USING_FRESH_AIR 'G'
#define OPERATION_MODE 'K'
#define SET_TYPE_AND_NUM_OF_DATA_OUTPUTS 'M'
#define SET_CO2_BGROUND_CONCENTRATION 'P'
#define GET_NUM_OF_OUTPUT_DATA_FIELDS 'Q'
#define SET_PRESSURE_AND_CONCENTRATION_COMPENSATION 'S'
#define GET_PRESSURE_AND_CONCENTRATION_COMPENSATION 's'
#define SET_ZERO_POINT_USING_NITROGEN 'U'
#define MANUALLY_SET_ZERO_POINT 'u'
#define SET_ZERO_POINT_USING_KNOWN_GAS 'X'
#define SENSOR_INFO 'Y'
#define FILTERED_CO2_MEASUREMENT 'Z'
#define UNFILTERED_CO2_MEASUREMENT 'z'
#define AUTO_ZERO '@'
#define SCALING_FACTOR '.'
#define TERMINATE '\n'
#define SPACE ' '
#define UNRECOGNIZED_CMD '?'

#define MAX_DIGITAL_FILTER 65365
#define MIN_DIGITAL_FILTER 0
#define DIGITAL_FILTER_DEFAULT 16

#define FILTERED_MASK 4
#define UNFILTERED_MASK 2

// @brief explorir operation modes
typedef enum {
    EXPLORIR_MODE_COMMAND = 0,// sensor sleep mode, waiting for commands but no measurements taken
    EXPLORIR_MODE_STREAMING, // streaming mode
    EXPLORIR_MODE_POLLING, // polling mode
    EXPLORIR_MODE_DEFAULT = EXPLORIR_MODE_COMMAND // default mode is command
} explorir_mode_t;

// @brief explorir return codes
typedef enum {
    /*
	Error: Invalid Mode Type
	Cause: explorir_set_operation_mode() was given an invalid explorir_mode_t
	Action:	- Check luminox_set_operation_mode() is given a valid explorir_mode_t
    */
    EXPLORIR_ERR_INVALID_MODE = 0, 
    /*
	Error: UART Response Timeout
	Cause: explorir library function waited too long for response from sensor
	Action: - Check that the sensor is in not in off mode
		- Check that the micro is sending and receiving data from the sensor
    */
    EXPLORIR_ERR_TIMEOUT,
    EXPLORIR_ERR_UNRECOGNIZED_COMMAND, // unrecognized command
    EXPLORIR_ERR_INVALID_INPUT, // input invalid or outside of range
    EXPLORIR_SUCCESS // message sent or response received successfully
} explorir_retcode_t;

typedef struct {
    uint8_t explorir_data[UART_RX_BUF_SIZE];
    explorir_retcode_t err_code;
    uint16_t scaling_factor;
    uint32_t current_filtered_co2;
    uint32_t current_unfiltered_co2;
    uint32_t digital_filter;
    uint32_t zero_point;
    uint32_t pressure_and_concentration_compensation;
    explorir_mode_t current_mode;
    void(*explorir_tx)(unsigned char *tx, uint8_t size); // must be initialized
} explorir_handler_t;

/*******************************[ High-Level Sensor Functions For General Use ]****************************************/

/*
    @brief ExplorIr initialization sequence

    @note Gets sensor firmware version and serial number, requests scaling factor, turns sensor off and resets variables
*/
void explorir_init(explorir_handler_t * explorir_handler);

/*
    @brief Function to request filtered CO2 measurement from sensor

    @note This value needs to be multiplied by the appropriate scaling factor to derive the ppm value. See the
	‘.’ command.

    @note Response: "Z #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_filtered_co2(explorir_handler_t * explorir_handler);

/*
    @Function to get integer value of most recent filtered co2 measurement
    
    @ret 32-bit integer value
*/
uint32_t explorir_get_filtered_co2(explorir_handler_t * explorir_handler);

/*
    @brief Function to request unfiltered CO2 measurement from sensor

    @note This value needs to be multiplied by the appropriate scaling factor to derive the ppm value. See the
	‘.’ command.

    @note Response: "z #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_unfiltered_co2(explorir_handler_t * explorir_handler);

/*
    @Function to get integer value of most recent unfiltered co2 measurement
    
    @ret 32-bit integer value
*/
uint32_t explorir_get_unfiltered_co2(explorir_handler_t * explorir_handler);

/*
    @brief Function to request the scaling factor
    
    @note Required to convert raw CO2 measurement to ppm

    @note Response: ". #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_scaling_factor(explorir_handler_t * explorir_handler);

/*
    @brief Function to set operation mode

    @param[in] mode Mode type

    @note Response: "K #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_operation_mode(explorir_mode_t mode, explorir_handler_t * explorir_handler);

/*
    @brief Function to set value of digital filter

    @note Default is 16, Range is 0 - 65365, responds with new digital filter value

    @param[in] filter Value of digital filter

    @note Response: "A #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_digital_filter(uint16_t filter, explorir_handler_t * explorir_handler);

/*
    @brief Function to request the current value of the digital filter

    @note Response: "a #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_digital_filter(explorir_handler_t * explorir_handler);

/*
    @brief Function to set zero point using known reading

    @param[in] reported Reported gas concentration

    @param[in] actual Actual gas concentration

    @note Response: "F #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_using_known_reading(uint32_t reported, uint32_t actual, explorir_handler_t * explorir_handler);

/*
    @brief Function to set zero point assuming sensor is in fresh air

    @note Typically 400ppm CO2, but level can be set by user – see P commands.

    @note Response: "G #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_in_fresh_air(explorir_handler_t * explorir_handler);

/*
    @brief Function to set zero point assuming sensor is in 0ppm CO2 such as nitrogen

    @note Response: "U #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_in_nitrogen(explorir_handler_t * explorir_handler);

/*
    @brief Function to force a specific zero point

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @param[in] zero_point Gas concentration to set zero point to

    @note Response: "u #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_manually(uint32_t zero_point, explorir_handler_t * explorir_handler);

/*
    @brief Function to set zero point assuming sensor is in known CO2 concentration

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @param[in] co2_concentration Known CO2 concentration to set zero point with

    @note Response: "X #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_using_known_co2(uint32_t co2_concentration, explorir_handler_t * explorir_handler);

/*
    @brief Function to set the value of CO2 in ppm used for auto-zeroing

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @note The value is entered as a two-byte word, MSB first.
	    MSB = Integer (Concentration/256)
	    LSB = Concentration – (256*MSB)

    @param[in] co2_concentration CO2 value to set

    @note Response: "p 8 #\r\n"
		    "p 9 ##\r\n"
	    Numbers mirror the input

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_co2_for_auto_zeroing(uint32_t co2_concentration, explorir_handler_t * explorir_handler);

/*
    @brief Function to set the value of CO2 in ppm for zero-point setting in fresh air

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @note The value is entered as a two-byte word, MSB first.
	    MSB = Integer (Concentration/256)
	    LSB = Concentration – (256*MSB)

    @param[in] co2_concentration CO2 value to set

    @note Response: "p 10 #\r\n"
		    "p 11 ###\r\n"
	    Numbers mirror the input

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_co2_for_zero_point_in_fresh_air(uint32_t co2_concentration, explorir_handler_t * explorir_handler);

/*
    @brief Function to set the 'Initial Interval' and 'Regular Interval' for auto-zeroing events

    @note Both the initial interval and regular interval are given in days. Both must be entered with a decimal
	    point and one figure after the decimal point. 

    @param[in] initial Initial interval in days

    @param[in] regular Regular interval in days

    @note Response: "@ #.# #.#\r\n"
	    Numbers mirror the input

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_auto_zero_intervals(uint8_t initial, uint8_t regular, explorir_handler_t * explorir_handler);

/*
    @brief Function to disable auto-zeroing

    @note Response: "@ 0\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_disable_auto_zeroing(explorir_handler_t * explorir_handler);

/*
    @brief Function to start an auto-zero immediately

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_start_auto_zero(explorir_handler_t * explorir_handler);

/*
    @brief Function to determine auto zero configuration

    @note Response: "@ #.# #.#\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_auto_zero_config(explorir_handler_t * explorir_handler);

/*
    @brief Function to set the 'Pressure and Concentration Compensation' value

    @param[in] value Pressure and Concetration Compensation value

    @note Response: "S ####\r\n"
	    Numbers mirror the input

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_pressure_and_concentration_compensation(uint32_t value, explorir_handler_t * explorir_handler);

/*
    @brief Function to request the 'Pressure and Concentration Compensation' value

    @note Response: "s ####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_pressure_and_concetration_compensation(explorir_handler_t * explorir_handler);

/*
    @brief Function to set the data types output by the sensor to filtered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_filtered(explorir_handler_t * explorir_handler);

/*
    @brief Function to set the data types output by the sensor to unfiltered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_unfiltered(explorir_handler_t * explorir_handler);

/* 
    @brief Function to set the data types output by the sensor to filtered and unfiltered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_all(explorir_handler_t * explorir_handler);

/*
    @brief Function to request the number of output data fields
    
    @note Response: " Q #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_output_data_fields(explorir_handler_t * explorir_handler);

/*
    @brief Function to request sensor firmware version and serial number

    @note This command returns two lines split by a carriage return line feed and terminated by a carriage
	    return line feed. This command requires that the sensor has been stopped (see ‘K’ command)

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_sensor_info(explorir_handler_t * explorir_handler);

/*
    @brief Function for processing the response from the ExplorIr sensor

    @note The ExplorIr sensor responds in ASCII encoded messages

    @note Updates the current_x variables and err_code
*/
void explorir_process_response(explorir_handler_t * explorir_handler);

/*
    @brief Function for updating the explorir_data array with the most recent response from the ExplorIr sensor

    @param[in] p_response Pointer to the byte string containing the response from the ExplorIr sensor

    @param[in] size Size, in bytes, of the response

    @note call this function in your uart_event_handler when a complete response from the sensor has been recognized
*/
void explorir_update_data(uint8_t * p_response, uint8_t size, explorir_handler_t * explorir_handler);

/*
    @brief Function for waiting for response from sensor
*/
void explorir_wait_for_response(explorir_handler_t * explorir_handler);

#endif // EXPLORIR_H