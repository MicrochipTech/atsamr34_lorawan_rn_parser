/**
* \file  lorawan_task_handler.h
*
* \brief LoRaWAN Task Handler header file
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
 
#ifndef LORAWAN_TASK_HANDLER_H_
#define LORAWAN_TASK_HANDLER_H_

/******************************************************************************
                   Includes section
******************************************************************************/
#include <system_task_manager.h>

/******************************************************************************
                              Types section
 ******************************************************************************/
/** External req identifiers. */
typedef enum
{	
	LORAWAN_JOIN_TASK_ID	= 0u,
    LORAWAN_TX_TASK_ID		= 1u,
    LORAWAN_RX_TASK_ID		= 2u
}lorawanTaskID_t;

/******************************************************************************
                             Constants section
 ******************************************************************************/
#define LORAWAN_TASKS_SIZE         3u

/*************************** FUNCTIONS PROTOTYPE ******************************/

/** LORAWAN Subtask Handlers*/
SYSTEM_TaskStatus_t LORAWAN_TxHandler(void);

SYSTEM_TaskStatus_t LORAWAN_JoinReqHandler(void);

SYSTEM_TaskStatus_t LORAWAN_RxHandler(void);

/** Lorawan post task - Post a task to Lorawan Handler*/
void LORAWAN_PostTask(const lorawanTaskID_t taskID);

/** helper function for setting up radio for transmission */
void ConfigureRadioTx(radioConfig_t radioConfig);

#endif /* LORAWAN_TASK_HANDLER_H_ */
