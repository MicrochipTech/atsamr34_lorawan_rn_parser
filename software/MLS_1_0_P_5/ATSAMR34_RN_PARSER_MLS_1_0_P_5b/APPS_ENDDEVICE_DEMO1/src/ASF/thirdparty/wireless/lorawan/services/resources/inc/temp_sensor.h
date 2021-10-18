/**
* \file  temp_sensor.h
*
* \brief Temperature Sensor service
*		
*
* Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/

#ifndef _TEMP_SENSOR_H
#define _TEMP_SENSOR_H

/***************************** DEFINES ****************************************/
#define ADC_TEMP_SAMPLE_LENGTH				63
#define INT1V_VALUE_FLOAT					1.0
#define INT1V_DIVIDER_1000					1000.0
#define ADC_12BIT_FULL_SCALE_VALUE_FLOAT	4095.0

/*************************** FUNCTIONS PROTOTYPE ******************************/

/*********************************************************************//**
 \brief      Function to initialize the ADC for reading temperature 
			 sensor value
*************************************************************************/
void temp_sensor_init(void);

/*********************************************************************//**
 \brief          Function to read the temperature sensor value
 \param[out]     Pointer to Temperature Sensor output value 
*************************************************************************/
void get_temp_sensor_data(uint8_t *data);

/*---------------------------------------------------------------------------*/
#endif  /* _TEMP_SENSOR_H */
