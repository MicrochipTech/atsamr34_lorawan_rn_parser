/**
* \file  conf_sal.h
*
* \brief Security Abstraction Layer Configuration Header
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

#ifndef CONF_SAL_H_INCLUDED
#define CONF_SAL_H_INCLUDED

#define APP_KEY_SLOT             0
#define APPS_KEY_SLOT            2
#define NWKS_KEY_SLOT            3
#define MCAST_APPS_KEY_SLOT		 11
#define MCAST_NWKS_KEY_SLOT      12
#define KEK_SLOT                 14
#define APP_EUI_SLOT			 9
#define DEV_EUI_SLOT			 10

#define APP_KEY_SLOT_BLOCK		 1 //Index where AppKey is stored in a slot, KeyLocation in bytes = SlotBlock * 16;

#define SERIAL_NUM_AS_DEV_EUI    0// Value is 1 if ECC608 Serial number is used as DEV_EUI otherwise DEV_EUI will be read from DEV_EUI_SLOT

#endif /* CONF_ATCAD_H_INCLUDED */
