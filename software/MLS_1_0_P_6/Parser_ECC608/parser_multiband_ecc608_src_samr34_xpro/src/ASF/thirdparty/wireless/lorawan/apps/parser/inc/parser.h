/**
* \file  parser.h
*
* \brief Main parser file
*
* Copyright (c) 2020 Microchip Technology Inc. and its subsidiaries. 
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
 
#ifndef _PARSER_H
#define _PARSER_H

#include <stdint.h>
#include "pds_interface.h"

typedef enum _appPdsEntries
{
	PDS_APP_CUSTOMPARAMETER = (PDS_FILE_APP_DATA1_13_IDX << 8),
	PDS_APP_FILEID_MAX_VALUE
} AppPdsEntries_t;

#define PARSER_MAX_DATA_LEN 530U

typedef union
{
    uint32_t value;
    uint8_t buffer[4];
} auint32_t;


typedef union
{
    uint16_t value;
    struct
    {
        uint16_t deveui       : 1;
        uint16_t joineui       : 1;
        uint16_t appkey       : 1;
        uint16_t devaddr      : 1;
        uint16_t nwkskey      : 1;
        uint16_t appskey      : 1;
		uint16_t mcastdevaddr : 1;
		uint16_t mcastnwkskey : 1;
		uint16_t mcastappskey : 1;
		uint16_t mcastfreq    : 1;
		uint16_t mcastdr      : 1;
    } flags;
} parserConfiguredJoinParameters_t;


extern char aParserData[PARSER_MAX_DATA_LEN];

void Parser_Init(void);
void Parser_Main(void);

void Parser_GetSwVersion(char* pBuffData);

uint8_t Parser_GetConfiguredJoinParameters(void);
void Parser_SetConfiguredJoinParameters(uint8_t val);
void parser_serial_data_handler(void);


#endif /* _PARSER_H */