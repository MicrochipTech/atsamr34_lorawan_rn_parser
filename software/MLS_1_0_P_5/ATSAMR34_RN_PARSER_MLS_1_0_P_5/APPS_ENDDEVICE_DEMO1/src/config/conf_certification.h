/**
* \file  conf_certification.h
*
* \brief Application Configuration
*		
*
* Copyright (c) 2018-2020 Microchip Technology Inc. and its subsidiaries. 
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
 

#ifndef ENDDEVICE_CERT_CONF_H_
#define ENDDEVICE_CERT_CONF_H_

/****************************** INCLUDES **************************************/

/****************************** MACROS **************************************/

/**** EndDevice Certification Macros ****/
#if (CERT_APP == 1)

#define CERT_APP_FPORT							1
#define TEST_PORT_NB       						224
#define DEACTIVATE_MODE    						0
#define ACTIVATE_MODE      						1
#define CNF_MODE           						2
#define UNCNF_MODE         						3
#define CRYPTO_MODE        						4
#define LINK_CHECK_MODE    						5
#define OTAA_TRIGGER_MODE  						6
#define LINK_CHECK_ANS_CID 						2
#define UPLINK_NO_RESPONSE_NB 					128
#define ON            							1
#define OFF           							0
#define NOT_JOINED    							0
#define JOINED        							1

/*Define the Sub band of Channels to be enabled by default for the application*/
#define SUBBAND 1
#if ((SUBBAND < 1 ) || (SUBBAND > 8 ) )
#error " Invalid Value of Subband"
#endif


/* Activation method constants */	
#define OVER_THE_AIR_ACTIVATION           			LORAWAN_OTAA
#define ACTIVATION_BY_PERSONALIZATION     			LORAWAN_ABP

/* Message Type constants */			
#define UNCONFIRMED                       			LORAWAN_UNCNF
#define CONFIRMED                         			LORAWAN_CNF

/* Enable one of the activation methods */
#define CERT_APP_ACTIVATION_TYPE				OVER_THE_AIR_ACTIVATION 
//#define CERT_APP_ACTIVATION_TYPE				ACTIVATION_BY_PERSONALIZATION

/* Certification is available only for Class A device */
#define CERT_APP_ENDDEVICE_CLASS				CLASS_A

/* Select the Type of Transmission - Confirmed(CNF) / Unconfirmed(UNCNF) */
#define CERT_APP_TRANSMISSION_TYPE				UNCONFIRMED
//#define CERT_APP_TRANSMISSION_TYPE			CONFIRMED

#define CERT_APP_TIMEOUT						5000 //ms

/* FPORT Value (1-255) */
#define CERT_APP_FPORT                           1

/* ABP Join Parameters */
#define CERT_DEVICE_ADDRESS						0x1BF05EB1
#define CERT_APPLICATION_SESSION_KEY			{0x56, 0x54, 0x67, 0x75, 0xbd, 0xb6, 0xb4, 0xb4, 0xc0, 0xae, 0x47, 0x4a, 0x69, 0xc0, 0x4c, 0x41}
#define CERT_NETWORK_SESSION_KEY				{0x70, 0x19, 0xb0, 0x63, 0x99, 0x17, 0x2c, 0xef, 0xb9, 0x28, 0x3a, 0x08, 0x61, 0x61, 0xbc, 0x81}

/* OTAA Join Parameters */
#define CERT_DEVICE_EUI							{0x51, 0x57, 0x35, 0x42, 0x3a, 0xee, 0xa1, 0xb8}
#define CERT_APPLICATION_EUI					{0xfb, 0x14, 0xdd, 0xe4, 0xba, 0x30, 0xa3, 0x04}
#define CERT_APPLICATION_KEY					{0x1d, 0x53, 0x60, 0xc7, 0x47, 0xbb, 0xde, 0xfc, 0x84, 0xbc, 0x4c, 0x05, 0x11, 0xf4, 0xfb, 0x3f}

#endif /* CERT_APP == 1 */

#endif /* ENDDEVICE_CERT_CONF_H_ */
