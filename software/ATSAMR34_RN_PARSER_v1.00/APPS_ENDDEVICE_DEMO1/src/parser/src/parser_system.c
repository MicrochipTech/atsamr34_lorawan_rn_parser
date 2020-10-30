/**
* \file  parser_system.c
*
* \brief This file contains all the commands used for testing the system peripherals
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
 
#include <stdlib.h>
#include <string.h>
#include "parser_system.h"
#include "parser.h"
#include "parser_utils.h"
#include "parser_tsp.h"
#include "system_low_power.h"
#include "sys.h"
#include "sw_timer.h"
#ifdef CONF_PMM_ENABLE
#include "pmm.h"
#include "conf_pmm.h"
#endif
#if (ENABLE_PDS == 1)
#include "pds_interface.h"
#endif
#include "lorawan.h"
#include "conf_sio2host.h"
#include "sio2host.h"
#include "radio_driver_hal.h"

#define STANDBY_STR_IDX        0U
#define BACKUP_STR_IDX         1U
#define OFF_STR_IDX            2U

extern uint32_t pdsAppCustomParameter;

static bool SleepEnabled = false;
#ifdef CONF_PMM_ENABLE
static const char* gapParserSysStatus[] =
{
   "ok",
   "invalid_param",
   "err"
};

static const char* gapParseSleepMode[] =
{
	"standby",
	"backup",
};
static void parserSleepCallback(uint32_t sleptDuration);
static void app_resources_uninit(void);

bool deviceResetsForWakeup = false;

#endif /* #ifdef CONF_PMM_ENABLE */

static void extint_callback(void);

void Parser_SystemGetHwEui(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t aDevEui[8];

    System_GetExternalEui(aDevEui);
    Parser_IntArrayToHexAscii(8, aDevEui, aParserData);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_SystemGetVer(parserCmdInfo_t* pParserCmdInfo)
{
	Parser_GetSwVersion(aParserData);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_SystemGetCustomParam(parserCmdInfo_t* pParserCmdInfo)
{
	PDS_RESTORE(PDS_APP_CUSTOMPARAMETER) ;
	utoa(pdsAppCustomParameter, aParserData,  10U);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_SystemSetCustomParam(parserCmdInfo_t* pParserCmdInfo)
{
	StackRetStatus_t status = INVALID_PARAM_IDX ;
	pdsAppCustomParameter = (uint32_t)strtoul(pParserCmdInfo->pParam1, NULL, 10);
	if (Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 10, UINT32_MAX))
	{
		status = OK_STATUS_IDX ;
		printf("\r\n%ld\r\n", pdsAppCustomParameter) ;
		PDS_STORE(PDS_APP_CUSTOMPARAMETER) ;
	}
	pParserCmdInfo->pReplyCmd = (char*)gapParserSysStatus[status] ;
}

void Parser_SystemReboot(parserCmdInfo_t* pParserCmdInfo)
{
	// Go for reboot, no reply necessary
	NVIC_SystemReset();
}

void Parser_SystemFactReset(parserCmdInfo_t* pParserCmdInfo)
{
	// Call PDS Delete All API to clear NVM Memory.
#if (ENABLE_PDS == 1)		
	PDS_DeleteAll();
#endif	
	// Go for reboot, no reply necessary
	NVIC_SystemReset();
}

#ifdef CONF_PMM_ENABLE
void Parser_SystemSleep (parserCmdInfo_t* pParserCmdInfo)
{
    /** Refer gapParseSleepMode[] array indices
    *         [0] --> "standby"
    *         [1] --> "backup"
    */
    uint8_t sleepModeValue = 0xFF;
    PMM_SleepReq_t sleepRequest = {
        .sleep_mode = 0xFF,
        .sleepTimeMs = 0,
        .pmmWakeupCallback = NULL
    };
    uint32_t sleepDuration = strtoul(pParserCmdInfo->pParam2, NULL, 10);

    /* Parser parameter validation */
	for(uint8_t iCount = 0; iCount < sizeof(gapParseSleepMode)/sizeof(gapParseSleepMode[0]); iCount++)
	{
		if(0 == stricmp(pParserCmdInfo->pParam1, gapParseSleepMode[iCount]))
		{
			sleepModeValue = iCount;
			break;
		}
	}

    /* Sleep parameters validation */
    if ( (1 /* invalid range */ < sleepModeValue) || \
         (PMM_SLEEPTIME_MIN_MS > sleepDuration)   || \
         (PMM_SLEEPTIME_MAX_MS < sleepDuration)
       )
    {
        pParserCmdInfo->pReplyCmd = (char *) gapParserSysStatus[INVALID_PARAM_IDX];
        return;
    }
    else
    {
        sleepRequest.sleepTimeMs = sleepDuration;
        sleepRequest.pmmWakeupCallback = parserSleepCallback;
        sleepRequest.sleep_mode = ( 0 /* standby */ == sleepModeValue ) ? SLEEP_MODE_STANDBY : SLEEP_MODE_BACKUP;
    }

    /* Sleep invocation */
    SleepEnabled = true;
    if ( LORAWAN_ReadyToSleep( deviceResetsForWakeup ) )
    {
        app_resources_uninit();
        if ( PMM_SLEEP_REQ_DENIED == PMM_Sleep( &sleepRequest ) )
        {
            HAL_Radio_resources_init();
            sio2host_init();
            pParserCmdInfo->pReplyCmd = (char *) gapParserSysStatus[ERR_STATUS_IDX];
        }
    }
}
#endif /* #ifdef CONF_PMM_ENABLE */


/*********************************************************************//**
\brief	Configures the External Interrupt Controller to detect changes in the board
        button state.
*************************************************************************/
void configure_extint(void)
{
	struct extint_chan_conf eint_chan_conf;
	extint_chan_get_config_defaults(&eint_chan_conf);

	eint_chan_conf.gpio_pin           = BUTTON_0_EIC_PIN;
	eint_chan_conf.gpio_pin_mux       = BUTTON_0_EIC_MUX;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_FALLING;
	eint_chan_conf.filter_input_signal = true;
	extint_chan_set_config(BUTTON_0_EIC_LINE, &eint_chan_conf);
}


/*********************************************************************//**
\brief	Configures and registers the External Interrupt callback function with the
        driver.
*************************************************************************/
void configure_eic_callback(void)
{
	extint_register_callback(
		extint_callback,
		BUTTON_0_EIC_LINE,
		EXTINT_CALLBACK_TYPE_DETECT
	);

	extint_chan_enable_callback(BUTTON_0_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT);
}

/*********************************************************************//**
\brief	Callback function for the EXTINT driver, called when an external interrupt
        detection occurs.
*************************************************************************/
static void extint_callback(void)
{
#ifdef CONF_PMM_ENABLE
	PMM_Wakeup();
#endif
	if(SleepEnabled)
	{
	    HAL_Radio_resources_init();
		sio2host_init();
		printf("\nExiting Sleep\n\r");
		SleepEnabled = false;
	}
}

#ifdef CONF_PMM_ENABLE
/*********************************************************************//**
\brief	Callback function of parser to power manager.
\param[in]	sleptDuration - duration for which sleep is done
*************************************************************************/
static void parserSleepCallback(uint32_t sleptDuration)
{
	HAL_Radio_resources_init();
	sio2host_init();
	printf("\nsleep_ok %ld ms\n\r", sleptDuration);
}

static void app_resources_uninit(void)
{
	/* Disable USART TX and RX Pins */
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.powersave  = true;
#ifdef HOST_SERCOM_PAD0_PIN
	port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
#endif
#ifdef HOST_SERCOM_PAD1_PIN
	port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
#endif
	/* Disable UART module */
	sio2host_deinit();
	/* Disable Transceiver SPI Module */
	HAL_RadioDeInit();
}

#endif /* #ifdef CONF_PMM_ENABLE */ 

/*EOF*/
