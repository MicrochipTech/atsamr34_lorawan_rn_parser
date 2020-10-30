/**
* \file  main.c
*
* \brief LORAWAN Parser Application
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
 
/****************************** INCLUDES **************************************/
#include "system_assert.h"
#include "sw_timer.h"
#include "system_low_power.h"
#include "radio_interface.h"
#include "radio_driver_hal.h"
#include "lorawan.h"
#include "sys.h"
#include "sio2host.h"
#include "parser.h"
#include "parser_tsp.h"
#include "parser_system.h"
#include "system_init.h"
#include "aes_engine.h"
#ifdef CONF_PMM_ENABLE
 #include "sleep_timer.h"
#endif /* CONF_PMM_ENABLE */
#if (ENABLE_PDS == 1)	
  #include "pds_interface.h"
#endif
#include "sal.h"

/************************** macro definition ***********************************/

/************************** Global variables ***********************************/
uint8_t devEui[8] = {0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd};

/****************************** PROTOTYPES *************************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void);
#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif

/****************************** FUNCTIONS *************************************/
static void print_reset_causes(void)
{
	
	enum system_reset_cause rcause = system_get_reset_cause();
	printf("\r\nLast reset cause: ");
	if(rcause & (1 << 6)) {
		printf("System Reset Request\r\n");
	}
	if(rcause & (1 << 5)) {
		printf("Watchdog Reset\r\n");
	}
	if(rcause & (1 << 4)) {
		printf("External Reset\r\n");
	}
	if(rcause & (1 << 2)) {
		printf("Brown Out 33 Detector Reset\r\n");
	}
	if(rcause & (1 << 1)) {
		printf("Brown Out 12 Detector Reset\r\n");
	}
	if(rcause & (1 << 0)) {
		printf("Power-On Reset\r\n");
	}
}

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code)
{
	printf("\r\n%04x\r\n", code);
	(void)level;
}
#endif /* #if (_DEBUG_ == 1) */

/**
 * \mainpage
 * \section preface Preface
 * This is the reference manual for the LORAWAN Parser Application of EU Band
 */

void print_array (uint8_t *array, uint8_t length)
{
	printf("0x");
	for (uint8_t i =0; i < length; i++)
	{
		printf("%02x", *array);
		array++;
	}
	printf("\n\r");
}

int main(void)
{
	system_init();
	delay_init();
	board_init();
	INTERRUPT_GlobalInterruptEnable();
	sio2host_init();
	print_reset_causes();
	
#if (_DEBUG_ == 1)
	SYSTEM_AssertSubscribe(assertHandler);
#endif

	/* Configure board button as external interrupt pin */
	configure_extint();	
	/* Register External Interrupt callback */
	configure_eic_callback();
	printf("LoRaWAN Stack UP\r\n");
	HAL_RadioInit();	
	AESInit();
#ifdef CRYPTO_DEV_ENABLED
	SAL_Init();	
#endif
	SystemTimerInit();
#ifdef CONF_PMM_ENABLE
	SleepTimerInit();
#endif /* CONF_PMM_ENABLE */


#if (ENABLE_PDS == 1)	
 	PDS_Init();
#endif	
	Stack_Init();
	Parser_Init();
    Parser_SetConfiguredJoinParameters(0x01);
    Parser_GetSwVersion(aParserData);
    Parser_TxAddReply((char *)aParserData, (uint16_t)strlen((char *)aParserData));

    while (1)
    {
		parser_serial_data_handler();
		SYSTEM_RunTasks();
    }
}

SYSTEM_TaskStatus_t APP_TaskHandler(void)
{
	Parser_Main();
	return SYSTEM_TASK_SUCCESS;
}

/**
 End of File
 */
