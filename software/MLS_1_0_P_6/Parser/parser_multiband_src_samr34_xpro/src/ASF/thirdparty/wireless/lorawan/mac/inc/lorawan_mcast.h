/**
* \file  lorawan_mcast.h
*
* \brief LoRaWAN header file for multicast data & control definition(classB & classC)
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
 
#ifndef _LORAWAN_MCAST_H_
#define _LORAWAN_MCAST_H_

/***************************** TYPEDEFS ***************************************/

/*************************** FUNCTIONS PROTOTYPE ******************************/

/*********************************************************************//**
\brief	Multicast - initialization of variables and states

\return					- none.
*************************************************************************/
void LorawanMcastInit(void);

/*********************************************************************//**
\brief	Check if the incoming packet is a multicast group the device
        supports
\param[in]  hdr - mac header of the received frame
\param[in]  mType - message type of the received frame
\param[in]  fPort - frame port of the received frame
\return	    LORAWAN_SUCCESS, if successfully validated
            LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastValidateHdr(Hdr_t *hdr, uint8_t mType, uint8_t fPort,uint8_t* groupId);

/*********************************************************************//**
\brief	Decrypt the packet and provide callback to application
\param[in,out]  buffer  - placeholder for input and output data
\param[in]      bufferLength - length of the received packet
\param[in]      hdr - mac header of the received packet
\return	LORAWAN_SUCCESS, if successfully processed
        LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastProcessPkt(uint8_t* buffer, uint8_t bufferLength, Hdr_t *hdr,uint8_t groupId);

/*********************************************************************//**
\brief	Multicast enable/disable configuration function
\param[in]  enable - notifies whether to enable or disable multicast
\return	LORAWAN_SUCCESS, if successfully enabled
        LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastEnable(bool enable, uint8_t groupid);

StackRetStatus_t LorawanAddMcastAddr(uint32_t mcast_devaddr, uint8_t groupId);

StackRetStatus_t LorawanAddMcastAppskey(uint8_t * appSkey, uint8_t groupId);

StackRetStatus_t LorawanAddMcastNwkskey(uint8_t * nwkSkey, uint8_t groupId);

StackRetStatus_t LorawanAddMcastFcntMin(uint32_t cnt, uint8_t groupId);
StackRetStatus_t LorawanAddMcastFcntMax(uint32_t cnt, uint8_t groupId);

StackRetStatus_t LorawanAddMcastDlFrequency(uint32_t dlFreq, uint8_t groupId);
StackRetStatus_t LorawanAddMcastDatarate(uint8_t dr, uint8_t groupId);
StackRetStatus_t LorawanAddMcastPeriodicity(uint8_t periodicity, uint8_t groupId);
#endif // _LORAWAN_MCAST_H_

//eof lorawan_mcast.h
