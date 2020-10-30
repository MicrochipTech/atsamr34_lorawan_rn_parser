/**
* \file  parser_system.h
*
* \brief Parser system header file
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
 
#ifndef _PARSER_SYSTEM_H
#define _PARSER_SYSTEM_H

#include "parser_private.h"

void Parser_SystemGetVer(parserCmdInfo_t* pParserCmdInfo);
void Parser_SystemReboot(parserCmdInfo_t* pParserCmdInfo);
void Parser_SystemGetHwEui(parserCmdInfo_t* pParserCmdInfo);
#ifdef CONF_PMM_ENABLE
void Parser_SystemSleep(parserCmdInfo_t* pParserCmdInfo);
#endif /* #ifdef CONF_PMM_ENABLE */
void configure_extint(void);
void configure_eic_callback(void);
void Parser_SystemFactReset(parserCmdInfo_t* pParserCmdInfo);
void Parser_SystemGetCustomParam(parserCmdInfo_t* pParserCmdInfo);
void Parser_SystemSetCustomParam(parserCmdInfo_t* pParserCmdInfo);
#endif /* _PARSER_SYSTEM_H */