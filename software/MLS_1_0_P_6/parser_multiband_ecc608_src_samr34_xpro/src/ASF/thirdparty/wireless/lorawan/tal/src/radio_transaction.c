/**
* \file  demo_output.c
*
* \brief Demo output Implementation.
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


#include "radio_task_manager.h"
#include "radio_driver_SX1276.h"
#include "radio_registers_SX1276.h"
#include "radio_transaction.h"
#include "radio_get_set.h"
#include "radio_driver_hal.h"
#include "radio_lbt.h"
#include "sw_timer.h"
#include "sys.h"

/************************************************************************/
/*  Static variables                                                    */
/************************************************************************/
static uint8_t                      txBufferLen;
static uint8_t                      *transmitBufferPtr = NULL;
static uint64_t                     timeOnAir;
static uint16_t                     rxWindowSize;

static int16_t                      instRSSI;

/************************************************************************/
/*  Global variables                                                    */
/************************************************************************/
volatile RadioState_t               radioState;
volatile RadioCallbackMask_t radioCallbackMask;
volatile RadioEvents_t       radioEvents;
uint8_t radioBuffer[RADIO_BUFFER_SIZE];

/************************************************************************/
/* Static Fuctions                                                      */
/************************************************************************/
static void Radio_ReadPktRssi(void);
static bool Radio_IsChannelFree(void);
static void Radio_EnableInterruptLines(void);
static void Radio_DisableInterruptLines(void);

/************************************************************************/
/* Implementations                                                      */
/************************************************************************/

/*********************************************************************//**
\brief This function sets the radio state.

\param state - The state of the radio to be set to.
*************************************************************************/
void RadioSetState(RadioState_t state)
{
	radioState = state;
}

/*********************************************************************//**
\brief This function gets the radio state.

\return - The current state of the radio.
*************************************************************************/
RadioState_t RADIO_GetState(void)
{
	return radioState;
}

/*********************************************************************//**
\brief	The function enables the callbacks by setting masks.

Setting a bit to 1 means that the callback is enabled
Setting a bit to 0 means that the callback is disabled

\param bitmask - The bitmask for enabling callbacks.
*************************************************************************/
void RADIO_SetCallbackBitmask(uint8_t bitmask)
{
  radioCallbackMask.callbackMask |= bitmask;
}

/*********************************************************************//**
\brief	The function disables the callbacks by setting masks.

Setting a bit to 1 means that the callback is enabled
Setting a bit to 0 means that the callback is disabled

\param bitmask - The bitmask for disabling callbacks.
*************************************************************************/
void RADIO_ClearCallbackBitmask(uint8_t bitmask)
{
  radioCallbackMask.callbackMask &= ~bitmask;
}

/*********************************************************************//**
\brief	The function enables or disables the callbacks by setting masks.

Setting a bit to 1 means that the callback is enabled
Setting a bit to 0 means that the callback is disabled

\return	- Returns the bit mask that is stored.
*************************************************************************/
uint8_t RADIO_GetCallbackBitmask(void)
{
  return radioCallbackMask.callbackMask;
}

/*********************************************************************//**
\brief	The function transmits a CW and is supported only in LORA mode.

\return	- returns the status of the operation
*************************************************************************/
RadioError_t RADIO_TransmitCW(void)
{
    if (RADIO_STATE_IDLE == RADIO_GetState())
    {
		// Turn on the RF switch.
		Radio_EnableRfControl(RADIO_RFCTRL_TX);
		
		//Power On the Oscillator before putting the radio to transmit state
		Radio_SetClockInput();
        radioConfiguration.modulation = MODULATION_LORA;

        // Since we're interested in a transmission, rxWindowSize is irrelevant.
        // Setting it to 4 is a valid option.
        Radio_WriteConfiguration(4);

        // Do not set the RadioState to RADIO_STATE_TX as we are not transmitting
        // any data so mac can override this by Tx'ing or Rx'ing.
        RADIO_RegisterWrite(0x3D, 0xA1);
        RADIO_RegisterWrite(0x36, 0x01);
        RADIO_RegisterWrite(0x1E, 0x08);
        RADIO_RegisterWrite(0x01, 0x8B);
    }
    else
    {
        return ERR_RADIO_BUSY;
    }

    return ERR_NONE;
}

/*********************************************************************//**
\brief	The function stops the transmission of a CW and is supported
		only in LORA mode.

\return	- returns the status of the operation
*************************************************************************/
RadioError_t RADIO_StopCW(void)
{
    if (!(RADIO_STATE_IDLE == RADIO_GetState()))
    {
        return ERR_RADIO_BUSY;
    }

    Radio_WriteMode(MODE_STANDBY, radioConfiguration.modulation, 0);
    SystemBlockingWaitMs(100);
    Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
    SystemBlockingWaitMs(100);
	
	// Turning off the RF switch now.
	Radio_DisableRfControl(RADIO_RFCTRL_TX);

	//Powering Off the Oscillator after putting TRX to sleep
	Radio_ResetClockInput();	
	
    return ERR_NONE;
}

/*********************************************************************//**
\brief	The function initializes the default attributes depending on
		region chosen.
*************************************************************************/
void RADIO_InitDefaultAttributes(void)
{
    RadioSetState(RADIO_STATE_IDLE);
    radioConfiguration.frequency = RADIO_DEFAULT_FREQ;
    radioConfiguration.frequencyDeviation = 25000;
    radioConfiguration.bitRate = 50000;
    radioConfiguration.modulation = MODULATION_LORA;
    radioConfiguration.bandWidth = BW_125KHZ;
    radioConfiguration.outputPower = 1;
    radioConfiguration.errorCodingRate = CR_4_5;
    radioConfiguration.implicitHeaderMode = 0;
    radioConfiguration.preambleLen = RADIO_PHY_PREAMBLE_LENGTH;
    radioConfiguration.dataRate = SF_7;
    radioConfiguration.crcOn = 1;
    radioConfiguration.paBoost = 0;
    radioConfiguration.iqInverted = 0;
    radioConfiguration.syncWord[0] = 0xC1;
    radioConfiguration.syncWord[1] = 0x94;
    radioConfiguration.syncWord[2] = 0xC1;
    radioConfiguration.syncWordLen = 3;
    radioConfiguration.syncWordLoRa = 0x34;
    radioConfiguration.frequencyHopPeriod = 0;
    radioConfiguration.packetSNR = -128;
    radioConfiguration.watchdogTimerTimeout = RADIO_WATCHDOG_TIMEOUT;
    radioConfiguration.fskDataShaping = FSK_SHAPING_GAUSS_BT_1_0;
    radioConfiguration.rxBw = FSKBW_50_0KHZ;
    radioConfiguration.afcBw = FSKBW_83_3KHZ;
    radioConfiguration.dataBufferLen = 0;
    radioConfiguration.dataBuffer = &radioBuffer[16];
	radioConfiguration.lbt.lbtChannelRSSI = 0;
	radioConfiguration.lbt.lbtIrqFlagsBackup = 0;
	radioConfiguration.lbt.lbtRssiSamples = 0;
	radioConfiguration.lbt.lbtRssiSamplesCount = 0;
	radioConfiguration.lbt.lbtScanTimeout = 0;
	radioConfiguration.lbt.params.lbtNumOfSamples = 0;
	radioConfiguration.lbt.params.lbtScanPeriod = 0;
	radioConfiguration.lbt.params.lbtThreshold = 0;
	radioConfiguration.lbt.params.lbtTransmitOn = false;
	radioConfiguration.clockStabilizationDelay = 0;
	radioConfiguration.clockSource             = XTAL;
	radioConfiguration.fskPayloadIndex = 0;


    // Make sure we do not allocate multiple software timers just because the
    // radio's initialization function is called multiple times.
    if (0 == radioConfiguration.initialized)
    {
		StackRetStatus_t retVal = LORAWAN_SUCCESS;
		
        // This behavior depends on the compiler's behavior regarding
        // uninitialized variables. It should be configured to set them to 0.
        if (LORAWAN_SUCCESS == retVal)
		{
			retVal = SwTimerCreate(&radioConfiguration.timeOnAirTimerId);
		}
		
        if (LORAWAN_SUCCESS == retVal)
        {
			retVal = SwTimerCreate(&radioConfiguration.fskRxWindowTimerId);
		}
		
        if (LORAWAN_SUCCESS == retVal)
        {
			retVal = SwTimerCreate(&radioConfiguration.watchdogTimerId);
		}
/*#ifdef LBT*/
        if (LORAWAN_SUCCESS == retVal)
        {
			retVal = SwTimerCreate(&radioConfiguration.lbt.lbtScanTimerId);
		}
		

/*#endif*/ // LBT
        if (LORAWAN_SUCCESS == retVal)
        {

			radioConfiguration.initialized = 1;
		}
		else
		{
			/* free the allocated timers, if any */
			SwTimerReset();
		}
    }
    else
    {
        SwTimerStop(radioConfiguration.timeOnAirTimerId);
        SwTimerStop(radioConfiguration.fskRxWindowTimerId);
        SwTimerStop(radioConfiguration.watchdogTimerId);
/*#ifdef LBT*/
		SwTimerStop(radioConfiguration.lbt.lbtScanTimerId);
/*#endif*/ // LBT
    }

    RADIO_Reset();

	if (TCXO == HAL_GetRadioClkSrc())
	{
		radioConfiguration.clockSource             = TCXO;
		radioConfiguration.clockStabilizationDelay = HAL_GetRadioClkStabilizationDelay();
	}

	//Power On the Oscillator before putting the radio to standby state
    Radio_SetClockInput();

    // Perform image and RSSI calibration. This also puts the radio in FSK mode.
    // In order to perform image and RSSI calibration, we need the radio in
    // FSK mode. To do this, we first put it in sleep mode.
    Radio_WriteMode(MODE_STANDBY, MODULATION_FSK, 1);

    // Set frequency to do calibration at the configured frequency
    Radio_WriteFrequency(radioConfiguration.frequency);

    // Do not do auto calibration at runtime, start calibration now, Temp
    // threshold for monitoring 10 deg. C, Temperature monitoring enabled
    RADIO_RegisterWrite(REG_FSK_IMAGECAL, 0x42);

    // Wait for calibration to complete
    while ((RADIO_RegisterRead(REG_FSK_IMAGECAL) & 0x20) != 0)
        ;

    // High frequency LNA current adjustment, 150% LNA current (Boost on)
    RADIO_RegisterWrite(REG_LNA, 0x23);

    // Preamble detector on, 2 bytes trigger an interrupt, Chip errors tolerated
    // over the preamble size
    RADIO_RegisterWrite(REG_FSK_PREAMBLEDETECT, 0xAA);

    // Set FSK max payload length to 255 bytes
    RADIO_RegisterWrite(REG_FSK_PAYLOADLENGTH, 0xFF);

    // Packet mode
    RADIO_RegisterWrite(REG_FSK_PACKETCONFIG2, 1 << SHIFT6);

    // Go to LoRa mode for this register to be set
    Radio_WriteMode(MODE_SLEEP, MODULATION_LORA, 1);

    // Set LoRa max payload length
    RADIO_RegisterWrite(REG_LORA_PAYLOADMAXLENGTH, 0xFF);

    radioConfiguration.regVersion = RADIO_RegisterRead(REG_VERSION);
	
	//Power Off the Oscillator after putting the radio sleep state
	Radio_ResetClockInput();
	
}

/*********************************************************************//**
\brief	This function transmits the data by doing a task post to the
		RADIO_TxHandler.

\param param 	- Stores the transmission parameters.
\return			- The error condition for the transmit operation.
*************************************************************************/
RadioError_t RADIO_Transmit(RadioTransmitParam_t *param)
{
	/************************************************************************/
	/*	Note :	Here the actual check should also include RADIO_STATE_RX	*/
	/*          but for now we only check RADIO_STATE_IDLE as this is the	*/
	/*          current behavior of the code because timers are not			*/
	/*          separated for Rx and Tx, they are reused.					*/
	/************************************************************************/
    if (RADIO_STATE_IDLE != RADIO_GetState())
    {
        return ERR_RADIO_BUSY;
    }

    // Make sure the watchdog won't trigger MAC functions erroneously
	SwTimerStop(radioConfiguration.watchdogTimerId);
	
    txBufferLen = param->bufferLen;
    transmitBufferPtr = (param->bufferPtr);
	/*#ifdef LBT*/
// 	if (true == radioConfiguration.lbt.params.lbtTransmitOn)
// 	{
// 		RadioSetState(RADIO_STATE_SCAN);
// 		radioPostTask(RADIO_SCAN_TASK_ID);
// 	}
// 	else
	/*#endif*/ // LBT
	{
		RadioSetState(RADIO_STATE_TX);
		radioPostTask(RADIO_TX_TASK_ID);
	}

	return ERR_NONE;
}

/*********************************************************************//**
\brief	This function receives the data and stores it in the buffer
		pointer space by doing a task post to the RADIO_RxHandler.

\param param    - Stores the receive parameters. 
\return         - The error condition for the transmit operation.
*************************************************************************/

RadioError_t RADIO_Receive(RadioReceiveParam_t *param)

{
    if (RECEIVE_START == param->action)
    {
        if (RADIO_STATE_IDLE != RADIO_GetState())
        {
            return ERR_RADIO_BUSY;
        }
		
		// Make sure the watchdog won't trigger MAC functions erroneously
		SwTimerStop(radioConfiguration.watchdogTimerId);
		if (MODULATION_FSK == radioConfiguration.modulation)
		{
			SwTimerStop(radioConfiguration.fskRxWindowTimerId);
		}
		
        rxWindowSize = param->rxWindowSize;
        RadioSetState(RADIO_STATE_RX);
        radioPostTask(RADIO_RX_TASK_ID);
		
		//Power On the Oscillator before putting the radio to receive
		Radio_SetClockInput();

        return ERR_NONE;
    } 
    else /* RECEIVE_STOP */
    {
		if (RADIO_STATE_IDLE == RADIO_GetState())
		{
			return ERR_NONE;
		}
        else if (RADIO_STATE_RX != RADIO_GetState())
        {
            return ERR_INVALID_REQ;
        } 

        // Make sure the watchdog won't trigger MAC functions erroneously
        SwTimerStop(radioConfiguration.watchdogTimerId);
        if (MODULATION_FSK == radioConfiguration.modulation)
        {
            SwTimerStop(radioConfiguration.fskRxWindowTimerId);
        }

        /************************************************************************/
        /*  Note :	This is an example where we need to stop the reception      */
		/*			but if i call RADIO_SetAttr it executes only if the			*/
		/*			radio is in RADIO_STATE_IDLE, so we don't call that.		*/
		/*			This means that if something must be set or get				*/
		/*			internally within RADIO layer we directly access			*/
		/*			the static functions.										*/
        /************************************************************************/ 
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Powering Off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();

        // RxBuffer is released
        RadioSetState(RADIO_STATE_IDLE);
        radioClearTask(RADIO_RX_TASK_ID);
        radioClearTask(RADIO_RX_DONE_TASK_ID);
    }

    return ERR_NONE;
}

/*********************************************************************//**
\brief	This function handles the payload transfer of bytes from buffer
		to FIFO. 

\param[in] buffer Pointer to the data to be written into the frame buffer
\param[in] bufferLen Length of the data to be written
\return     - none
*************************************************************************/
void Radio_FSKTxPayloadHandler(uint8_t *buffer, uint8_t bufferLen)
{

	cpu_irq_enter_critical();
	if (radioConfiguration.fskPayloadIndex == 0)
	{
		if (bufferLen != 0)
		{
			if (bufferLen < RADIO_TX_FIFO_LEVEL)
			{
				RADIO_FrameWrite(REG_FIFO_ADDRESS, transmitBufferPtr, bufferLen);
				radioConfiguration.fskPayloadIndex = bufferLen;
			}
			else
			{
				RADIO_FrameWrite(REG_FIFO_ADDRESS, transmitBufferPtr, RADIO_TX_FIFO_LEVEL);
				radioConfiguration.fskPayloadIndex = RADIO_TX_FIFO_LEVEL;
			}
		}
	} 
	else
	{
		if ((bufferLen - radioConfiguration.fskPayloadIndex) != 0)
		{
			if ((uint8_t)(bufferLen - radioConfiguration.fskPayloadIndex) <= RADIO_TX_FIFO_LEVEL)
			{
				RADIO_FrameWrite(REG_FIFO_ADDRESS, transmitBufferPtr + radioConfiguration.fskPayloadIndex, (bufferLen - radioConfiguration.fskPayloadIndex));
				radioConfiguration.fskPayloadIndex = bufferLen;
			}
			else
			{
				RADIO_FrameWrite(REG_FIFO_ADDRESS, transmitBufferPtr + radioConfiguration.fskPayloadIndex, RADIO_TX_FIFO_LEVEL);
				radioConfiguration.fskPayloadIndex += RADIO_TX_FIFO_LEVEL;
			}
		}
	}
	cpu_irq_leave_critical();
	
}

/*********************************************************************//**
\brief	This function receives the data and stores it in the buffer
		pointer space by doing a task post to the RADIO_RxHandler.

\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t RADIO_TxHandler(void)
{
    uint8_t regValue;
	
	//Power on the Oscillator before putting the radio to transmit state
	Radio_SetClockInput();
		
    SwTimerStop(radioConfiguration.timeOnAirTimerId);
	
	if (true == radioConfiguration.lbt.params.lbtTransmitOn)
	{
		if (Radio_IsChannelFree() == false)
		{
			// Turning off the RF switch now.
			Radio_DisableRfControl(RADIO_RFCTRL_RX);
			//Powering Off the Oscillator after putting TRX to sleep
			Radio_ResetClockInput();
			
			RadioCallbackParam_t RadioCallbackParam;
			RadioCallbackParam.status = ERR_CHANNEL_BUSY;
			RadioSetState(RADIO_STATE_IDLE);
			if (1 == radioCallbackMask.BitMask.radioTxDoneCallback)
			{
				if (radioConfiguration.radioCallback)
				{
					radioConfiguration.radioCallback(RADIO_TX_DONE_CALLBACK, (void *) &(RadioCallbackParam));
					return SYSTEM_TASK_SUCCESS;

				}
			}
		}		
	}
	
	// Turn on the RF switch.
	Radio_EnableRfControl(RADIO_RFCTRL_TX);

	// Since we're interested in a transmission, rxWindowSize is irrelevant.
	// Setting it to 4 is a valid option.
	Radio_WriteConfiguration(4);

	if (MODULATION_LORA == radioConfiguration.modulation)
	{
		RADIO_RegisterWrite(REG_LORA_PAYLOADLENGTH, txBufferLen);

		// Configure PaRamp
		regValue = RADIO_RegisterRead(REG_PARAMP);
		regValue &= ~0x0F;    // Clear lower 4 bits
		regValue |= 0x08;     // 50us PA Ramp-up time
		RADIO_RegisterWrite(REG_PARAMP, regValue);

		// DIO0 = 01 means TxDone in LoRa mode.
		// DIO2 = 00 means FHSSChangeChannel
		RADIO_RegisterWrite(REG_DIOMAPPING1, 0x40);
		RADIO_RegisterWrite(REG_DIOMAPPING2, 0x00);

		Radio_WriteMode(MODE_STANDBY, radioConfiguration.modulation, 1);
		RADIO_FrameWrite(REG_FIFO_ADDRESS, transmitBufferPtr, txBufferLen);

	} 
	else // if (MODULATION_FSK == radioConfiguration.modulation)
	{
        //Radio_WriteMode(MODE_STANDBY, radioConfiguration.modulation, 1);
        
		radioConfiguration.fskPayloadIndex = 0;
        RADIO_FrameWrite(REG_FIFO, &txBufferLen, 1);
        if (txBufferLen > RADIO_TX_FIFO_LEVEL)
        {
            RADIO_FrameWrite(REG_FIFO, transmitBufferPtr, RADIO_TX_FIFO_LEVEL);
            radioConfiguration.fskPayloadIndex += RADIO_TX_FIFO_LEVEL;
        }
        else
        {
            RADIO_FrameWrite(REG_FIFO, transmitBufferPtr, txBufferLen);
            radioConfiguration.fskPayloadIndex = txBufferLen;
        }              
        
        RADIO_RegisterWrite(REG_DIOMAPPING1, 0x14);
        //    | REG_DIOMAPPING1_DIO0_BITS_00
        //    | REG_DIOMAPPING1_DIO1_BITS_01
        //    | REG_DIOMAPPING1_DIO2_BITS_01
        //    | REG_DIOMAPPING1_DIO3_BITS_00);

        RADIO_RegisterWrite(REG_DIOMAPPING2,
            (RADIO_RegisterRead(REG_DIOMAPPING2)
                & REG_DIOMAPPING2_DIO4_BITMASK
                & REG_DIOMAPPING2_DIO_BITMASK));
	}

	/****************************************************************************/
	/*  Non blocking switch. We don't really care when it starts transmitting.  */
	/*	If accurate timing of the time on air is required, the simplest way to	*/
	/*	achieve it is to change this to a blocking mode switch.					*/
	/****************************************************************************/
	Radio_WriteMode(MODE_TX, radioConfiguration.modulation, 0);

	/****************************************************************************/
	/*  Set timeout to some very large value since the timer counts down.		*/
	/*	Leaving the callback uninitialized it will assume the default value of	*/
	/*	NULL which in turns means no callback.									*/
	/****************************************************************************/
    timeOnAir = SwTimerGetTime();
		
	if (0 != radioConfiguration.watchdogTimerTimeout)
	{
		SwTimerStart(radioConfiguration.watchdogTimerId, MS_TO_US(radioConfiguration.watchdogTimerTimeout), SW_TIMEOUT_RELATIVE, (void *)Radio_WatchdogTimeout, NULL);
	}
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function receives the data and stores it in the buffer
		pointer space by doing a task post to the RADIO_RxHandler.

\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t RADIO_RxHandler(void)
{
	// Turn on the RF switch.
	Radio_EnableRfControl(RADIO_RFCTRL_RX); 

    if (0 == rxWindowSize)
    {
        Radio_WriteConfiguration(4);
    }
    else
    {
        Radio_WriteConfiguration(rxWindowSize);
    }

    if (MODULATION_LORA == radioConfiguration.modulation)
    {
        // All LoRa packets are received with explicit header, so this register
        // is not used. However, a value of 0 is not allowed.
        RADIO_RegisterWrite(REG_LORA_PAYLOADLENGTH, 0x01);

        // DIO0 = 00 means RxDone in LoRa mode
        // DIO1 = 00 means RxTimeout in LoRa mode
        // DIO2 = 00 means FHSSChangeChannel
        // Other DIOs are unused.
        RADIO_RegisterWrite(REG_DIOMAPPING1, 0x00);
        RADIO_RegisterWrite(REG_DIOMAPPING2, 0x00);
    }
    else
    {
        RADIO_RegisterWrite(REG_FSK_FIFOTHRESH, (0x80 | RADIO_RX_FIFO_LEVEL));
                
        RADIO_RegisterWrite(REG_FSK_RXBW, radioConfiguration.rxBw);
        RADIO_RegisterWrite(REG_FSK_AFCBW, radioConfiguration.afcBw);

        RADIO_RegisterWrite(REG_DIOMAPPING1,
            REG_DIOMAPPING1_DIO0_BITS_00 |
            REG_DIOMAPPING1_DIO1_BITS_00 |
            REG_DIOMAPPING1_DIO2_BITS_11 |
            REG_DIOMAPPING1_DIO3_BITS_01
        );

        RADIO_RegisterWrite(REG_DIOMAPPING2,
            (RADIO_RegisterRead(REG_DIOMAPPING2) &
                REG_DIOMAPPING2_DIO4_BITMASK &
                REG_DIOMAPPING2_DIO_BITMASK
            ) |
            REG_DIOMAPPING2_DIO4_BITS_11 |
            REG_DIOMAPPING2_MAPPREAMBLEDETECT
        );

		//Clear the packetRSSI value before putting the radio to FSK Rx
		radioConfiguration.packetRSSI = 0;
		
		//Clear the FSK length and index variables
		radioConfiguration.dataBufferLen = 0;
		radioConfiguration.fskPayloadIndex = 0;
    }

    // Will use non blocking switches to RadioSetMode. We don't really care
    // when it starts receiving.
    if (0 == rxWindowSize)
    {
        Radio_WriteMode(MODE_RXCONT, radioConfiguration.modulation, 0);
    }
    else
    {
        if (MODULATION_LORA == radioConfiguration.modulation)
        {
            Radio_WriteMode(MODE_RXSINGLE, MODULATION_LORA, 0);
        }
        else
        {
            Radio_WriteMode(MODE_RXCONT, MODULATION_FSK, 0);
            SwTimerStart(radioConfiguration.fskRxWindowTimerId, MS_TO_US(rxWindowSize), SW_TIMEOUT_RELATIVE, (void *)Radio_RxFSKTimeout, NULL);
        }
    }

    if ((0 != radioConfiguration.watchdogTimerTimeout) && (0 != rxWindowSize))
    {
         SwTimerStart(radioConfiguration.watchdogTimerId, MS_TO_US(radioConfiguration.watchdogTimerTimeout), SW_TIMEOUT_RELATIVE, (void *)Radio_WatchdogTimeout, NULL);
    }
    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function receives the data and stores it in the buffer
		pointer space by doing a task post to the RADIO_RxHandler.

\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t RADIO_TxDoneHandler(void)
{
    RadioCallbackParam_t RadioCallbackParam;
    if (1 == radioEvents.TxWatchdogTimoutEvent)
    {
        radioEvents.TxWatchdogTimoutEvent = 0;
        Radio_WriteMode(MODE_STANDBY, radioConfiguration.modulation, 1);
        RadioCallbackParam.TX.timeOnAir = radioConfiguration.watchdogTimerTimeout;
		RadioCallbackParam.status = ERR_NONE;
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		RadioSetState(RADIO_STATE_IDLE);
		// clearing fsk playload index
		radioConfiguration.fskPayloadIndex = 0;
        if (1 == radioCallbackMask.BitMask.radioTxTimeoutCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_TX_TIMEOUT_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
		RADIO_Reset();
		RADIO_InitDefaultAttributes();
    }
    else if ((1 == radioEvents.LoraTxDoneEvent) || (1 == radioEvents.FskTxDoneEvent))
    {
        radioEvents.LoraTxDoneEvent = 0;
        radioEvents.FskTxDoneEvent = 0;
        RadioCallbackParam.TX.timeOnAir = (uint32_t) timeOnAir;
		RadioCallbackParam.status = ERR_NONE;
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Powering Off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
		RadioSetState(RADIO_STATE_IDLE);
		// clearing fsk playload index
		radioConfiguration.fskPayloadIndex = 0;
        if (1 == radioCallbackMask.BitMask.radioTxDoneCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_TX_DONE_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
/*#ifdef LBT*/
	else if (1 == radioEvents.LbtScanDoneEvent)
	{
		radioScanDoneHandler();		
	}
/*#endif*/ //LBT
    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function receives the data and stores it in the buffer
		pointer space by doing a task post to the RADIO_RxHandler.

\param 	- none
\return	- returns the success or failure of a task
*************************************************************************/
SYSTEM_TaskStatus_t RADIO_RxDoneHandler(void)
{
    RadioCallbackParam_t RadioCallbackParam;
    if ((1 == radioEvents.RxWatchdogTimoutEvent))
    {
        radioEvents.RxWatchdogTimoutEvent = 0;
        Radio_WriteMode(MODE_STANDBY, radioConfiguration.modulation, 1);
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Power off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
        RadioSetState(RADIO_STATE_IDLE);
		RadioCallbackParam.status = ERR_NONE;
		//Clear the FSK length and index variables
		radioConfiguration.dataBufferLen = 0;
		radioConfiguration.fskPayloadIndex = 0;
        if (1 == radioCallbackMask.BitMask.radioRxTimeoutCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_RX_TIMEOUT_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
    else if ((1 == radioEvents.LoraRxTimoutEvent) || (1 == radioEvents.FskRxTimoutEvent))
    {
        radioEvents.LoraRxTimoutEvent = 0;
        radioEvents.FskRxTimoutEvent = 0;
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Power off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
        RadioSetState(RADIO_STATE_IDLE);
		RadioCallbackParam.status = ERR_NONE;
		//Clear the FSK length and index variables
		radioConfiguration.dataBufferLen = 0;
		radioConfiguration.fskPayloadIndex = 0;
        if (1 == radioCallbackMask.BitMask.radioRxTimeoutCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_RX_TIMEOUT_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
    else if (1 == radioEvents.LoraRxDoneEvent)
    {
        radioEvents.LoraRxDoneEvent = 0;

        radioConfiguration.dataBufferLen = RADIO_RegisterRead(REG_LORA_RXNBBYTES);
        RADIO_RegisterWrite(REG_LORA_FIFOADDRPTR, 0x00);
        RADIO_FrameRead(REG_FIFO_ADDRESS,radioConfiguration.dataBuffer,radioConfiguration.dataBufferLen);
		Radio_ReadPktRssi();

        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Power off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
        RadioCallbackParam.RX.buffer = radioConfiguration.dataBuffer;
        RadioCallbackParam.RX.bufferLength = radioConfiguration.dataBufferLen;
		RadioCallbackParam.status = ERR_NONE;
        RadioSetState(RADIO_STATE_IDLE);
        if (1 == radioCallbackMask.BitMask.radioRxDoneCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_RX_DONE_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
    else if (1 == radioEvents.FskRxDoneEvent)
    {
        radioEvents.FskRxDoneEvent = 0;
        radioConfiguration.packetSNR = -128;
        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Power off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
        RadioCallbackParam.RX.buffer = radioConfiguration.dataBuffer;
        RadioCallbackParam.RX.bufferLength = radioConfiguration.dataBufferLen;
		//Clear the FSK index variables
		radioConfiguration.fskPayloadIndex = 0;
		RadioCallbackParam.status = ERR_NONE;
        RadioSetState(RADIO_STATE_IDLE);
        if (1 == radioCallbackMask.BitMask.radioRxDoneCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_RX_DONE_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
    else if (1 == radioEvents.RxError)
    {
        radioEvents.RxError = 0;

        RADIO_FrameRead(REG_FIFO_ADDRESS,radioConfiguration.dataBuffer,radioConfiguration.dataBufferLen);

        Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
		//Power off the Oscillator after putting TRX to sleep
		Radio_ResetClockInput();
        RadioCallbackParam.RX.buffer = radioConfiguration.dataBuffer;
        RadioCallbackParam.RX.bufferLength = radioConfiguration.dataBufferLen;
		//Clear the FSK index variables
		radioConfiguration.fskPayloadIndex = 0;
		RadioCallbackParam.status = ERR_NONE;
        RadioSetState(RADIO_STATE_IDLE);
        if (1 == radioCallbackMask.BitMask.radioRxErrorCallback)
        {
			if (radioConfiguration.radioCallback)
			{
				radioConfiguration.radioCallback(RADIO_RX_ERROR_CALLBACK, (void *) &(RadioCallbackParam));
			}
        }
    }
    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief	This function is the callback function for watchdog timer 
        timeout.

\param time - The watchdog timeout time.
\return     - none
*************************************************************************/
void Radio_WatchdogTimeout(uint8_t time)
{
    (void)time;
	
    if (RADIO_STATE_RX == RADIO_GetState())
    {
        radioEvents.RxWatchdogTimoutEvent = 1;
		Radio_DisableRfControl(RADIO_RFCTRL_RX);
        radioPostTask(RADIO_RX_DONE_TASK_ID);
    } else if (RADIO_STATE_TX == RADIO_GetState())
    {
        radioEvents.TxWatchdogTimoutEvent = 1;
		Radio_DisableRfControl(RADIO_RFCTRL_TX);
        radioPostTask(RADIO_TX_DONE_TASK_ID);
    }
}



/*********************************************************************//**
\brief	This function is triggered when no packets are received within
        a specified Rx window. This is triggered by a hardware interrupt. 

\param  - none
\return - none
*************************************************************************/
void RADIO_RxTimeout(void)
{
    // Make sure the watchdog won't trigger MAC functions erroneously.
    SwTimerStop(radioConfiguration.watchdogTimerId);
	
	// Turning off the RF switch now.
	Radio_DisableRfControl(RADIO_RFCTRL_RX);
    RADIO_RegisterWrite(REG_LORA_IRQFLAGS, 1 << SHIFT7);

    radioEvents.LoraRxTimoutEvent = 1;
    radioPostTask(RADIO_RX_DONE_TASK_ID);
}


/*********************************************************************//**
\brief	This function is called when a sync address match happens or 
		when packet collision happens. In that case we clear the 
		FSKPayload index and datalen variables to download the new packet.  

\param		- none	
\return		- none.
*************************************************************************/
void RADIO_FSKSyncAddr(void)
{
	cpu_irq_enter_critical();
	if (MODULATION_FSK == radioConfiguration.modulation)
	{
		//Clear the FSK length and index variables
		radioConfiguration.dataBufferLen = 0;
		radioConfiguration.fskPayloadIndex = 0;
	}
	cpu_irq_leave_critical();
}

/*********************************************************************//**
\brief	This function is triggered when the FIFO is above the 
		threshold value. This is triggered by a hardware interrupt. 

\param  - none
\return - none
*************************************************************************/
void RADIO_FSKFifoLevel(void)
{
	cpu_irq_enter_critical();
	// The following code will be executed if radioConfiguration.dataBufferLen, radioConfiguration.fskPayloadIndex are not zero and if they are not equal
	if (!((radioConfiguration.dataBufferLen == radioConfiguration.fskPayloadIndex) && (radioConfiguration.dataBufferLen != 0) && (radioConfiguration.fskPayloadIndex != 0)))
	{
		if (radioConfiguration.dataBufferLen == 0)
		{
			RADIO_FrameRead(REG_FIFO_ADDRESS,(uint8_t *)&radioConfiguration.dataBufferLen, 1);
		}
		if ((uint8_t)(radioConfiguration.dataBufferLen - radioConfiguration.fskPayloadIndex) < RADIO_RX_FIFO_LEVEL)
		{
			RADIO_FrameRead(REG_FIFO_ADDRESS, radioConfiguration.dataBuffer + radioConfiguration.fskPayloadIndex, radioConfiguration.dataBufferLen - radioConfiguration.fskPayloadIndex);
			radioConfiguration.fskPayloadIndex = radioConfiguration.dataBufferLen;
		}
		else if ((uint8_t)(radioConfiguration.dataBufferLen - radioConfiguration.fskPayloadIndex) > RADIO_RX_FIFO_LEVEL)
		{
			RADIO_FrameRead(REG_FIFO_ADDRESS, radioConfiguration.dataBuffer + radioConfiguration.fskPayloadIndex, RADIO_RX_FIFO_LEVEL);
			radioConfiguration.fskPayloadIndex += RADIO_RX_FIFO_LEVEL;
		}
		// intentionally omitted handling the condition "if(radioConfiguration.dataBufferLen == RADIO_RX_FIFO_LEVEL) and 
		// if(radioConfiguration.dataBufferLen - radioConfiguration.fskPayloadIndex == RADIO_RX_FIFO_LEVEL)"
		// because emptying the frame buffer here will prevent "Payload ready" interrupt from being fired.
	}
	cpu_irq_leave_critical();

}

/*********************************************************************//**
\brief	This function is triggered when the FIFO is empty.
        This is triggered by a hardware interrupt. 

\param  - none
\return - none
*************************************************************************/
void RADIO_FSKFifoEmpty(void)
{
    if ((uint8_t)(txBufferLen - radioConfiguration.fskPayloadIndex) > RADIO_TX_FIFO_LEVEL)
    {
        // write next frag
        RADIO_FrameWrite(REG_FIFO,
            transmitBufferPtr + radioConfiguration.fskPayloadIndex,
            RADIO_TX_FIFO_LEVEL);
        radioConfiguration.fskPayloadIndex += RADIO_TX_FIFO_LEVEL;
    }
    else
    {
        // write last frag
        RADIO_FrameWrite(REG_FIFO,
            transmitBufferPtr + radioConfiguration.fskPayloadIndex,
            (uint8_t)(txBufferLen - radioConfiguration.fskPayloadIndex));
        radioConfiguration.fskPayloadIndex = txBufferLen;
    }
}

/*********************************************************************//**
\brief	This function is triggered by a timer timeout in FSK mode to save
        Rx power. 

\param time - The time for which the timeout was started.
\return     - none
*************************************************************************/
void Radio_RxFSKTimeout(uint8_t time)
{
    (void)time;
    // Make sure the watchdog won't trigger MAC functions erroneously.
    SwTimerStop(radioConfiguration.watchdogTimerId);
	
	// Turning off the RF switch now.
	Radio_DisableRfControl(RADIO_RFCTRL_RX);
    radioEvents.FskRxTimoutEvent = 1;
    radioPostTask(RADIO_RX_DONE_TASK_ID);
}

/*********************************************************************//**
\brief	This function is triggered by hardware interrupt when a LoRa
        is transmitted.

\param   - none
\return  - none
*************************************************************************/
void RADIO_TxDone(void)
{
    // Make sure the watchdog won't trigger MAC functions erroneously.
    SwTimerStop(radioConfiguration.watchdogTimerId);
	
	// Turning off the RF switch now.
	Radio_DisableRfControl(RADIO_RFCTRL_TX);
	
    RADIO_RegisterWrite(REG_LORA_IRQFLAGS, 1 << SHIFT3);
    if ((RADIO_GetState() == RADIO_STATE_TX) || (0 == radioEvents.RxWatchdogTimoutEvent))
    {
        radioEvents.LoraTxDoneEvent = 1;
        radioPostTask(RADIO_TX_DONE_TASK_ID);
       
        timeOnAir = US_TO_MS(SwTimerGetTime() - timeOnAir);
    }
}

/*********************************************************************//**
\brief	This function is triggered by hardware interrupt when a FSK
        is transmitted.

\param   - none
\return  - none
*************************************************************************/
void RADIO_FSKPacketSent(void)
{
    uint8_t irqFlags;

    irqFlags = RADIO_RegisterRead(REG_FSK_IRQFLAGS2);
    if ((1 << SHIFT3) == (irqFlags & (1 << SHIFT3)))
    {
        // Make sure the watchdog won't trigger MAC functions erroneously.
        SwTimerStop(radioConfiguration.watchdogTimerId);
	
		// Turning off the RF switch now.
		Radio_DisableRfControl(RADIO_RFCTRL_TX);
		
        if ((RADIO_GetState() == RADIO_STATE_TX) || (0 == radioEvents.RxWatchdogTimoutEvent))
        {
			timeOnAir =  US_TO_MS(SwTimerGetTime() - timeOnAir);
			radioPostTask(RADIO_TX_DONE_TASK_ID);
            radioEvents.FskTxDoneEvent = 1;
        }
    }
}

/*********************************************************************//**
\brief	This function is triggered by hardware interrupt when a LoRa
        is received.

\param   - none
\return  - none
*************************************************************************/
void RADIO_RxDone(void)
{
    uint8_t crc, irqFlags;
    irqFlags = RADIO_RegisterRead(REG_LORA_IRQFLAGS);
    // Clear RxDone interrupt (also CRC error and ValidHeader interrupts, if
    // they exist)
    RADIO_RegisterWrite(REG_LORA_IRQFLAGS, (1 << SHIFT6) | (1 << SHIFT5) | (1 << SHIFT4));

    if (((1 << SHIFT6) | (1 << SHIFT4)) == (irqFlags & ((1 << SHIFT6) | (1 << SHIFT4))))
    {
        // Make sure the watchdog won't trigger MAC functions erroneously.
        SwTimerStop(radioConfiguration.watchdogTimerId);
	
		// Turning off the RF switch now.
		Radio_DisableRfControl(RADIO_RFCTRL_RX);

        // Read CRC info from received packet header
        crc = RADIO_RegisterRead(REG_LORA_HOPCHANNEL);
        if ((0 == radioConfiguration.crcOn) || ((0 == (irqFlags & (1 << SHIFT5))) && (0 != (crc & (1 << SHIFT6)))))
        {
            // ValidHeader and RxDone are set from the initial if condition.
            // We get here either if CRC doesn't need to be set (crcOn == 0) OR
            // if it is present in the header and it checked out.

            // Radio did not go to standby automatically. Will need to be set
            // later on.
            radioEvents.LoraRxDoneEvent = 1;
            radioPostTask(RADIO_RX_DONE_TASK_ID);
        } 
        else
        {
            radioEvents.RxError = 1;
            radioPostTask(RADIO_RX_DONE_TASK_ID);
        }
    }
}

/*********************************************************************//**
\brief	This function is triggered by hardware interrupt when a FSK
        is received.

\param   - none
\return  - none
*************************************************************************/
void RADIO_FSKPayloadReady(void)
{
    uint8_t irqFlags;

    irqFlags = RADIO_RegisterRead(REG_FSK_IRQFLAGS2);
    if ((1 << SHIFT2) == (irqFlags & (1 << SHIFT2)))
    {
        // Clearing of the PayloadReady (and CrcOk) interrupt is done when the
        // FIFO is empty
        if (1 == radioConfiguration.crcOn)
        {
            if ((1 << SHIFT1) == (irqFlags & (1 << SHIFT1)))
            {
                // Make sure the watchdog won't trigger MAC functions erroneously
                SwTimerStop(radioConfiguration.watchdogTimerId);
                SwTimerStop(radioConfiguration.fskRxWindowTimerId);

				cpu_irq_enter_critical();
				// The following code will be executed if radioConfiguration.dataBufferLen, radioConfiguration.fskPayloadIndex are not zero and if they are not equal
				if (!( (radioConfiguration.dataBufferLen == radioConfiguration.fskPayloadIndex) && (radioConfiguration.dataBufferLen != 0) && (radioConfiguration.fskPayloadIndex != 0) ))
				{
					if (radioConfiguration.dataBufferLen == 0)
					{
						RADIO_FrameRead(REG_FIFO_ADDRESS,(uint8_t *)&radioConfiguration.dataBufferLen, 1);
					}
					if (radioConfiguration.fskPayloadIndex == 0)
					{
						RADIO_FrameRead(REG_FIFO_ADDRESS,radioConfiguration.dataBuffer,radioConfiguration.dataBufferLen);
						radioConfiguration.fskPayloadIndex = radioConfiguration.dataBufferLen;
					}
					else if ((radioConfiguration.dataBufferLen - radioConfiguration.fskPayloadIndex) > 0)
					{
						RADIO_FrameRead(REG_FIFO_ADDRESS,radioConfiguration.dataBuffer+radioConfiguration.fskPayloadIndex,radioConfiguration.dataBufferLen-radioConfiguration.fskPayloadIndex);
						radioConfiguration.fskPayloadIndex = radioConfiguration.dataBufferLen;
					}
				}	
				cpu_irq_leave_critical();			

				// Turning off the RF switch now.
				Radio_DisableRfControl(RADIO_RFCTRL_RX);
                radioEvents.FskRxDoneEvent = 1;
                radioPostTask(RADIO_RX_DONE_TASK_ID);
            } 
            else
            {
                // This error handing did not exist before for FSK
                // Previously a packet with CRC error in FSK will be dropped
                radioEvents.RxError = 1;
                radioPostTask(RADIO_RX_DONE_TASK_ID);
            }
        } 
        else
        {
            // Make sure the watchdog won't trigger MAC functions erroneously
            SwTimerStop(radioConfiguration.watchdogTimerId);
            SwTimerStop(radioConfiguration.fskRxWindowTimerId);
			RADIO_FrameRead(REG_FIFO_ADDRESS,(uint8_t *)&radioConfiguration.dataBufferLen, 1);
			RADIO_FrameRead(REG_FIFO_ADDRESS,radioConfiguration.dataBuffer,radioConfiguration.dataBufferLen);
			// Turning off the RF switch now.
			Radio_DisableRfControl(RADIO_RFCTRL_RX);
            radioEvents.FskRxDoneEvent = 1;
            radioPostTask(RADIO_RX_DONE_TASK_ID);
        }
    }
}

/*********************************************************************//**
\brief	This function can be called by MAC to read radio buffer pointer
		and length of a receive frame(typically in rxdone)

\param   - place holder for data pointer, length
\return  - ERR_NONE. Other types are not used now.
*************************************************************************/
RadioError_t RADIO_GetData(uint8_t **data, uint16_t *dataLen)
{
	*data = radioConfiguration.dataBuffer;
	*dataLen = radioConfiguration.dataBufferLen;

	return ERR_NONE;
}

/*********************************************************************//**
\brief This function is to set RF front End Control.

\param state - The state of the radio to be set to.
*************************************************************************/
void Radio_EnableRfControl(bool type)
{
	RFCtrl1_t RFCtrl1 = 0;
	RFCtrl2_t RFCtrl2 = (RFCtrl2_t)type;
	if ( (radioConfiguration.frequency >= FREQ_410000KHZ) && (radioConfiguration.frequency <= FREQ_525000KHZ))
	{
		RFCtrl1 = RFO_LF;
	}
	else if ((radioConfiguration.frequency >= FREQ_862000KHZ) && (radioConfiguration.frequency <= FREQ_1020000KHZ))
	{
		if(!radioConfiguration.paBoost)
		{
			RFCtrl1 = RFO_HF;
		}
		else
		{
			RFCtrl1 = PA_BOOST;
		}
	}
	HAL_EnableRFCtrl(RFCtrl1,RFCtrl2);	
}

/*********************************************************************//**
\brief This function is to disable the RF front End Control.

\param state - The state of the radio to be set to.
*************************************************************************/
void Radio_DisableRfControl(bool type)
{
	RFCtrl1_t RFCtrl1 = 0;
	RFCtrl2_t RFCtrl2 = (RFCtrl2_t)type;	
	if ( (radioConfiguration.frequency >= FREQ_410000KHZ) && (radioConfiguration.frequency <= FREQ_525000KHZ))
	{
		RFCtrl1 = RFO_LF;
	}
	else if ((radioConfiguration.frequency >= FREQ_862000KHZ) && (radioConfiguration.frequency <= FREQ_1020000KHZ))
	{
		if(!radioConfiguration.paBoost)
		{
			RFCtrl1 = RFO_HF;
		}
		else
		{
			RFCtrl1 = PA_BOOST;
		}
	}
	HAL_DisableRFCtrl(RFCtrl1,RFCtrl2);
}

/*********************************************************************//**
\brief	This function sets the clock input to Radio.
		It will inform the radio from which clock it is sourced
*************************************************************************/
void Radio_SetClockInput(void)
{
	uint8_t tcxoOn;
	if (TCXO == radioConfiguration.clockSource)
	{
		tcxoOn = RADIO_RegisterRead(REG_TCXO);
		// Set TcxoInputOn bit (bit 4) to One
		RADIO_RegisterWrite(REG_TCXO, tcxoOn | (1 << SHIFT4));
		HAL_TCXOPowerOn();
	}
    //else if XTAL is a source it will be powered on by default

}

/*********************************************************************//**
\brief	This function power off the clock source of Radio
*************************************************************************/
void Radio_ResetClockInput(void)
{
	if (TCXO == radioConfiguration.clockSource)
	{
		HAL_TCXOPowerOff();
	}
}

/*********************************************************************//**
\brief	This function reads the packetRSSI value from Radio register
*************************************************************************/
static void Radio_ReadPktRssi(void)
{
	radioConfiguration.packetSNR = RADIO_RegisterRead(REG_LORA_PKTSNRVALUE);
	if (radioConfiguration.packetSNR & 0x80)
	{
		radioConfiguration.packetSNR = ((~ radioConfiguration.packetSNR + 1) & 0xFF) >> 2;
		radioConfiguration.packetSNR = -radioConfiguration.packetSNR;
	}
	else
	{
		radioConfiguration.packetSNR = (radioConfiguration.packetSNR & 0xFF) >> 2;
	}
	
	int16_t pktrssi = RADIO_RegisterRead(REG_LORA_PKTRSSIVALUE);
	
	if (radioConfiguration.packetSNR < 0)
	{
		if ((radioConfiguration.frequency >= FREQ_862000KHZ) && (radioConfiguration.frequency <= FREQ_1020000KHZ))
		{
			radioConfiguration.packetRSSI = RSSI_HF_OFFSET + pktrssi + (pktrssi >> 4) + radioConfiguration.packetSNR;
		}
		else if ( (radioConfiguration.frequency >= FREQ_410000KHZ) && (radioConfiguration.frequency <= FREQ_525000KHZ))
		{
			radioConfiguration.packetRSSI = RSSI_LF_OFFSET + pktrssi + (pktrssi >> 4) + radioConfiguration.packetSNR;
		}
	}
	else
	{
		if ((radioConfiguration.frequency >= FREQ_862000KHZ) && (radioConfiguration.frequency <= FREQ_1020000KHZ))
		{
			radioConfiguration.packetRSSI = RSSI_HF_OFFSET + pktrssi + (pktrssi >> 4);
		}
		else if ( (radioConfiguration.frequency >= FREQ_410000KHZ) && (radioConfiguration.frequency <= FREQ_525000KHZ))
		{
			radioConfiguration.packetRSSI = RSSI_LF_OFFSET + pktrssi + (pktrssi >> 4) ;
		}
	}
	
}

/*********************************************************************//**
\brief	This function checks whether the configured channel is free 
		by reading the RSSI at a particular frequency and comparing 
		that with lbtThreshold
*************************************************************************/
static bool Radio_IsChannelFree(void)
{
	uint16_t lbtScanPeriod = radioConfiguration.lbt.params.lbtScanPeriod;
	
	//50uS for reading a single RSSI sample
	radioConfiguration.lbt.params.lbtNumOfSamples = MS_TO_US(lbtScanPeriod) / 50;

	//Power on the Oscillator before putting the radio to receive state
	Radio_SetClockInput();
	
	// Turn on the RF switch.
	Radio_EnableRfControl(RADIO_RFCTRL_RX);

	//Write the center frequency of the channel to be check for RSSI
	Radio_WriteFrequency(radioConfiguration.frequency);
	radioConfiguration.lbt.lbtChannelRSSI = 0;
	
	Radio_WriteMode(MODE_SLEEP, MODULATION_FSK, BLOCKING_REQ);
	
	/* Disable all the interrupt line before putting Radio to RX to avoid  unwanted interrupts */
	Radio_DisableInterruptLines();
	
	/* Write Bandwidth as 200KHz to read RSSI throughout channel bandwidth */
	RADIO_RegisterWrite(REG_FSK_RXBW, FSKBW_200_0KHZ);
	
	/* Put radio to RX Continuous mode */
	Radio_WriteMode(MODE_RXCONT, MODULATION_FSK, BLOCKING_REQ);
	
	/* Check the channel freeness for lbtScanPeriod */
	for(uint8_t i = 0; i < radioConfiguration.lbt.params.lbtNumOfSamples; i++) 
	{
		 /* Read the channel Instantaneous RSSI value */
		 Radio_ReadFSKRssi(&instRSSI );		 

		 /* Inform the upper layer immediately if Inst RSSI is greater than RSSI Threshold*/
		 if (instRSSI > radioConfiguration.lbt.params.lbtThreshold)
		 {
			 /* Enable all the interrupt lines back */
			 Radio_EnableInterruptLines();
			 return false;
		 }
	}
	/* Enable all the interrupt lines back */
	Radio_EnableInterruptLines();
	return true;	
}

/*********************************************************************//**
\brief	This function enables all the interrupt line from RADIO
*************************************************************************/
static void Radio_EnableInterruptLines(void)
{
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
}

/*********************************************************************//**
\brief	This function disables all the interrupt line from RADIO to avoid 
		the unwanted interrupts
*************************************************************************/
static void Radio_DisableInterruptLines(void)
{
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
}

/* eof radio_transaction.c */
