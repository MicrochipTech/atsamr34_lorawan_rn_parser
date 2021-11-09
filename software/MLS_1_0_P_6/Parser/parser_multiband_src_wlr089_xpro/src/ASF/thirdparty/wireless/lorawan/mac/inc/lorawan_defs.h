/**
* \file  lorawan_defs.h
*
* \brief This is the LoRaWAN utility definitions file 
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
 

#ifndef MCC_LORA_DEFS_H
#define	MCC_LORA_DEFS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define DEF_CNF_UL_REPT_CNT							(0)

#define DEF_UNCNF_UL_REPT_CNT						(0)

#define MAC_CONFIRMABLE_UPLINK_REPITITIONS_MAX      (7)

#define MAC_UNCONFIRMABLE_UPLINK_REPITITIONS_MAX    (0)

#define DEF_MACSTATUS								(0)

#define MAC_LINK_CHECK_GATEWAY_COUNT				(0)

#define MAC_LINK_CHECK_MARGIN						(255)

#define MAC_LORA_MODULATION_SYNCWORD				(0x34)

#define MAC_AGGREGATED_DUTYCYCLE					(0)

#define MAC_DEVNONCE								(0)

#define MAC_JOINNONCE                               (0xFFFFFFFF)

#define TRANSMISSION_ERROR_TIMEOUT					(2000UL)

#define CLASS_C_RX2_WINDOW_SIZE						(0)

#define MAX_FCNT_PDS_UPDATE_VALUE               (1) // Keep this as power of 2. Easy for bit manipulation.

#define LORAWAN_MCAST_GROUP_COUNT_SUPPORTED         4

#ifdef	__cplusplus
}
#endif

#endif	/* MCC_LORA_DEFS_H */

/**
 End of File
*/