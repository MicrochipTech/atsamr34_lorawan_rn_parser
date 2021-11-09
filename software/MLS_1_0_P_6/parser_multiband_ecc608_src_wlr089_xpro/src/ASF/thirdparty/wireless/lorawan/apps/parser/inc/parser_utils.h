/**
* \file  parser_utils.h
*
* \brief Parser utils header file
*		
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
 
#ifndef _PARSER_UTILS_H
#define _PARSER_UTILS_H

/****************************** INCLUDES **************************************/  
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/****************************** DEFINES ***************************************/ 
#define PARSER_DEF_WORD_MAX_LEN      106U
#define PARSER_DEF_CMD_MAX_LEN      550U
#define PARSER_DEF_CMD_MAX_IDX      10U
#define PARSER_DEF_CMD_REPLY_LEN    550U
#define PARSER_DEF_DISPATCH_LEN     3U

/*************************** FUNCTIONS PROTOTYPE ******************************/
bool Validate_HexValue(void* pValue);
uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt);
void Parser_IntArrayToHexAscii(uint8_t arrayLen, uint8_t* pInArray, char* pOutHexAscii);
bool Validate_Uint16DecAsciiValue(void* pValue, uint16_t* pDecValue);
bool Validate_Uint8DecAsciiValue(void* pValue, uint8_t* pDecValue);
bool Validate_UintDecAsciiValue(void* pValue, uint8_t DigitsNb, uint32_t maxValue);
uint8_t Validate_OnOffAsciiValue(void* pValue);
uint8_t Validate_Str1Str2AsciiValue(void* pValue,const void* pStr1,const void* pStr2);
int8_t Pin_Index(char* pinName);



/*
 * \brief Converts String to Un-singed Long Integer
 */ 
char * ultoa(char * str,unsigned long num,int radix);

/*
 * \brief Compare Strings without Case Sensitivity
 */ 
int stricmp( char *s1, const char *s2 );

/*
 * \brief Converts the input string consisting of hexadecimal digits into an integer value
 */ 
int xtoi(char *c);
#endif	/* _PARSER_UTILS_H */

