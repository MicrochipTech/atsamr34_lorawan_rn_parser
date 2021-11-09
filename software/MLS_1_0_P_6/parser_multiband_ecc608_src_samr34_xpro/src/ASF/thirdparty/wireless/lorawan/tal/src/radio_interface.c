/**
* \file  radio_interface.c
*
* \brief This is the Radio Driver SX1276 source file which contains LoRa-specific
*        Radio Driver functions declarations and defines for SX1276
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

/************************************************************************/
/*  Includes                                                            */
/************************************************************************/
#include "radio_interface.h"
#include "radio_registers_SX1276.h"
#include "radio_driver_hal.h"
#include <delay.h>
/************************************************************************/
/*  Defines                                                             */
/************************************************************************/
#define SHIFT0                                  (0)
#define SHIFT1                                  (1)
#define SHIFT2                                  (2)
#define SHIFT3                                  (3)
#define SHIFT4                                  (4)
#define SHIFT5                                  (5)
#define SHIFT6                                  (6)
#define SHIFT7                                  (7)
#define SHIFT8                                  (8)

/************************************************************************/
/*  Global variables                                                    */
/************************************************************************/

/************************************************************************/
/*  Static functions                                                    */
/************************************************************************/

/*********************************************************************//**
\brief	This function handles the rogue radio interrupts.

\param modulation	- modulation when this function is called.
\return				- none.
*************************************************************************/
static void RADIO_getMappingAndOpmode(uint8_t *dioMapping, uint8_t *opMode, uint8_t mask, uint8_t shift);

/************************************************************************/
/* Implementations                                                      */
/************************************************************************/

/*********************************************************************//**
\brief	This function handles the rogue radio interrupts.

\param modulation	- modulation when this function is called.
\return				- none.
*************************************************************************/
void RADIO_UnhandledInterrupt(RadioModulation_t modulation)
{
    // Clear all IRQ flags to recover gracefully.
    // Since we already know the radio modulation settings we can go access
    // those registers directly.
    if (MODULATION_LORA == modulation)
    {
        RADIO_RegisterWrite(REG_LORA_IRQFLAGS, 0xFF);
    }
    else
    {
        // Although just some of the bits can be cleared, try to clear
        // everything
        RADIO_RegisterWrite(REG_FSK_IRQFLAGS1, 0xFF);
        RADIO_RegisterWrite(REG_FSK_IRQFLAGS2, 0xFF);
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO0
		occurs.
*************************************************************************/
void RADIO_DIO0(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO0 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode, 0xC0, SHIFT6);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            case 0x00:
                RADIO_RxDone();
                break;
            case 0x01:
                RADIO_TxDone();
                break;
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    } else
    {
        // FSK modulation
        switch (dioMapping)
        {
            case 0x00:
                // Check if the radio state is Tx or Rx
                opMode &= RADIO_OPMODE_MASK;
                if (MODE_TX == opMode)
                {
                    // PacketSent
                    RADIO_FSKPacketSent();
                }
                else if (MODE_RXCONT == opMode)
                {
                    // PayloadReady
                    RADIO_FSKPayloadReady();
                }
                else
                {
                    RADIO_UnhandledInterrupt(MODULATION_FSK);
                }
                break;
            default:
                RADIO_UnhandledInterrupt(MODULATION_FSK);
                break;
        }
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO1
		occurs.
*************************************************************************/
void RADIO_DIO1(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO1 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode, 0x30, SHIFT4);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            case 0x00:
                RADIO_RxTimeout();
                break;
            case 0x01:

                RADIO_FHSSChangeChannel();

                break;
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    }
    else
    {
        // FSK modulation
		opMode &= RADIO_OPMODE_MASK;
        switch (dioMapping)
        {
            case 0x00:
				 if (MODE_RXCONT == opMode)
				 {
					RADIO_FSKFifoLevel();
				 }
                break;
			case 0x01:
				if (MODE_TX == opMode)
				{
                    delay_us(10); /* Without this delay, TX always times out */
					RADIO_FSKFifoEmpty();
				}
				break;
            default:
                // RADIO_UnhandledInterrupt(MODULATION_FSK);
                break;
        }
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO2
		occurs.
*************************************************************************/
void RADIO_DIO2(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO2 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode, 0x0C, SHIFT2);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            // Intentional fall-through
            case 0x00:
            case 0x01:
            case 0x02:

				RADIO_FHSSChangeChannel();

                break;
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    }
    else
    {
        // FSK modulation
        opMode &= RADIO_OPMODE_MASK;
        switch (dioMapping)
        {
	        case 0x03:
	        if (MODE_RXCONT == opMode)
	        {
		        RADIO_FSKSyncAddr();
	        }
	        break;
	        default:
	        // RADIO_UnhandledInterrupt(MODULATION_FSK);
	        break;
        }
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO3
		occurs.
*************************************************************************/
void RADIO_DIO3(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO3 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode, 0x03, 0);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    }
    else
    {
        // FSK modulation
        // RADIO_UnhandledInterrupt(MODULATION_FSK);
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO4
		occurs.
*************************************************************************/
void RADIO_DIO4(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO4 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode,  0xC0, SHIFT6);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    }
    else
    {
        // FSK modulation
        // RADIO_UnhandledInterrupt(MODULATION_FSK);
    }
}

/*********************************************************************//**
\brief	This function is called when a hardware radio interrupt DIO5
		occurs.
*************************************************************************/
void RADIO_DIO5(void)
{

    uint8_t dioMapping;
    uint8_t opMode;

    // Check radio configuration (modulation and DIO5 settings).
    RADIO_getMappingAndOpmode(&dioMapping, &opMode, 0x30, SHIFT4);

    if ((opMode & 0x80) != 0)
    {
        // LoRa modulation
        switch (dioMapping)
        {
            default:
                RADIO_UnhandledInterrupt(MODULATION_LORA);
                break;
        }
    }
    else
    {
        // FSK modulation
        // RADIO_UnhandledInterrupt(MODULATION_FSK);
    }
}

/*********************************************************************//**
\brief	This function handles the rogue radio interrupts.

\param modulation	- modulation when this function is called.
\return				- none.
*************************************************************************/
static void RADIO_getMappingAndOpmode(uint8_t *dioMapping, uint8_t *opMode, uint8_t mask, uint8_t shift)
{
    *dioMapping = (RADIO_RegisterRead(REG_DIOMAPPING1) & mask) >> shift;
    *opMode = RADIO_RegisterRead(REG_OPMODE);
}

/* eof radio_interface.c */
