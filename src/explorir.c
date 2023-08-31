/* ****************************************************************************/
/** ExplorIr CO2 Sensor Function Library

  @File Name
    explorir.c

  @Summary
    Driver Library for ExplorIr CO2 Sensor

  @Description
    Implements functions that allow the user to interact with ExplorIr
******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "explorir.h"

extern volatile bool explorir_complete_uart_rx;

/*******************************[ High-Level Sensor Functions For General Use ]****************************************/

/*
    @brief ExplorIr initialization sequence

    @note Gets sensor firmware version and serial number, requests scaling factor, turns sensor off and resets variables
*/
void explorir_init(explorir_handler_t * explorir_handler) {
#ifdef DEBUG_OUTPUT
    NRF_LOG_INFO("ExplorIR Initialization...");
    NRF_LOG_FLUSH();
#endif
    explorir_handler->err_code = explorir_set_operation_mode(EXPLORIR_MODE_COMMAND, explorir_handler);

    explorir_handler->err_code = explorir_request_sensor_info(explorir_handler);

    explorir_handler->err_code = explorir_request_scaling_factor(explorir_handler);

    explorir_handler->err_code = explorir_set_digital_filter(DIGITAL_FILTER_DEFAULT, explorir_handler);

    explorir_handler->err_code = explorir_request_pressure_and_concetration_compensation(explorir_handler);

    explorir_handler->err_code = explorir_set_output_data_all(explorir_handler);

    explorir_handler->err_code = explorir_set_operation_mode(EXPLORIR_MODE_DEFAULT, explorir_handler);

    explorir_handler->current_mode = EXPLORIR_MODE_DEFAULT;
    explorir_handler->current_filtered_co2 = 0;
    explorir_handler->current_unfiltered_co2 = 0;
    explorir_handler->digital_filter = DIGITAL_FILTER_DEFAULT;

}

/*
    @brief Function to request filtered CO2 measurement from sensor

    @note This value needs to be multiplied by the appropriate scaling factor to derive the ppm value. See the
	‘.’ command.

    @note Response: "Z #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_filtered_co2(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "Z\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @Function to get integer value of most recent filtered co2 measurement
    
    @ret 32-bit integer value
*/
uint32_t explorir_get_filtered_co2(explorir_handler_t * explorir_handler) {
    return explorir_handler->current_filtered_co2;
}

/*
    @brief Function to request unfiltered CO2 measurement from sensor

    @note This value needs to be multiplied by the appropriate scaling factor to derive the ppm value. See the
	‘.’ command.

    @note Response: "z #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_unfiltered_co2(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "z\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @Function to get integer value of most recent unfiltered co2 measurement
    
    @ret 32-bit integer value
*/
uint32_t explorir_get_unfiltered_co2(explorir_handler_t * explorir_handler) {
    return explorir_handler->current_unfiltered_co2;
}

/*
    @brief Function to request the scaling factor
    
    @note Required to convert raw CO2 measurement to ppm

    @note Response: ". #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_scaling_factor(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = ".\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set operation mode

    @param[in] mode Mode type

    @note Response: "K #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_operation_mode(explorir_mode_t mode, explorir_handler_t * explorir_handler) {
    char msg[] = "K x\r\n";
    // set command to given mode
    switch(mode) {
	case EXPLORIR_MODE_STREAMING:
	    msg[2] = '1';
	    break;
	case EXPLORIR_MODE_POLLING:
	    msg[2] = '2';
	    break;
	case EXPLORIR_MODE_COMMAND:
	    msg[2] = '0';
	    break;
	default:
	    return EXPLORIR_ERR_INVALID_MODE;
    }

    explorir_handler->explorir_tx(msg, 5); // transmit message

    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set value of digital filter

    @note Default is 16, Range is 0 - 65365, responds with new digital filter value

    @param[in] filter Value of digital filter

    @note Response: "A #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_digital_filter(uint16_t filter, explorir_handler_t * explorir_handler) {
    if(filter > MAX_DIGITAL_FILTER || filter < MIN_DIGITAL_FILTER) {
	return EXPLORIR_ERR_INVALID_INPUT;
    }
    
    char buf[8];
    uint8_t str_size = sprintf(buf, "%d", filter);

    // compile message
    uint8_t msg_size = 4 + str_size;
    unsigned char msg[msg_size];
    msg[0] = 'A';
    msg[1] = ' ';
    for(uint8_t i = 0; i < str_size; i++) {
	msg[i+2] = buf[i];
    }
    msg[msg_size-2] = '\r';
    msg[msg_size-1] = '\n';

    explorir_handler->explorir_tx(msg, msg_size); // transmit the message

    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to request the current value of the digital filter

    @note Response: "a #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_digital_filter(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "a\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set zero point using known reading

    @param[in] reported Reported gas concentration

    @param[in] actual Actual gas concentration

    @note Response: "F #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_using_known_reading(uint32_t reported, uint32_t actual, explorir_handler_t * explorir_handler) {
    // TODO need to figure out how to turn inputs into string to send to sensor
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set zero point assuming sensor is in fresh air

    @note Typically 400ppm CO2, but level can be set by user – see P commands.

    @note Response: "G #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_in_fresh_air(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "G\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set zero point assuming sensor is in 0ppm CO2 such as nitrogen

    @note Response: "U #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_in_nitrogen(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "U\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to force a specific zero point

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @param[in] zero_point Gas concentration to set zero point to

    @note Response: "u #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_manually(uint32_t zero_point, explorir_handler_t * explorir_handler) {
    char buf[8];
    uint8_t str_size = sprintf(buf, "%d", zero_point);

    // compile message
    uint8_t msg_size = 4 + str_size;
    unsigned char msg[msg_size];
    msg[0] = 'u';
    msg[1] = ' ';
    for(uint8_t i = 0; i < str_size; i++) {
	msg[i+2] = buf[i];
    }
    msg[msg_size-2] = '\r';
    msg[msg_size-1] = '\n';

    explorir_handler->explorir_tx(msg, msg_size); // transmit the message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set zero point assuming sensor is in known CO2 concentration

    @note Input value is scaled by CO2 value multiplier, see ‘.’ command.

    @param[in] co2_concentration Known CO2 concentration to set zero point with

    @note Response: "X #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_zero_point_using_known_co2(uint32_t co2_concentration, explorir_handler_t * explorir_handler) {
    char buf[8];
    uint8_t str_size = sprintf(buf, "%d", co2_concentration);

    // compile message
    uint8_t msg_size = 4 + str_size;
    unsigned char msg[msg_size];
    msg[0] = 'X';
    msg[1] = ' ';
    for(uint8_t i = 0; i < str_size; i++) {
	msg[i+2] = buf[i];
    }
    msg[msg_size-2] = '\r';
    msg[msg_size-1] = '\n';

    explorir_handler->explorir_tx(msg, msg_size); // transmit the message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

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
explorir_retcode_t explorir_set_co2_for_auto_zeroing(uint32_t co2_concentration, explorir_handler_t * explorir_handler) {
    // TODO need to figure out how to turn input into string to send to sensor
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

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
explorir_retcode_t explorir_set_co2_for_zero_point_in_fresh_air(uint32_t co2_concentration, explorir_handler_t * explorir_handler) {
    // TODO need to figure out how to turn input into string to send to sensor
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

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
explorir_retcode_t explorir_set_auto_zero_intervals(uint8_t initial, uint8_t regular, explorir_handler_t * explorir_handler) {
    if(initial < 0 || initial > 9 || regular < 0 || regular > 9)
	return EXPLORIR_ERR_INVALID_INPUT;

    char initial_char;
    char regular_char;
    sprintf(&initial_char, "%d", initial);
    sprintf(&regular_char, "%d", regular);

    unsigned char msg[] = "@ x.0 x.0\r\n";
    msg[2] = initial_char;
    msg[6] = regular_char;

    explorir_handler->explorir_tx(msg, 10); // transmit the message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to disable auto-zeroing

    @note Response: "@ 0\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_disable_auto_zeroing(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "@ 0\r\n";
    explorir_handler->explorir_tx(msg, 5); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to start an auto-zero immediately

    @note This is according to the datasheet, no more information is given

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_start_auto_zero(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "65222\r\n";
    explorir_handler->explorir_tx(msg, 7); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to determine auto zero configuration

    @note Response: "@ #.# #.#\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_auto_zero_config(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "@\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set the 'Pressure and Concentration Compensation' value

    @param[in] value Pressure and Concetration Compensation value

    @note Response: "S ####\r\n"
	    Numbers mirror the input

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_pressure_and_concentration_compensation(uint32_t value, explorir_handler_t * explorir_handler) {
    char buf[8];
    uint8_t str_size = sprintf(buf, "%d", value);

    // compile message
    uint8_t msg_size = 4 + str_size;
    unsigned char msg[msg_size];
    msg[0] = 'S';
    msg[1] = ' ';
    for(uint8_t i = 0; i < str_size; i++) {
	msg[i+2] = buf[i];
    }
    msg[msg_size-2] = '\r';
    msg[msg_size-1] = '\n';

    explorir_handler->explorir_tx(msg, msg_size); // transmit the message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to request the 'Pressure and Concentration Compensation' value

    @note Response: "s ####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_pressure_and_concetration_compensation(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "s\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set the data types output by the sensor to filtered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_filtered(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "M 00004\r\n";
    explorir_handler->explorir_tx(msg, 9); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to set the data types output by the sensor to unfiltered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_unfiltered(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "M 00002\r\n";
    explorir_handler->explorir_tx(msg, 9); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/* 
    @brief Function to set the data types output by the sensor to filtered and unfiltered CO2

    @note Measurement Parameter Field Identifier Mask Value
	    CO2 (Filtered)	      Z		    4
	    CO2 (Unfiltered)	      z		    2

    @note Response: "M #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_set_output_data_all(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "M 00006\r\n";
    explorir_handler->explorir_tx(msg, 9); // transmit message

    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to request the number of output data fields
    
    @note Response: " Q #####\r\n"

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_output_data_fields(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "Q\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message
    
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function to request sensor firmware version and serial number

    @note This command returns two lines split by a carriage return line feed and terminated by a carriage
	    return line feed. This command requires that the sensor has been stopped (see ‘K’ command)

    @ret ExplorIr return code, either SUCCESS or failure
*/
explorir_retcode_t explorir_request_sensor_info(explorir_handler_t * explorir_handler) {
    unsigned char msg[] = "Y\r\n";
    explorir_handler->explorir_tx(msg, 3); // transmit message

    // wait for firmware version
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);

    // wait for serial number
    //explorir_wait_for_response(explorir_handler);
    explorir_process_response(explorir_handler);
    
    return explorir_handler->err_code;
}

/*
    @brief Function for processing the response from the ExplorIr sensor

    @note The ExplorIr sensor responds in ASCII encoded messages

    @note Updates the current_x variables and err_code
*/
void explorir_process_response(explorir_handler_t * explorir_handler) {
    uint16_t i = 0;
    while(explorir_handler->explorir_data[i] != TERMINATE) {
	switch(explorir_handler->explorir_data[i]) {
	    case SCALING_FACTOR:
		uint8_t scaling_factor_data[5] = {0};
		i += 2; // increment by 2 to move index to start of scaling factor
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(scaling_factor_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->scaling_factor = atoi(scaling_factor_data);
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Scaling Factor: %d ", explorir_handler->scaling_factor);
		NRF_LOG_FLUSH();
#endif
		goto EndWhile;
	    case FILTERED_CO2_MEASUREMENT:
		uint8_t filtered_co2_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(filtered_co2_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->current_filtered_co2 = atoi(filtered_co2_data) * explorir_handler->scaling_factor;
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Filtered CO2: %d ppm ", explorir_handler->current_filtered_co2);
		NRF_LOG_FLUSH();
#endif	
		i = 9;
		break;
	    case UNFILTERED_CO2_MEASUREMENT:
		uint8_t unfiltered_co2_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(unfiltered_co2_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->current_unfiltered_co2 = atoi(unfiltered_co2_data) * explorir_handler->scaling_factor;
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Unfiltered CO2: %d ppm ", explorir_handler->current_unfiltered_co2);
		NRF_LOG_FLUSH();
#endif	
		goto EndWhile;
	    case OPERATION_MODE:
		uint8_t operation_mode_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(operation_mode_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->current_mode = atoi(operation_mode_data);
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Current Mode: %d ", explorir_handler->current_mode);
		NRF_LOG_FLUSH();
#endif	
		goto EndWhile;
	    case SET_DIGITAL_FILTER:
	    case GET_DIGITAL_FILTER:
		uint8_t digital_filter_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(digital_filter_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->digital_filter = atoi(digital_filter_data);
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Digital Filter: %d ", explorir_handler->digital_filter);
		NRF_LOG_FLUSH();
#endif	
		goto EndWhile;
	    case FINE_TUNE_ZERO_POINT:
	    case SET_ZERO_POINT_USING_FRESH_AIR:
	    case SET_ZERO_POINT_USING_KNOWN_GAS:
	    case SET_ZERO_POINT_USING_NITROGEN:
	    case MANUALLY_SET_ZERO_POINT:
		uint8_t zero_point_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(zero_point_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->zero_point = atoi(zero_point_data);
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Zero Point: %d ", explorir_handler->zero_point);
		NRF_LOG_FLUSH();
#endif	
		goto EndWhile;
	    case SET_PRESSURE_AND_CONCENTRATION_COMPENSATION:
	    case GET_PRESSURE_AND_CONCENTRATION_COMPENSATION:
		uint8_t pcc_data[5] = {0};
		i += 2;
		while(explorir_handler->explorir_data[i] == '0') {
		    i++; // remove leading 0s
		}
		memcpy(pcc_data, &explorir_handler->explorir_data[i], sizeof(explorir_handler->explorir_data[i]) * 5);
		explorir_handler->pressure_and_concentration_compensation = atoi(pcc_data);
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("Pressure and Concentration Compensation: %d ", explorir_handler->pressure_and_concentration_compensation);
		NRF_LOG_FLUSH();
#endif	
		goto EndWhile;
	    case SPACE:
		i++;
		break;
	    default:
#ifdef DEBUG_OUTPUT
		NRF_LOG_INFO("%c", explorir_handler->explorir_data[i]);
		NRF_LOG_FLUSH();
#endif
		i++;
		break;
	}
    }
    EndWhile: ;
#ifdef DEBUG_OUTPUT
    NRF_LOG_INFO("");
    NRF_LOG_FLUSH();
#endif
    memset(explorir_handler->explorir_data, 0, sizeof(explorir_handler->explorir_data));
}

/*
    @brief Function for updating the explorir_data array with the most recent response from the ExplorIr sensor

    @param[in] p_response Pointer to the byte string containing the response from the ExplorIr sensor

    @param[in] size Size, in bytes, of the response

    @note call this function in your uart_event_handler when a complete response from the sensor has been recognized
*/
void explorir_update_data(uint8_t * p_response, uint8_t size, explorir_handler_t * explorir_handler) {
    memcpy(explorir_handler->explorir_data, p_response, size);
}

/*
    @brief Function for waiting for response from sensor
*/
void explorir_wait_for_response(explorir_handler_t * explorir_handler) {
    // wait for response
    uint32_t timer = 0;
    while(!explorir_complete_uart_rx) {
	if(timer >= RESPONSE_TIMEOUT) {
	    explorir_handler->err_code = EXPLORIR_ERR_TIMEOUT; // timeout
	    break;
	}   
	timer++;
    }
    explorir_complete_uart_rx = false; // reset flag
}