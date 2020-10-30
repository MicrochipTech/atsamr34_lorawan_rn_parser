/**
* \file  parser_tsp.c
*
* \brief Parser transport source file
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
#include "parser_tsp.h"
#include "parser.h"
#include "parser_lorawan.h"
#include "parser_private.h"
#include "parser_utils.h"
#include "sio2host.h"

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

volatile parserRxCmd_t mRxParserCmd;
static const char* gpParserLineDelim = {PARSER_END_LINE_DELIM_STRING};

static const char* gapParserTspStatus[] =
{
    "ok",
    "invalid_param",
    "err"
};

void Parser_RxClearBuffer(void)
{
    mRxParserCmd.bCmdStatus = 0;
    mRxParserCmd.crtWordIdx = 0;
    mRxParserCmd.crtCmdPos = 0;
    mRxParserCmd.crtWordPos = 0;
    
    memset((_PTR)mRxParserCmd.wordLen, 0, PARSER_DEF_CMD_MAX_IDX << 1);
    memset((_PTR)mRxParserCmd.wordStartPos, 0, PARSER_DEF_CMD_MAX_IDX << 1);
}

void Parser_RxAddChar(uint8_t rxChar)
{
    uint8_t retStatus = STATUS_DONE;
    bool bIsEndLine = false;
    uint8_t iCount;
    
    // Process special character: '\b'
    if(rxChar == '\b')
    {
        /* process delete character '\b' */
        if(mRxParserCmd.crtCmdPos > 0U)
        {
            // Check for ' '. This was previously replaced with '\0'
            //mRxParserCmd.crtCmdPos always indicates the first free position
            if(mRxParserCmd.cmd[mRxParserCmd.crtCmdPos - 1] == '\0')
            {
                mRxParserCmd.crtWordIdx --;
                mRxParserCmd.crtWordPos = mRxParserCmd.wordLen[mRxParserCmd.crtWordIdx];
                mRxParserCmd.wordLen[mRxParserCmd.crtWordIdx] = 0U;
                mRxParserCmd.wordStartPos[mRxParserCmd.crtWordIdx] = 0U;
            }
            else
            {
                mRxParserCmd.crtWordPos --;
            }

            mRxParserCmd.crtCmdPos --;
        }

        return;
    }

    // Regular command
    if(mRxParserCmd.crtCmdPos < PARSER_DEF_CMD_MAX_LEN - 1)
    {
        if(rxChar == ' ')
        {
            if(mRxParserCmd.crtWordIdx < PARSER_DEF_CMD_MAX_IDX - 1)
            {
                /* Command separator received */

                /* Replace ' ' with \0 */
                mRxParserCmd.cmd[mRxParserCmd.crtCmdPos ++] = '\0';

                /* Save last filled position in mRxParserCmd.wordLen[mRxParserCmd.crtWordIdx] */
                mRxParserCmd.wordLen[mRxParserCmd.crtWordIdx] = mRxParserCmd.crtWordPos;
                mRxParserCmd.wordStartPos[mRxParserCmd.crtWordIdx] = mRxParserCmd.crtCmdPos - mRxParserCmd.crtWordPos - 1;

                /* Prepare to receive next word */
                mRxParserCmd.crtWordIdx ++;
                mRxParserCmd.crtWordPos = 0;
            }
            else
            {
                retStatus = STATUS_ERROR;
            }
        }
        else
        {
            /* Save the character */
            mRxParserCmd.cmd[mRxParserCmd.crtCmdPos ++] = rxChar;
            mRxParserCmd.crtWordPos ++;

            if(mRxParserCmd.crtCmdPos >= strlen(gpParserLineDelim))
            {
                bIsEndLine = true;

                for(iCount = strlen(gpParserLineDelim); (iCount > 0U) && bIsEndLine; iCount --)
                {
					
                    if(mRxParserCmd.cmd[mRxParserCmd.crtCmdPos - iCount] != gpParserLineDelim[strlen(gpParserLineDelim) - iCount])
                    {
                        bIsEndLine = false;
                    }
					//printf("Comp %d ",bIsEndLine);
                }
            }
            if(bIsEndLine)
            {
                /* Entire command received */

                /* Replace new line with \0 */
                mRxParserCmd.cmd[mRxParserCmd.crtCmdPos - strlen(gpParserLineDelim)] = '\0';

                mRxParserCmd.wordLen[mRxParserCmd.crtWordIdx] = mRxParserCmd.crtWordPos - strlen(gpParserLineDelim);
                mRxParserCmd.wordStartPos[mRxParserCmd.crtWordIdx] = mRxParserCmd.crtCmdPos - mRxParserCmd.crtWordPos;

                mRxParserCmd.bCmdStatus = 1;
            }
        }
    }
    else
    {
        retStatus = STATUS_ERROR;
    }

    if(STATUS_ERROR == retStatus)
    {
        Parser_RxClearBuffer();
        /* Send reply code */
        Parser_TxAddReply((char*)gapParserTspStatus[ERR_STATUS_IDX], strlen(gapParserTspStatus[ERR_STATUS_IDX]));
    }

}

void Parser_TxAddReply(char* pReplyStr, uint16_t replyStrLen)
{
    uint16_t iCtr = replyStrLen;
	
	/* Check if the length of UART String is can be fit in SIO2HOST TX Buffer */
	while(0 != iCtr)
	{
		if(BYTE_VALUE_LEN >= iCtr)
		{
			sio2host_tx((uint8_t *)pReplyStr,(uint8_t)iCtr);
			iCtr = 0;
		}
		else
		{
			sio2host_tx((uint8_t *)pReplyStr, BYTE_VALUE_LEN);
			iCtr -= BYTE_VALUE_LEN;
			pReplyStr = pReplyStr + BYTE_VALUE_LEN;
		}
	}
	
    /* Put the delimiter string in UART */
	sio2host_tx((uint8_t *)gpParserLineDelim,strlen(gpParserLineDelim));
	
}
