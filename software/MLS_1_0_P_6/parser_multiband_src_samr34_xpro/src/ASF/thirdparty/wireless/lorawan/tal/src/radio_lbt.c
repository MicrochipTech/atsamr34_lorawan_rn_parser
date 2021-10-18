/**
* \file  radio_lbt.c
*
* \brief This is the Radio LBT header file which contains listen before talk specific
*		Radio functions declarations and defines for SX1276
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


/*#ifdef LBT*/

/************************************************************************/
/*  Includes                                                            */
/************************************************************************/
#include "radio_lbt.h"
#include "radio_task_manager.h"
#include "radio_driver_SX1276.h"
#include "radio_registers_SX1276.h"
#include "radio_interface.h"
#include "radio_transaction.h"
#include "sw_timer.h"
#include "radio_driver_hal.h"
#include "radio_get_set.h"

/************************************************************************/
/*  Defines                                                             */
/************************************************************************/

/************************************************************************/
/*  Global variables                                                    */
/************************************************************************/

/************************************************************************/
/*  Extern variables                                                    */
/************************************************************************/
extern volatile RadioEvents_t       radioEvents;
extern volatile RadioCallbackMask_t radioCallbackMask;

/************************************************************************/
/*  Static functions                                                    */
/************************************************************************/

/************************************************************************/
/*  Function Definitions                                                */
/************************************************************************/
/*********************************************************************//**
\brief	This function sets up the radio for scan.
 
\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t RADIO_ScanHandler(void)
{

    Radio_SetClockInput();
	// Turn on the RF switch.
	Radio_EnableRfControl(RADIO_RFCTRL_RX);
	uint32_t local;
	//Radio_WriteConfiguration(4);
	Radio_WriteFrequency(radioConfiguration.frequency);
	radioConfiguration.lbt.lbtChannelRSSI = 0;
	
	Radio_WriteMode(MODE_SLEEP, MODULATION_FSK, 1);
	
	// Mask all interrupts
#ifdef ENABLE_DIO0
	HAL_DisbleDIO0Interrupt();
#endif /* ENABLE_DIO0 */
#ifdef ENABLE_DIO1
	HAL_DisbleDIO1Interrupt();
#endif /* ENABLE_DIO1 */
#ifdef ENABLE_DIO2
	HAL_DisbleDIO2Interrupt();
#endif /* ENABLE_DIO2 */
#ifdef ENABLE_DIO3
	HAL_DisbleDIO3Interrupt();
#endif /* ENABLE_DIO3 */
#ifdef ENABLE_DIO4
	HAL_DisbleDIO4Interrupt();
#endif /* ENABLE_DIO4 */
#ifdef ENABLE_DIO5
	HAL_DisbleDIO5Interrupt();
#endif /* ENABLE_DIO5 */
	/* Write Bandwidth as 200KHz to read RSSI throughout channel bandwidth */
	RADIO_RegisterWrite(REG_FSK_RXBW, FSKBW_200_0KHZ);
	
	Radio_WriteMode(MODE_RXCONT, MODULATION_FSK, 0);

	// Time needed for Radio to be ready to take RSSI measurements
	local = MS_TO_US(9U);
	SwTimerStart(radioConfiguration.lbt.lbtScanTimerId, local, SW_TIMEOUT_RELATIVE, (void *)Radio_LBTScanTimeout, NULL);
	
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function is triggered by the scan done event and it sets up
		the radio for scanning.

\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t radioScanDoneHandler(void)
{
	radioEvents.LbtScanDoneEvent = 0;
	RadioCallbackParam_t RadioCallbackParam;

	Radio_WriteMode(MODE_SLEEP, MODULATION_FSK, 0);
	
#ifdef ENABLE_DIO0
	HAL_EnableDIO0Interrupt();
#endif /* ENABLE_DIO0 */
#ifdef ENABLE_DIO1
	HAL_EnableDIO1Interrupt();
#endif /* ENABLE_DIO1 */
#ifdef ENABLE_DIO2
	HAL_EnableDIO2Interrupt();
#endif /* ENABLE_DIO2 */
#ifdef ENABLE_DIO3
	HAL_EnableDIO3Interrupt();
#endif /* ENABLE_DIO3 */
#ifdef ENABLE_DIO4
	HAL_EnableDIO4Interrupt();
#endif /* ENABLE_DIO4 */
#ifdef ENABLE_DIO5
	HAL_EnableDIO5Interrupt();
#endif /* ENABLE_DIO5 */

	// Turning off the RF switch now.
	Radio_DisableRfControl(RADIO_RFCTRL_RX);
	//Powering Off the Oscillator after putting TRX to sleep
	Radio_ResetClockInput();
	
	if (radioConfiguration.lbt.lbtChannelRSSI < radioConfiguration.lbt.params.lbtThreshold)
	{
		RadioSetState(RADIO_STATE_TX);
		RADIO_TxHandler();
	}
	else
	{
		RadioCallbackParam.status = ERR_CHANNEL_BUSY;
		RadioSetState(RADIO_STATE_IDLE);
		if (1 == radioCallbackMask.BitMask.radioTxDoneCallback)
		{
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_TX_DONE_CALLBACK, (void *) &(RadioCallbackParam));
			}
		}
	}
	
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function is the callback function for LBT scan timer 
        timeout.

\param time - not used.
\return     - none
*************************************************************************/
void Radio_LBTScanTimeout(uint8_t time)
{	
	(void)time;
	int16_t tempRssi;
	radioConfiguration.lbt.lbtRssiSamplesCount++;
	if (radioConfiguration.lbt.lbtRssiSamplesCount <= radioConfiguration.lbt.params.lbtNumOfSamples)
	{
		Radio_ReadFSKRssi(&tempRssi);
		radioConfiguration.lbt.lbtRssiSamples += (tempRssi * radioConfiguration.lbt.lbtRssiSamplesCount);
		SwTimerStart(radioConfiguration.lbt.lbtScanTimerId, radioConfiguration.lbt.lbtScanTimeout, SW_TIMEOUT_RELATIVE, (void *)Radio_LBTScanTimeout, NULL);
	}
	else
	{
		radioEvents.LbtScanDoneEvent = 1;
		// Here we are dividing the weighted radioConfiguration.lbt.lbtRssiSamples by the sum of their weights to calculate the weighted average.
		// Since the weights increment linearly like 1, 2, 3 and so on we can use the sum of n natural numbers formula n=(n*(n+1))/2 to calculate the total weights added.
		// Here n is radioConfiguration.lbt.lbtRssiSamplesCount and since it is pre incremented in line #264 we use n=(n*(n-1))/2.
		// So radioConfiguration.lbt.lbtRssiSamples/(((radioConfiguration.lbt.lbtRssiSamplesCount*(radioConfiguration.lbt.lbtRssiSamplesCount-1)))/2) is
		// (radioConfiguration.lbt.lbtRssiSamples*2)/((radioConfiguration.lbt.lbtRssiSamplesCount*(radioConfiguration.lbt.lbtRssiSamplesCount-1)))
		// optimizing to (radioConfiguration.lbt.lbtRssiSamples<<1)/((radioConfiguration.lbt.lbtRssiSamplesCount*(radioConfiguration.lbt.lbtRssiSamplesCount-1)))
		radioConfiguration.lbt.lbtChannelRSSI = (radioConfiguration.lbt.lbtRssiSamples<<1)/((uint16_t)(radioConfiguration.lbt.lbtRssiSamplesCount*(radioConfiguration.lbt.lbtRssiSamplesCount-1)));
		radioConfiguration.lbt.lbtRssiSamplesCount = 0;
		radioConfiguration.lbt.lbtRssiSamples = 0;
		radioPostTask(RADIO_TX_DONE_TASK_ID);
	}
}

/*#endif LBT*/

// EOL