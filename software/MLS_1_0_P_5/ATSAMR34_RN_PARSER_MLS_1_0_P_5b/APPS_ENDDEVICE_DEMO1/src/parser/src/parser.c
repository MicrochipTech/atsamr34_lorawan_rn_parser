/**
* \file  parser.c
*
* \brief Main parser file
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
#include "stack_common.h"
#include "parser.h"
#include "conf_board.h"
#include "parser_commands.h"
#include "parser_tsp.h"
#include "parser_lorawan.h"
#include "conf_app.h"
#include "parser_utils.h"
#include "sio2host.h"
#include "system_task_manager.h"
#include "pds_interface.h"

#define VER_STR            STACK_VER

#ifndef HW_STR
#define HW_STR			   "USER BOARD"
#endif

uint32_t pdsAppCustomParameter;
#define PDS_FILE_APP_CUSTOMPARAMETER_ADDR ((uint8_t *) & pdsAppCustomParameter)
#define PDS_FILE_APP_CUSTOMPARAMETER_SIZE (sizeof(pdsAppCustomParameter))
PdsOperations_t appPdsFileOps[PDS_APP_FILEID_MAX_VALUE];

ItemMap_t appPdsFileItemList[] = {
	DECLARE_ITEM(PDS_FILE_APP_CUSTOMPARAMETER_ADDR,
	PDS_FILE_APP_DATA1_13_IDX,
	(uint8_t)PDS_APP_CUSTOMPARAMETER,
	PDS_FILE_APP_CUSTOMPARAMETER_SIZE,
	PDS_FILE_START_OFFSET)
};

typedef struct parserRxCmd_tag
{
    char cmd[PARSER_DEF_CMD_MAX_LEN];
    uint16_t wordLen[PARSER_DEF_CMD_MAX_IDX];
    uint16_t wordStartPos[PARSER_DEF_CMD_MAX_IDX];
    uint8_t bCmdStatus;
    uint8_t crtWordIdx;
    uint16_t crtCmdPos;
    uint16_t crtWordPos;
}parserRxCmd_t;

static uint8_t Parser_ProcessCmd(const parserCmdEntry_t* pParserCmd, uint8_t nbParserCmd, uint8_t rxCmdIdx,
                                 uint8_t* pSavedCmdIdx);


extern volatile parserRxCmd_t mRxParserCmd;

static const char* gapParserStatus[] =
{
    "ok",
    "invalid_param",
    "err"
};

char aParserData[PARSER_MAX_DATA_LEN];

void parser_serial_data_handler(void)
{
    int rxChar;
   /* verify if there was any character received*/
    if((-1) != (rxChar = sio2host_getchar_nowait()))
    {
        Parser_RxAddChar( (uint8_t)rxChar );
        SYSTEM_PostTask(APP_TASK_ID);
    }
}

void Parser_Init(void)
{
    Parser_RxClearBuffer();
    /* Initialize LoRaWAN stack */
    Parser_LorawanInit();
	
	{
		PdsFileMarks_t appFileMarks;
		appFileMarks.fileMarkListAddr = appPdsFileOps;
		appFileMarks.numItems = (uint8_t)(PDS_APP_FILEID_MAX_VALUE & 0x00FF);
		appFileMarks.itemListAddr = appPdsFileItemList;
		appFileMarks.fIDcb = NULL;
		PDS_RegFile(PDS_FILE_APP_DATA1_13_IDX, appFileMarks);
	}
}

void Parser_Main (void)
{
    uint8_t cmdTotalNb;
    uint8_t startCmdSize = gParserStartCmdSize;
    const parserCmdEntry_t* pStartCmd = gpParserStartCmd;
    uint8_t crtWordIdx;
    uint8_t savedCmdIdx;
    parserCmdEntry_t tempCmd;

    /* verify if there was any character received */
    int rxChar;
    while((-1) != (rxChar = sio2host_getchar_nowait()))
    {
        Parser_RxAddChar( (uint8_t)rxChar );
    }

    /* Verify if an entire command is received */
    if(mRxParserCmd.bCmdStatus)
    {
        cmdTotalNb = mRxParserCmd.crtWordIdx + 1;
        crtWordIdx = 0;

        while(cmdTotalNb)
        {
            if(Parser_ProcessCmd(pStartCmd, startCmdSize, crtWordIdx, &savedCmdIdx))
            {
                /* Further processing is needed, continue with group commands */
                tempCmd = *(pStartCmd + savedCmdIdx);
                startCmdSize = tempCmd.nextParserCmdSize;
                pStartCmd = tempCmd.pNextParserCmd;

                /* Process the next command */
                crtWordIdx ++;

                cmdTotalNb --;
            }
            else
            {
                break;
            }
        }

        Parser_RxClearBuffer();
    }

}

void Parser_GetSwVersion(char* pBuffData)
{
    /* Set HW */
    memcpy(pBuffData, HW_STR, sizeof(HW_STR));
    pBuffData[sizeof(HW_STR) - 1] = ' ';
    /* Set firmware version */
    memcpy(&pBuffData[sizeof(HW_STR)], VER_STR, sizeof(VER_STR));
    pBuffData[sizeof(HW_STR) + sizeof(VER_STR) - 1] = ' ';
    /* Set date and time */
    memcpy(&pBuffData[sizeof(HW_STR) + sizeof(VER_STR)], __DATE__, sizeof(__DATE__));
    pBuffData[sizeof(HW_STR) + sizeof(VER_STR) + sizeof(__DATE__) - 1] = ' ';
    memcpy(&pBuffData[sizeof(HW_STR) + sizeof(VER_STR) + sizeof(__DATE__)], __TIME__, sizeof(__TIME__));
    pBuffData[sizeof(HW_STR) + sizeof(VER_STR) + sizeof(__DATE__) + sizeof(__TIME__)] = '\0';
}

static uint8_t Parser_ProcessCmd(const parserCmdEntry_t* pParserCmd, uint8_t nbParserCmd, uint8_t rxCmdIdx,
                                 uint8_t* pSavedCmdIdx)
{
    uint8_t cmdCtr;
    uint8_t retValue = 0x00U; /* Consider returning error by default */
    parserCmdInfo_t parserCmdInfo;
    parserCmdEntry_t parserCmdEntry;

    parserCmdInfo.pReplyCmd = (char*)gapParserStatus[INVALID_PARAM_IDX]; /* Reply with error by default */;

    /* Validate and find the group command */
    for(cmdCtr = 0; cmdCtr < nbParserCmd; cmdCtr ++)
    {
        parserCmdEntry = *(pParserCmd + cmdCtr);
        if(strcmp(parserCmdEntry.pCommand, (char*)&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx]]) == 0U)
        {
            /* Command found */
            break;
        }
    }

    if(cmdCtr < nbParserCmd)
    {
        if(parserCmdEntry.pNextParserCmd == NULL)
        {
            /* No other commands, just execute the callback */
            if(parserCmdEntry.pActionCbFct)
            {
                if((mRxParserCmd.crtWordIdx - rxCmdIdx) == parserCmdEntry.flags)
                {
                    uint8_t iCtr = rxCmdIdx + 1;
                    bool bInvalidParam = false;
                    if(parserCmdEntry.flags > 0)
                    {
                        do
                        {
                            //Make sure that the parameters are not empty
                            if(mRxParserCmd.wordLen[iCtr ++] == 0)
                            {
                                bInvalidParam = true;
                                break;
                            }
                        }while(iCtr <= mRxParserCmd.crtWordIdx);
                    }

                    if(bInvalidParam == false)
                    {
                        memset(&parserCmdInfo, 0, sizeof(parserCmdInfo_t));

                        if((rxCmdIdx + 1U < PARSER_DEF_CMD_MAX_IDX) && (mRxParserCmd.wordLen[rxCmdIdx + 1U] > 0U))
                        {
                            parserCmdInfo.pParam1 = (char*)(&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx + 1]]);
                        }

                        if((rxCmdIdx + 2U < PARSER_DEF_CMD_MAX_IDX) && (mRxParserCmd.wordLen[rxCmdIdx + 2U] > 0U))
                        {
                            parserCmdInfo.pParam2 = (char*)(&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx + 2]]);
                        }

                        if((rxCmdIdx + 3U < PARSER_DEF_CMD_MAX_IDX) && (mRxParserCmd.wordLen[rxCmdIdx + 3U] > 0U))
                        {
                            parserCmdInfo.pParam3 = (char*)(&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx + 3]]);
                        }

                        if((rxCmdIdx + 4U < PARSER_DEF_CMD_MAX_IDX) && (mRxParserCmd.wordLen[rxCmdIdx + 4U] > 0U))
                        {
                            parserCmdInfo.pParam4 = (char*)(&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx + 4]]);
                        }

                        if((rxCmdIdx + 5U < PARSER_DEF_CMD_MAX_IDX) && (mRxParserCmd.wordLen[rxCmdIdx + 5U] > 0U))
                        {
                            parserCmdInfo.pParam5 = (char*)(&mRxParserCmd.cmd[mRxParserCmd.wordStartPos[rxCmdIdx + 5]]);
                        }

                        /* Execute callback */
                        parserCmdEntry.pActionCbFct(&parserCmdInfo);
                    }
                }
            }
        }
        else
        {
            /* Additional parsing */
            retValue = 1U;
            *pSavedCmdIdx = cmdCtr;
            /* DO not send a reply yet */
            parserCmdInfo.pReplyCmd = NULL;
        }
    }

    if(parserCmdInfo.pReplyCmd)
    {
        Parser_TxAddReply(parserCmdInfo.pReplyCmd, strlen(parserCmdInfo.pReplyCmd));
    }

    return retValue;
}

