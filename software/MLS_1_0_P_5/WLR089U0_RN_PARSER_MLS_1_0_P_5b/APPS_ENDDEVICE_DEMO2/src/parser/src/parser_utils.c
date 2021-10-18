/**
* \file  parser_utils.c
*
* \brief Parser Utils file
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
 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser_utils.h"

/* String reply values */
#define OFF_STR_IDX         0U // 0 = false;
#define ON_STR_IDX          1U // 1 = true;
#define INVALID_STR_IDX     2U // 2 = invalid parameter

#define FIRST_STR_IDX    0U
#define SECOND_STR_IDX   1U
#define NONE_STR_IDX     2U

static const char* gapParseOnOff[] =
{
    "off",
    "on"
};

bool Validate_HexValue(void* pValue)
 {
    bool flag = true;
    char* character;

    for(character = pValue; *character; character++)
    {
        if(isxdigit(*character) == 0)
        {
            flag = false;
            break;
        }
    }

    return flag;
}

uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt)
{
    uint16_t rxHexAsciiLen = strlen(pInHexAscii);
    uint16_t iCtr = 0;
    uint16_t jCtr = rxHexAsciiLen >> 1;
    uint8_t retValue = 0;
    char tempBuff[3];

    if(rxHexAsciiLen % 2 == 0)
    {
        jCtr --;
    }

    if(hexAsciiLen == rxHexAsciiLen)
    {
        while(rxHexAsciiLen > 0)
        {
            if(rxHexAsciiLen >= 2U)
            {
                tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 2));
                iCtr ++;
                tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

                rxHexAsciiLen -= 2U;
            }
            else
            {
                tempBuff[iCtr] = '0';
                iCtr ++;
                tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

                rxHexAsciiLen --;
            }

            iCtr ++;
            tempBuff[iCtr] = '\0';
            *(pOutInt + jCtr) = xtoi(tempBuff); 
            iCtr = 0;
            jCtr --;
        }

        retValue = 1;
    }

    return retValue;
}

void Parser_IntArrayToHexAscii(uint8_t arrayLen, uint8_t* pInArray, char* pOutHexAscii)
{
    uint8_t iCtr = 0U;

    for(iCtr = 0; iCtr < arrayLen; iCtr ++)
    {
        itoa(pInArray[iCtr], (char *)&pOutHexAscii[iCtr << 1], 16);

        if(pInArray[iCtr] <= 15)
        {
            /* Special treatment for figures [0..9]  */
            pOutHexAscii[(iCtr << 1) + 1] = pOutHexAscii[iCtr << 1];
            pOutHexAscii[iCtr << 1] = '0';
        }
    }

    pOutHexAscii[(iCtr << 1)] = '\0';
}

bool Validate_Uint8DecAsciiValue(void* pValue, uint8_t* pDecValue)
{
    bool flag = false;
    char* character;
    uint16_t valueLen = strlen(pValue);
    int32_t value = atoi(pValue);

    // Maximum 3 ascii characters 0-> 255
    if((valueLen <= 3U) && (value >= 0) && (value <= 255))
    {
        flag = true;

        for(character = pValue; *character; character++)
        {
            if(isdigit(*character) == 0)
            {
                flag = false;
                break;
            }
        }
    }

    if(flag)
    {
        *pDecValue = (uint8_t)value;
    }

    return flag;
}

bool Validate_Uint16DecAsciiValue(void* pValue, uint16_t* pDecValue)
{
	bool flag = false;
	char* character;
	uint16_t valueLen = strlen(pValue);
	int32_t value = atoi(pValue);

	// Maximum 5 ascii characters 0-> 65535
	if((valueLen <= 5U) && (value >= 0) && (value <= 65535))
	{
		flag = true;

		for(character = pValue; *character; character++)
		{
			if(isdigit(*character) == 0)
			{
				flag = false;
				break;
			}
		}
	}

	if(flag)
	{
		*pDecValue = (uint16_t)value;
	}

	return flag;
}


bool Validate_UintDecAsciiValue(void* pValue, uint8_t DigitsNb, uint32_t maxValue)
{
    bool flag = false;
    char buf[11];
    unsigned long value = strtoul(pValue, NULL, 10);

    ultoa(buf, value, 10U);
    
    flag = true;
    
    if (0 != strcmp((char*)pValue, buf))
    {
        flag = false;
    }
    
    if (value > maxValue)
    {
        flag = false;
    }

    return flag;
}

// Possible return values:
// 0 = false;
// 1 = true;
// 2 = invalid parameter
uint8_t Validate_OnOffAsciiValue(void* pValue)
{
    uint8_t result = INVALID_STR_IDX;


    if(0U == stricmp(pValue, gapParseOnOff[OFF_STR_IDX]))
    {
        result = OFF_STR_IDX;
    }
    else if (0U == stricmp(pValue, gapParseOnOff[ON_STR_IDX]))
    {
        result = ON_STR_IDX;
    }

    return result;
}

// Possible return values:
// 0 = pValue equal to pStr1;
// 1 = pValue equal to pStr2;
// 2 = pValue not equal to pStr1 or pStr1
uint8_t Validate_Str1Str2AsciiValue(void* pValue,const void* pStr1,const void* pStr2)
{
    uint8_t result = NONE_STR_IDX;


    if(0U == stricmp(pValue, pStr1))
    {
        result = FIRST_STR_IDX;
    }
    else if (0U == stricmp(pValue, pStr2))
    {
        result = SECOND_STR_IDX;
    }

    return result;
}

int8_t Pin_Index(char* pinName)
{
    int8_t pinNumber = -1;
    
/*    if(0 == strncmp(pinName, "GPIO", 4))
    {
        if(Validate_UintDecAsciiValue(&pinName[4], 2, 13))
        {
            pinNumber = atoi(&pinName[4]);
        }
    }
    else if (0 == strcmp(pinName, "UART_RTS"))
    {
        pinNumber = 14;
    }
    else if (0 == strcmp(pinName, "UART_CTS"))
    {
        pinNumber = 15;
    }
    else if (0 == strcmp(pinName, "TEST0"))
    {
        pinNumber = 16;
    }
    else if (0 == strcmp(pinName, "TEST1"))
    {
        pinNumber = 17;
    }
*/    
    return pinNumber;
}

/*
 * \brief Converts String to Un-singed Long Integer
 */ 
char * ultoa(char * str, unsigned long num,  int radix)
{
	/* An int can only be 16 bits long at radix 2 (binary) the string
	  is at most 16 + 1 null long. */	
   char temp[33];  
   int temp_loc = 0;
   int digit;
   int str_loc = 0;

   /*construct a backward string of the number. */
   do {
	   digit = (unsigned long)num % radix;
	   if (digit < 10)
	   temp[temp_loc++] = digit + '0';
	   else
	   temp[temp_loc++] = digit - 10 + 'A';
	   num = ((unsigned long)num) / radix;
   } while ((unsigned long)num > 0);

   temp_loc--;


   /* now reverse the string. */
   while ( temp_loc >=0 ) {
	   /* while there are still chars */
	   str[str_loc++] = temp[temp_loc--];
   }
   /* add null termination. */
   str[str_loc] = 0; 

   return str;
}

/*
 * \brief Compare Strings without Case Sensitivity
 */ 
int stricmp( char *s1, const char *s2 )
{
	if (s1 == NULL) return s2 == NULL ? 0 : -(*s2);
	if (s2 == NULL) return *s1;

	char c1, c2;
	while ((c1 = tolower (*s1)) == (c2 = tolower (*s2)))
	{
		if (*s1 == '\0') break;
		++s1; ++s2;
	}

	return c1 - c2;
}

/*
 * \brief Converts the input string consisting of hexadecimal digits into an integer value
 */ 
int xtoi(char *c)
{
  size_t szlen = strlen(c);
  int idx, ptr, factor,result =0;

  if(szlen > 0){
    if(szlen > 8) return 0;
    result = 0;
    factor = 1;

    for(idx = szlen-1; idx >= 0; --idx){
    if(isxdigit( *(c+idx))){
	if( *(c + idx) >= 97){
	  ptr = ( *(c + idx) - 97) + 10;
	}else if( *(c + idx) >= 65){
	  ptr = ( *(c + idx) - 65) + 10;
	}else{
	  ptr = *(c + idx) - 48;
	}
	result += (ptr * factor);
	factor *= 16;
    }else{
		return 4;
    }
    }
  }

  return result;
}