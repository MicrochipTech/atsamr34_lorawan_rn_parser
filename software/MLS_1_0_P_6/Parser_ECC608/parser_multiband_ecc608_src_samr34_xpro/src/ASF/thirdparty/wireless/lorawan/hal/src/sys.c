/**
* \file  sys.c
*
* \brief System management file
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
 
#include "sys.h"
#include "atomic.h"
#include "delay.h"
#include "system_interrupt.h"

/** 
 * \brief Performs a blocking delay
 * \param[in] ms Delay time in milliseconds
 * \note : This function should allow interrupts to happen (unless it was called from
 * an interrupt itself) and keep its timing accurate. Ideally it should do the
 * waiting with the MCU in sleep.
 * Find out how long it takes the MCU to go to and wake up from sleep to see if
 * it makes sense to go to sleep at all 
 */
void SystemBlockingWaitMs(uint32_t ms)
{
    delay_ms(ms);
}

#if (EDBG_EUI_READ == 1)
#include "edbg_eui.h"
#endif

void System_GetExternalEui(uint8_t *id)
{
#if (BOARD == SAMR34_XPLAINED_PRO && defined(__SAMR34J18B__) && EDBG_EUI_READ == 1)

	for (uint8_t i = 0; i < 8; i++)
	{
		*id = EDBGDevEUI[i] ;
		id++ ;
	}
#elif (defined(__WLR089U0__) && MODULE_EUI_READ == 1)

	#define NVM_UID_ADDRESS   ((volatile uint16_t *)(0x0080400AU))
	uint8_t i = 0, j = 0 ;
	uint8_t moduleDevEUI[8] ;
	for (i = 0; i < 8; i += 2, j++)
	{
		moduleDevEUI[i] = (NVM_UID_ADDRESS[j] & 0xFF) ;
		moduleDevEUI[i + 1] = (NVM_UID_ADDRESS[j] >> 8) ;
	}
	for (i= 0; i < 8; i++)
	{
		*id = moduleDevEUI[i] ;
		id++ ;
	}
#endif
}

void system_enter_critical_section(void)
{
	system_interrupt_enter_critical_section();
}

void system_leave_critical_section(void)
{
	system_interrupt_leave_critical_section();
}
