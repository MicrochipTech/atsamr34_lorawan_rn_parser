/**
* \file  radio_task_manager.c
*
* \brief This is the Radio Driver Task Manager source file which contains Radio task
*		 scheduler of the Radio Driver
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


/******************************************************************************
                   Includes section
******************************************************************************/
#include "radio_task_manager.h"
#include "atomic.h"
#include <stdint.h>

/************************************************************************/
/*  Static variables                                                    */
/************************************************************************/

/**************************************************************************//**
\brief radioTaskFlags - 16-bit bitmap for the tasks of RADIO layer.
******************************************************************************/
static volatile uint16_t radioTaskFlags = 0x0000u;

/**************************************************************************//**
\brief Array of radio task handlers.
******************************************************************************/
static SYSTEM_TaskStatus_t (*radioTaskHandlers[RADIO_TASKS_COUNT])(void) = {
    /* In the order of descending priority */
    RADIO_TxDoneHandler,
    RADIO_RxDoneHandler,
    RADIO_TxHandler,
    RADIO_RxHandler,
	RADIO_ScanHandler
    /* , RADIO_SleepHandler */
};

/******************************************************************************
                   Prototypes section
******************************************************************************/
SYSTEM_TaskStatus_t RADIO_TaskHandler(void);

/******************************************************************************
                   Implementations section
******************************************************************************/
/**************************************************************************//**
\brief Set task for RADIO task manager.

\param[in] id - a single value from the type RadioTaskIds_t
******************************************************************************/
void radioPostTask(RadioTaskIds_t id)
{
    ATOMIC_SECTION_ENTER
    radioTaskFlags |= id;
    ATOMIC_SECTION_EXIT

    /* Also post a RADIO task to the system */
    SYSTEM_PostTask(RADIO_TASK_ID);
}

/**************************************************************************//**
\brief Clear task for RADIO task manager.

\param[in] id - a single value from the type RadioTaskIds_t
******************************************************************************/
void radioClearTask(RadioTaskIds_t id)
{
    ATOMIC_SECTION_ENTER
    radioTaskFlags &= ~id;
    ATOMIC_SECTION_EXIT
}

/**************************************************************************//**
\brief RADIO task handler.
******************************************************************************/
SYSTEM_TaskStatus_t RADIO_TaskHandler(void)
{
    if (radioTaskFlags)
    {
        for (uint16_t taskId = 0; taskId < RADIO_TASKS_COUNT; taskId++)
        {
            if ((1 << taskId) & (radioTaskFlags))
            {

                ATOMIC_SECTION_ENTER
                radioTaskFlags &= ~(1 << taskId);
                ATOMIC_SECTION_EXIT

                radioTaskHandlers[taskId]();

                if (radioTaskFlags)
                {
                    SYSTEM_PostTask(RADIO_TASK_ID);
                }
                
                break;
            }
        }
    }
    /*
     * else
     * {
     *   radioPostTask(RADIO_SLEEP_TASK_ID);
       * SYSTEM_PostTask(RADIO_TASK_ID);
     * }
     */

    return SYSTEM_TASK_SUCCESS;
}

/* eof radio_task_manager.c */
