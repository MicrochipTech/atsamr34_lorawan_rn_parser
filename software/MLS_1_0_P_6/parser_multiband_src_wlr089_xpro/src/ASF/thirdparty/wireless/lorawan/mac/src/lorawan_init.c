/**
* \file  lorawan_init.c
*
* \brief LoRaWAN Initialization file
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
 

/****************************** INCLUDES **************************************/

#include "sw_timer.h"
#include "lorawan_defs.h"
#include "lorawan_init.h"

/*
void DIO0_ISR_Lora_Init(void);
void DIO1_ISR_Lora_Init(void);
void DI02_ISR_Lora_Init(void);
void DIO4_ISR_Lora_Init(void);
void DIO5_ISR_Lora_Init(void);

void DIO0_ISR_Lora_Init(void)
{
    INT1_SetInterruptHandler(DIO0_ISR_Lora);
}

void DIO1_ISR_Lora_Init(void)
{
    INT2_SetInterruptHandler(DIO1_ISR_Lora);
}

void DIO2_ISR_Lora_Init(void)
{
    IOCB4_SetInterruptHandler(DIO2_ISR_Lora);
}

void DIO3_ISR_Lora_Init(void)
{
}

void DIO4_ISR_Lora_Init(void)
{
    IOCB5_SetInterruptHandler(DIO4_ISR_Lora);
}

void DIO5_ISR_Lora_Init(void)
{
    INT0_SetInterruptHandler(DIO5_ISR_Lora);
}*/

void LORAWAN_PlatformInit(void)
{
/*
    TMR_ISR_Lora_Init();
    DIO0_ISR_Lora_Init();
    DIO1_ISR_Lora_Init();
    DIO2_ISR_Lora_Init();
    DIO3_ISR_Lora_Init();
    DIO4_ISR_Lora_Init();
    DIO5_ISR_Lora_Init();
    SystemTimerInit();*/
}
/**
 End of File
*/