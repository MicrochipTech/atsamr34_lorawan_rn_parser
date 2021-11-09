/**
* \file  lorawan_classc.c
*
* \brief LoRaWAN Class C implementation and helper functions
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
#include "lorawan.h"
#include "lorawan_private.h"
#include "conf_stack.h"

/****************************** INCLUDES **************************************/
#include "lorawan_defs.h"
#include "lorawan_radio.h"
#include "sw_timer.h"
#include "lorawan_task_handler.h"
#include "lorawan_reg_params.h"
#include "radio_interface.h"
#include "system_assert.h"

/************************** EXTERNAL VARIABLES ********************************/
extern LoRa_t loRa;

/************************ PRIVATE FUNCTION PROTOTYPES *************************/

/****************************** PUBLIC FUNCTIONS ******************************/

/*********************************************************************//**
\brief	Class C handling for uplink ack timer callback
\param[in]  - parameter for optional usage
*************************************************************************/
void LorawanClasscUlAckTimerCallback(uint8_t param)
{
#if (FEATURE_CLASSC == 1)
    loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = false;
#endif /* #if (FEATURE_CLASSC == 1) */
}

/*********************************************************************//**
\brief	Class C handling for transmit request

\return		- LORAWAN_SUCCESS, if request is honored,
              LORAWAN_BUSY, if stack is busy,
			  LORAWAN_INVALID_REQUEST, if request is not valid 
			  with current configuration of stack
*************************************************************************/
StackRetStatus_t LorawanClasscValidateSend(void)
{
    StackRetStatus_t status = LORAWAN_SUCCESS;

#if (FEATURE_CLASSC == 1)
    /*  class C:

        Follow a common approach of "if the previous transmit
        transaction is not over(callback not yet done), reject
        the new one.

        Two exceptions though : (1) UL Ack timer running,
        which means NS is waiting for ack hence takes priority (2) Radio
        is transmitting a frame(and it can not be aborted)

        if ((!transactionDone) ||
            (state == TRANSMISSION_OCCURING) ||
            (!UL Ack Timer Running))
            return equivalent failure
    */
    if (((!loRa.isTransactionDone) && (!SwTimerIsRunning(loRa.classCParams.ulAckTimerId))) ||
        (TRANSMISSION_OCCURRING == loRa.macStatus.macState))
    {
        status = LORAWAN_BUSY;
    }
#else /* #if (FEATURE_CLASSC == 1) */
    status = LORAWAN_INVALID_REQUEST;
#endif /* #if (FEATURE_CLASSC == 1) */

    return status;
}

/*********************************************************************//**
\brief	Class C handling for pausing MAC for direct access of radio from
        application. Returns the time after which mac can be paused

\return	- Returns the time after which mac can be paused
*************************************************************************/
uint32_t LorawanClasscPause(void)
{
    uint32_t timeToPause = 0;
#if (FEATURE_CLASSC == 1)
    /*
        class C:
        if(transactionDone)
			receive_stop()
			return UINT32_MAX
    */
    if (RX2_OPEN == loRa.macStatus.macState)
	{
		if (true == loRa.isTransactionDone)
		{
			RadioReceiveParam_t RadioReceiveParam;

			RadioReceiveParam.action = RECEIVE_STOP;
			if (ERR_NONE != RADIO_Receive(&RadioReceiveParam))
			{
				SYS_ASSERT_ERROR(ASSERT_MAC_PAUSE_RXSTOPFAIL);
			}

			timeToPause = UINT32_MAX;
		}
		else
		{
			if(SwTimerIsRunning(loRa.ackTimeoutTimerId) || SwTimerIsRunning(loRa.classCParams.ulAckTimerId))
			{
				timeToPause = 0;
			}
			else
			{
				timeToPause = UINT32_MAX;
			}
		}
	}
	else if (IDLE == loRa.macStatus.macState)
	{
		timeToPause = UINT32_MAX;
	}

	if (UINT32_MAX == timeToPause)
    {
        loRa.macStatus.macState = IDLE;
    }

#else /* #if (FEATURE_CLASSC == 1) */
    timeToPause = UINT32_MAX;
#endif /* #if (FEATURE_CLASSC == 1) */

    return timeToPause;
}

/*********************************************************************//**
\brief	Class C handling for receive-delay-1 timeout. Switch to RX1
        receive
*************************************************************************/
void LorawanClasscReceiveWindowCallback(void)
{
#if (FEATURE_CLASSC == 1)
    /*
        Class C:

        radio_receive(stop)
        ((class A code configures radio receive for preamble timeout
          with RX1 params))
    */
  
    RadioReceiveParam_t RadioReceiveParam;
    RadioReceiveParam.action = RECEIVE_STOP;
    if (ERR_NONE != RADIO_Receive(&RadioReceiveParam))
    {
        SYS_ASSERT_ERROR(ASSERT_MAC_RXCALLBACK_RXSTOPFAIL);
    }
#endif /* #if (FEATURE_CLASSC == 1) */
}

/*********************************************************************//**
\brief	Class C handling for receive data. The caller validates the frame.
        Just class C handling is down here

\param[in] hdr  - mac header of the incoming frame as received
*************************************************************************/
void LorawanClasscRxDone(Hdr_t *hdr)
{
#if (FEATURE_CLASSC == 1)
    /*
        class C

        if (mcast pkt)
            notify app
            continue in Rx(do not move to idle)
        else
            if(loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage)
                if(ack set in DL pkt)
                    stop automaticReplyTimerId
                    clear ackRequiredFromNextDownlinkMessage
                    send app callback
                    //rx2ReceiveDuration = infinite
                    Continue receive, wait for timeout
                else
                    if (ackRequiredFromNextUplinkMessage)
                        if ((automaticReply == 1) && (syncronization == 0))
                            Send pkt to app
                            transmit ack immediately
                            clear ackRequiredFromNextUplinkMessage
                            Continue receive, wait for timeout
                        else
                            if(ulAckTimerId running)
                                if(DL packet a retry packet)
                                    continue receive, wait for timeout
                                else
                                    send pkt to app
                                    clear ackRequiredFromNextUplinkMessage
                                    stop ulAckTimerId
                                    continue receive, wait for timeout
                            else
                                start ulAckTimerId
                                set ackRequiredFromNextUplinkMessage
                                continue receive, wait for timeout

                    else
                        Send pkt to app
                        Continue receive, wait for timeout

            else    //UNCNF
                notify app of tx complete
                stay back in the same receive state
                check for UL Ack handling
    */

    if (FRAME_TYPE_DATA_CONFIRMED_DOWN == hdr->members.mhdr.bits.mType)
    {
        if (SwTimerIsRunning(loRa.classCParams.ulAckTimerId))
        {
            loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = false;
        }
        else
        {
			SwTimerStart(loRa.classCParams.ulAckTimerId, MS_TO_US(RETRANSMIT_TIMEOUT), SW_TIMEOUT_RELATIVE, (void *)LorawanClasscUlAckTimerCallback, NULL);
        }
    }

	if (loRa.macStatus.macState == RX1_OPEN)
	{
		loRa.macStatus.macState = RX2_OPEN;
	}
	//Move to Receive state after packet reception
	loRa.enableRxcWindow = true;
	LorawanConfigureRadioForRX2(false);
#endif /* #if (FEATURE_CLASSC == 1) */
}

/*********************************************************************//**
\brief	Class C handling for transmit completion from radio

\param[in]  rxWindowOffset2  - preamble wait time for RX2
\return	    - none.
*************************************************************************/
void LorawanClasscTxDone(int8_t rxWindowOffset2)
{
#if (FEATURE_CLASSC == 1)
    /*  Class C:
        if (UL CNF && retry count is non zero)
            start UL retry timer for (RX2 + ACKTIMEOUT)
        else
            start UL retry timer for (RX2 + PRMBL)

        Configure RX2 receive
    */

    uint32_t timeout = loRa.protocolParameters.receiveDelay2;
    bool isSet = false;

    if (((true == loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage) &&
        ((loRa.maxRepetitionsConfirmedUplink + 1) >= loRa.counterRepetitionsConfirmedUplink) && (loRa.retransmission == ENABLED)))
    {
        timeout += RETRANSMIT_TIMEOUT;
		timeout += rxWindowOffset2;
        isSet = true;
    }

	if(((false == loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage) &&
    ((loRa.maxRepetitionsUnconfirmedUplink + 1) >= loRa.counterRepetitionsUnconfirmedUplink) && (loRa.retransmission == ENABLED)))
	 {
		 timeout += 2000;
		 timeout += rxWindowOffset2;
		 isSet = true;
	 }

    if (false == isSet)
    {
        timeout += rxWindowOffset2;
    }

	/* ACK sent - Stop the UL ACK timer */
	SwTimerStop(loRa.classCParams.ulAckTimerId);

	SwTimerStart(loRa.ackTimeoutTimerId, MS_TO_US(timeout), SW_TIMEOUT_RELATIVE, (void *)AckRetransmissionCallback, NULL);

    LorawanConfigureRadioForRX2(false);
#endif /* #if (FEATURE_CLASSC == 1) */
}

/*********************************************************************//**
\brief	Class C handling for receive timeout from radio. Generally, radio
        will be put back to idle and again to receive. Check if RX1 or RX2
        timeout, and take action
*************************************************************************/
void LorawanClasscRxTimeout(void)
{
#if (FEATURE_CLASSC == 1)
    /*
        if (state is RX1 open)
            move the state to rx2_open

        put radio to idle
        configure radio again to receive
    */

    if(RX1_OPEN == loRa.macStatus.macState) 
    {
        loRa.macStatus.macState = BETWEEN_RX1_RX2;
		LorawanConfigureRadioForRX2(false);	
    }
    
	else
 	{
 		if((RX2_OPEN == loRa.macStatus.macState) && (loRa.isTransactionDone == true))
 		{
 			SYS_ASSERT_ERROR(ASSERT_MAC_CLASSCRX2TIMEOUT_STATEFAIL);
 		}
		else
		{
		    LorawanConfigureRadioForRX2(false);
		}
	}
#endif /* #if (FEATURE_CLASSC == 1) */
}

/*********************************************************************//**
\brief	Provide receive data callback to application conditionally on
        few parameters
\param[in]  devAddr - 32 bit device address
\param[in]  pData   - pointer to received data
\param[in]  dataLength - length of the received data
\param[in]  status  -   LORAWAN_SUCCESS if valid packet
                        LORAWAN_INVALID_PARAMETER otherwise
*************************************************************************/
void LorawanClasscNotifyAppOnReceive(uint32_t devAddr, uint8_t *pData,uint8_t dataLength, StackRetStatus_t status)
{
#if (FEATURE_CLASSC == 1)
	if(!(SwTimerIsRunning(loRa.classCParams.ulAckTimerId)))
	{
		/* Send RX Available Event */
		UpdateRxDataAvailableCbParams(devAddr, pData, dataLength, status);
	}
#endif /* #if (FEATURE_CLASSC == 1) */
}

