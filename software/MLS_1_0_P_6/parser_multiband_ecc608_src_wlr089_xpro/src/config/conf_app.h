/**
* \file  conf_app.h
*
* \brief LORAWAN Demo Application include file
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


#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

/****************************** INCLUDES **************************************/

/****************************** MACROS **************************************/

/*Supported Bands*/
#define  EU868  	0    
#define  EU433  	1  
#define  NA915  	2
#define  AU915  	3
#define  KR920  	4
#define	 JPN923		5
#define  BRN923 	6	
#define	 CMB923		7
#define	 INS923		8
#define	 LAOS923	9
#define	 NZ923		10
#define	 SP923		11
#define	 TWN923		12	
#define	 THAI923	13
#define	 VTM923		14
#define  IND865     15

/* Number of software timers */
#define TOTAL_NUMBER_OF_TIMERS        (25u)

#define DEFAULT_ISM_BAND	EU868
//#define DEFAULT_ISM_BAND	NA915
//#define DEFAULT_ISM_BAND	AU915
//#define DEFAULT_ISM_BAND	KR920
//#define DEFAULT_ISM_BAND	THAI923
//#define DEFAULT_ISM_BAND	JPN920
//#define DEFAULT_ISM_BAND	IND865

#endif /* APP_CONFIG_H_ */
