/**
* \file  parser_lorawan.c
*
* \brief LoRaWAN command parser
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
 
#include "stdlib.h"
#include "string.h"
#include "parser.h"
#include "parser_lorawan.h"
#include "parser_private.h"
#include "parser_tsp.h"
#include "parser_utils.h"
#include "lorawan.h"
#include "sys.h"
#include "pds_interface.h"

#define JOIN_DENY_STR_IDX				0U
#define JOIN_ACCEPT_STR_IDX				1U
#define JOIN_NO_FREE_CHANNEL_STR_IDX	2U
#define JOIN_TX_TIMEOUT_STR_IDX			3U
#define JOIN_MIC_ERROR_STR_IDX			4U

#define OTAA_STR_IDX        0U
#define ABP_STR_IDX         1U

#define CNF_STR_IDX         0U
#define UNCNF_STR_IDX       1U

#define RADIO_TX_OK_STR_IDX 0U
#define MAC_TX_OK_STR_IDX   1U
#define RADIO_RX_DATA_STR_IDX 2U
#define MAC_RX_DATA_STR_IDX 3U
#define RADIO_ERR_STR_IDX   4U
#define MAC_ERR_STR_IDX     5U
#define MAC_INVALID_LEN_STR_IDX     6U
#define MIC_ERROR_STR_IDX 7u
#define INVALID_FCNTR_STR_IDX 8u
#define INVALID_MTYPE_STR_IDX 9u
#define MCAST_HDR_INVALID_STR_IDX 10u
#define NO_ACK_STR_IDX 11u
#define RADIO_NO_DATA_STR_IDX 1u
#define RADIO_DATA_SIZE_STR_IDX 2u
#define RADIO_INVALID_REQ_STR_IDX 3u
#define LORAWAN_RADIO_BUSY_STR_IDX 4u
#define RADIO_OUT_OF_RANGE_STR_IDX 5u
#define RADIO_UNSUP_ATTR_STR_IDX 6u
#define RADIO_CHANNEL_BUSY_STR_IDX  7u
#define MAC_ACK_RXED_STR_IDX 12u
#define INVALID_BUFFER_LEN_STR_IDX 13u

#define RADIO_RX_CHANNEL_BUSY_IDX   8U
#define RADIO_TX_CHANNEL_BUSY_IDX   4U

#define MAC_NOT_OK_STR_IDX   2U
#define RADIO_BUSY_STR_IDX   3U

#define OFF_STR_IDX								0U
#define ON_STR_IDX								1U
#define PARSER_LORA_MAC_IFC						0U

#define NOT_JOINED_STR_IDX						9u
#define INVALID_PARAM_STR_IDX					10u
#define KEYS_NOT_INIT_STR_IDX					11u
#define SILENT_STR_IDX							12u
#define FRAME_CNTR_ERR_REJOIN_NEEDED_STR_IDX	13u
#define INVALID_DATA_LEN_STR_IDX				14u
#define MAC_PAUSED_STR_IDX						15u
#define NO_FREE_CHANNEL_STR_IDX					16u
#define BUSY_STR_IDX							17u
#define TRN_NO_ACK_STR_IDX                      18u
#define JOIN_IN_PROGRESS_STR_IDX				19u
#define RESOURCE_UNAVAILABLE_STR_IDX			20u
#define INVALID_REQ_STR_IDX			    	    21u
#define RADIO_TX_TIMEOUT_IDX					24u
#define TX_TIMEOUT_IDX							23u
#define INVALID_PACKET_STR_IDX					25u

parserConfiguredJoinParameters_t gParserConfiguredJoinParameters;

LorawanSendReq_t parser_data;

static void ParserAppData(void *appHandle, appCbParams_t *data);
static void ParserJoinData(StackRetStatus_t status);

static const char* gapParseJoinMode[] =
{
    "otaa",
    "abp"
};

static const char* gapParseJoinStatus[] =
{
    "denied",	
    "accepted",
	"no_free_ch",
	"tx_timeout",
	"mic_error"
};

static const char* gapParserSendMode[] =
{
    "cnf",
    "uncnf"
};

static const char* gapParserRxStatus[] =
{
    "radio_tx_ok", 
    "mac_tx_ok",
    "radio_rx ",
    "mac_rx ",
    "radio_err",
    "mac_err",
    "invalid_data_len",
	"mic_error",
	"invalid_fcntr",
	"invalid_mtype",
	"mcast_hdr_invalid",
	"no_ack ",
	"ack_received",
	"invalid_buffer_length"
};

static const char* gapParserTxStatus[] =
{
	"radio_tx_ok",
	"mac_tx_ok",
	"mac_not_ok",
	"busy",
	"radio_channel_busy"
};

static const char* gapParseOnOff[] =
{
	"off",
    "on"
};

static const char* gapParseIsmBand[] =
{
    "868",//ISM_EU868
    "433",//ISM_EU433
    "na915",
	"au915",
	"kr920",
	"jpn923",
	"brn923",
	"cmb923",
	"ins923",
	"laos923",
	"nz923",
	"sp923",
	"twn923",
	"thai923",
	"vtm923",
	"ind865"
};

static const char* gapParserLorawanStatus[] =
{   
	"radio_ok",
	"radio_no_data",
	"radio_data_size",
	"radio_invalid_req",
	"radio_busy",
	"radio_out_of_range",
	"radio_unsup_attr",
	"radio_channel_busy",
    "ok",
    "not_joined",                                           
    "invalid_param",
    "keys_not_init",	
    "silent",
    "fram_counter_err_rejoin_needed",
    "invalid_data_len",
    "mac_paused",
    "no_free_ch",
	"busy",
	"no_ack",
	"join_in_progress",
	"resource_unavailable",
	"invalid_request",
	"unsupported_band",
	"tx_timeout",
	"radio_tx_timeout",
	"invalid_packet"
};

static const char* gapParserEdClass[] =
{
	"CLASS A",
	"CLASS B",
	"CLASS C",
	"CLASS INVALID"
};

static const char* gapParserBool[] = 
{
	"false",
	"true"
};

uint8_t Parser_GetConfiguredJoinParameters()
{
    return gParserConfiguredJoinParameters.value;
}

void Parser_SetConfiguredJoinParameters(uint8_t val)
{
    gParserConfiguredJoinParameters.value = val;
}

void Parser_LorawanInit(void)
{
    LORAWAN_Init(ParserAppData, ParserJoinData);
    gParserConfiguredJoinParameters.value = 0x00;
}

void Parser_LoraReset (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t iCount;
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    
    for(iCount = 0; iCount < sizeof(gapParseIsmBand)/sizeof(gapParseIsmBand[0]); iCount++)
    {

	    if(0 == stricmp(pParserCmdInfo->pParam1, gapParseIsmBand[iCount]))
	    {
			uint16_t supportedBands = 0;
			
			LORAWAN_GetAttr(SUPPORTED_BANDS,NULL,&supportedBands);
			if(((1 << iCount) & supportedBands) == 0)
			{
				printf("Band %s not supported\r\n",gapParseIsmBand[iCount]);
				status = LORAWAN_UNSUPPORTED_BAND;
				break;
			}

#if (ENABLE_PDS == 1)			
			if(PDS_IsRestorable())
			{
				uint8_t prevBand = 0xFF;
				int8_t isSwitchReq = false;
				PDS_RestoreAll();
				LORAWAN_GetAttr(ISMBAND,NULL,&prevBand);
				if(prevBand != iCount)
				{
					PDS_DeleteAll();
					isSwitchReq = true;
				}
				status = LORAWAN_Reset(iCount);
				
				if(isSwitchReq == true && status == LORAWAN_SUCCESS)
				{
					PDS_StoreAll();
				}
				else
				{
					PDS_RestoreAll();
				}
			}
			else
			{
				status = LORAWAN_Reset(iCount);
				if(status == LORAWAN_SUCCESS)
				{
				    PDS_StoreAll();					
				}
			}
#else
		    status = LORAWAN_Reset(iCount);
#endif
		    gParserConfiguredJoinParameters.value = 0x00;		    
		    break;
	    }
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraJoin (parserCmdInfo_t* pParserCmdInfo)
{
   
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t validationVal;

    //Parameter validation
    validationVal = Validate_Str1Str2AsciiValue(pParserCmdInfo->pParam1, gapParseJoinMode[OTAA_STR_IDX], gapParseJoinMode[ABP_STR_IDX]);

    if(validationVal < 2U)
    {
        status = LORAWAN_Join(validationVal);
    }

    // A status is returned immediately after the command is executed.
    // An asynchronous reply with the status will be displayed once the join process is finished.
    // While the answers is expected other commands may be received
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraSend (parserCmdInfo_t* pParserCmdInfo)
{    
    uint8_t portValue;
    uint16_t asciiDataLen = strlen(pParserCmdInfo->pParam3);
    uint16_t  dataLen = asciiDataLen >> 1;
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t validationVal;

    validationVal = Validate_Str1Str2AsciiValue(pParserCmdInfo->pParam1, gapParserSendMode[UNCNF_STR_IDX], gapParserSendMode[CNF_STR_IDX]);

    // Parameter validation
    // MacSendIfc function expects a buffer length of max. 255 bytes. Check dataLen (uint16_t) to be less than 255 in order to avoid overflow 
    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam2, &portValue) && (dataLen <= 255) &&
       (validationVal < 2U) && Validate_HexValue(pParserCmdInfo->pParam3))
    {
        Parser_HexAsciiToInt(asciiDataLen, pParserCmdInfo->pParam3, (uint8_t *)aParserData);

        if(asciiDataLen % 2 == 1)
        {
            //Odd number of characters, an extra '0' character was added to the payload
            dataLen += 1;
        }
         
        parser_data.confirmed = validationVal;
        parser_data.port = portValue;
        parser_data.buffer = aParserData;
        parser_data.bufferLength = (uint8_t)dataLen;
		 
        status = LORAWAN_Send(&parser_data);
    }

    // A status is returned immediately after the command is executed.
    // An asynchronous reply with the possible data and tx status will be displayed once the tx process is finished.
    // While the anwers is expected other commands may be received
    //if (status != LORAWAN_SUCCESS)
    {
        pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
    }
}

void Parser_LoraSetCrtDataRate (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t datarate;

    // Parameter validation
    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &datarate))
    {
        status = LORAWAN_SetAttr(CURRENT_DATARATE,&datarate);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetCrtDataRate (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t crtDatarate;

	LORAWAN_GetAttr(CURRENT_DATARATE,NULL,&crtDatarate);
	
 	itoa(crtDatarate, aParserData, 10U);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetAdr (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t validationVal;

    validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam1);
    if(validationVal < 2U)
    {
        status = LORAWAN_SetAttr(ADR,&validationVal);
         
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetAdr (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t adrEnabled;

    LORAWAN_GetAttr(ADR,NULL,&adrEnabled);

    pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[adrEnabled];
}

void Parser_LoraSetDevAddr(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t tempBuff[DEV_ADDR_LEN];
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;
    uint32_t devAddr;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(sizeof(devAddr) << 1, pParserCmdInfo->pParam1, tempBuff))
        {
            devAddr = (uint32_t)tempBuff[3];
            devAddr += ((uint32_t)tempBuff[2]) << 8;
            devAddr += ((uint32_t)tempBuff[1]) << 16;
            devAddr += ((uint32_t)tempBuff[0]) << 24;

            statusIdx = LORAWAN_SetAttr (DEV_ADDR,&devAddr);
            gParserConfiguredJoinParameters.flags.devaddr = 1;

             
       }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetDevAddr(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t devAddr;
    uint8_t tempBuff[DEV_ADDR_LEN];

    LORAWAN_GetAttr(DEV_ADDR, NULL, &devAddr);

    tempBuff[3] = (uint8_t)devAddr;
    tempBuff[2] = (uint8_t)(devAddr >> 8);
    tempBuff[1] = (uint8_t)(devAddr >> 16);
    tempBuff[0] = (uint8_t)(devAddr >> 24);

    Parser_IntArrayToHexAscii(4, tempBuff, aParserData);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetDevEui(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(16, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            statusIdx = LORAWAN_SetAttr (DEV_EUI,aParserData);
            gParserConfiguredJoinParameters.flags.deveui = 1;
            
        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetDevEui(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t aDevEui[DEV_EUI_LEN];

    LORAWAN_GetAttr (DEV_EUI, NULL, aDevEui);
	Parser_IntArrayToHexAscii(DEV_EUI_LEN, aDevEui, aParserData);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetDevEuiArray(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;
    uint8_t aux;
    uint8_t i;
    
    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(16, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            for(i = 0; i < 4; i++)
            {
                aux = aParserData[i];
                aParserData[i] = aParserData[7 - i];
                aParserData[7 - i] = aux;
            }
            
            statusIdx = LORAWAN_SetAttr(DEV_EUI, aParserData);
            gParserConfiguredJoinParameters.flags.deveui = 1;           
        }
    }
    
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetDevEuiArray(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t aDevEui[DEV_EUI_LEN];
    uint8_t i;
    uint8_t aux;
    
    LORAWAN_GetAttr (DEV_EUI, NULL, aDevEui);
    
    for (i = 0; i < 4; i++)
    {
        aux = aDevEui[i];
        aDevEui[i] = aDevEui[7 - i];
        aDevEui[7 - i] = aux;
    }
    
    Parser_IntArrayToHexAscii(DEV_EUI_LEN, aDevEui, aParserData);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetJoinEui(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(16, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            statusIdx = LORAWAN_SetAttr(JOIN_EUI,aParserData);
            gParserConfiguredJoinParameters.flags.joineui = 1;          
        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}


void Parser_LoraGetJoinEui(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t aJoinEui[JOIN_EUI_LEN];

    LORAWAN_GetAttr(JOIN_EUI, NULL, aJoinEui);
	Parser_IntArrayToHexAscii(JOIN_EUI_LEN, aJoinEui, aParserData);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetJoinEuiArray(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;
    uint8_t aux;
    uint8_t i;
    
    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(16, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            for(i = 0; i < 4; i++)
            {
                aux = aParserData[i];
                aParserData[i] = aParserData[7 - i];
                aParserData[7 - i] = aux;
            }
            
            statusIdx = LORAWAN_SetAttr(JOIN_EUI, aParserData);
            gParserConfiguredJoinParameters.flags.joineui = 1;
        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetJoinEuiArray(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t aJoinEui[JOIN_EUI_LEN];
    uint8_t i;
    uint8_t aux;

    LORAWAN_GetAttr(JOIN_EUI, NULL, aJoinEui);
    
    for (i = 0; i < 4; i++)
    {
        aux = aJoinEui[i];
        aJoinEui[i] = aJoinEui[7 - i];
        aJoinEui[7 - i] = aux;
    }
    
    Parser_IntArrayToHexAscii(JOIN_EUI_LEN, aJoinEui, aParserData);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetNwkSKey(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(32U, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            statusIdx = LORAWAN_SetAttr(NWKS_KEY,aParserData);
            gParserConfiguredJoinParameters.flags.nwkskey = 1;
			
        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraSetAppSKey(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(32U, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
            statusIdx = LORAWAN_SetAttr(APPS_KEY, aParserData);
            gParserConfiguredJoinParameters.flags.appskey = 1;

        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraSetAppKey(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t statusIdx = LORAWAN_INVALID_PARAMETER;

    if(Validate_HexValue(pParserCmdInfo->pParam1))
    {
        if(Parser_HexAsciiToInt(32U, pParserCmdInfo->pParam1, (uint8_t *)aParserData))
        {
             statusIdx = LORAWAN_SetAttr(APP_KEY,aParserData);
            gParserConfiguredJoinParameters.flags.appkey = 1;
        }
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraSetChannelFreq (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t channelId;
    unsigned long freq = strtoul(pParserCmdInfo->pParam2, NULL, 10);
    ChannelParameters_t ch_params = {0};
    
    // The frequency is 10 ascii characters long
    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId) &&
       Validate_UintDecAsciiValue(pParserCmdInfo->pParam2, 10, UINT32_MAX))
    {
        ch_params.channelId = channelId;
        ch_params.channelAttr.frequency = (uint32_t)freq;
        status = LORAWAN_SetAttr(CH_PARAM_FREQUENCY,&ch_params);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetChannelFreq(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t freq;
    uint8_t channelId;
	StackRetStatus_t status;
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_INVALID_PARAMETER];

    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId))
    {
        status = LORAWAN_GetAttr(CH_PARAM_FREQUENCY,&channelId,&freq);
		
		if(status == LORAWAN_SUCCESS)
		{
			ultoa(aParserData, freq, 10U);
			pParserCmdInfo->pReplyCmd = aParserData;	
		}        
    }
}

void Parser_LoraSetSubBandStatus (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t subBandId;
    uint8_t i;
    
    uint8_t validationVal;
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
	
	ChannelParameters_t ch_params;
    
    validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam2);
    
    if (Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &subBandId) && 
            (validationVal < 2U) && 
            (subBandId >= 1) && 
            (subBandId <= 8)) 
    {
           for (i = ((subBandId - 1) * 8); i <= ((subBandId * 8) - 1); i++)
           {
			   ch_params.channelId = i;
			   ch_params.channelAttr.status = validationVal;
               status = LORAWAN_SetAttr(CH_PARAM_STATUS, &ch_params);
           }
               ch_params.channelId = (63 + subBandId);
               ch_params.channelAttr.status = validationVal;
               status = LORAWAN_SetAttr(CH_PARAM_STATUS, &ch_params);
           
           /*
            * The return status is not verified since we presume that at the call time the channel id si correct
            */
    }
     
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetSubBandStatus (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t subBandId;
    uint8_t i;
    uint8_t chStatus;
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_INVALID_PARAMETER];
    
    if (Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &subBandId) && 
            (subBandId >= 1) && 
            (subBandId <= 8))
    {
        for (i = ((subBandId - 1) * 8); i <= ((subBandId * 8) - 1); i++)
        {
             if(LORAWAN_GetAttr(CH_PARAM_STATUS, &subBandId, &chStatus) == LORAWAN_SUCCESS)
			 {
				pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[chStatus];
			 }
        }
             uint8_t chId = 63 + subBandId;
             if(LORAWAN_GetAttr(CH_PARAM_STATUS, &chId, &chStatus) == LORAWAN_SUCCESS)
			 {
				pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[chStatus];
			 }
    }
}

void Parser_LoraSetChannelStatus (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t channelId;
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t validationVal;
    ChannelParameters_t ch_params = {0};
    validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam2);

    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId) &&(validationVal < 2U))
    {
        ch_params.channelId = channelId;
        ch_params.channelAttr.status = validationVal;
        status = LORAWAN_SetAttr(CH_PARAM_STATUS,&ch_params);
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetChannelStatus (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t chStatus;
    uint8_t channelId;	
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_INVALID_PARAMETER];

    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId))
    {
        if(LORAWAN_GetAttr(CH_PARAM_STATUS,&channelId,&chStatus) == LORAWAN_SUCCESS)
		{
			pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[chStatus];
		}
    }
}

void Parser_LoraSetDatarateRange (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t channelId;
    uint8_t minDr;
    uint8_t maxDr;
    ChannelParameters_t ch_params = {0};

    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId) &&
       Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam2, &minDr) &&
       Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam3, &maxDr) &&
       minDr < 16 && maxDr < 16)
    {
        ch_params.channelId = channelId;
        ch_params.channelAttr.dataRange = ((maxDr << 4) | minDr);

        status = LORAWAN_SetAttr(CH_PARAM_DR_RANGE, &ch_params);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetDatarateRange (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t channelId;
    uint8_t drRange;
    uint8_t minDr;
    uint8_t maxDr;
    uint16_t crtIdx = 0;;

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_INVALID_PARAMETER];


    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &channelId))
    {
        if(LORAWAN_GetAttr(CH_PARAM_DR_RANGE,&channelId,&drRange) == LORAWAN_SUCCESS)
		{		
			minDr = drRange & 0x0F;
			maxDr = (drRange >> 4) & 0x0F;

			itoa(minDr, aParserData, 10U);
			crtIdx = strlen(aParserData);
			aParserData[crtIdx] = ' ';
			crtIdx ++;
			itoa(maxDr, &aParserData[crtIdx], 10U);
			pParserCmdInfo->pReplyCmd = aParserData;
		}
    }
}

void Parser_LoraSetTxPower (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t txPowerIdx;
    uint8_t ismBand;
    
    LORAWAN_GetAttr(ISMBAND, NULL, &ismBand);

    if((Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &txPowerIdx)))
    {
        status = LORAWAN_SetAttr(TX_POWER,&txPowerIdx);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetTxPower (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t txPowerIdx;

    LORAWAN_GetAttr(TX_POWER,NULL,&txPowerIdx);

    itoa( txPowerIdx, aParserData, 10);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraPause (parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t resumeInterval;

    resumeInterval = LORAWAN_Pause();

    ultoa(aParserData, resumeInterval, 10);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraResume (parserCmdInfo_t* pParserCmdInfo)
{
    LORAWAN_Resume();
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_SUCCESS];
}

void Parser_LoraSave (parserCmdInfo_t* pParserCmdInfo)
{
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_SUCCESS];
}

//void MacGetReceiveWindow2ParametersIfc (uint8_t ifc, bool ismBandDual, uint32_t* frequency, uint8_t* dataRate)
void Parser_LoraSetRx2WindowParams (parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    unsigned long freq = strtoul(pParserCmdInfo->pParam2, NULL, 10);
    uint8_t datarate;
    ReceiveWindow2Params_t rx2Params;
    // Parameter validation
    // The frequency is 10 ascii characters long
    if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &datarate) &&
       Validate_UintDecAsciiValue(pParserCmdInfo->pParam2, 10, UINT32_MAX))
    {
        rx2Params.dataRate = datarate;
        rx2Params.frequency = freq;
        status = LORAWAN_SetAttr(RX2_WINDOW_PARAMS,&rx2Params);
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetRx2WindowParams(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t freq = 0;
    uint8_t datarate = 0xFF;
    uint8_t dataLen;
    ReceiveWindow2Params_t rx2Params;
     pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_INVALID_PARAMETER];

    //Return value of the function not checked but default values for freq and datarate are invalid
    if(LORAWAN_GetAttr(RX2_WINDOW_PARAMS, NULL, &rx2Params) == LORAWAN_SUCCESS)
	{
		datarate = rx2Params.dataRate;
		freq = rx2Params.frequency;
		ultoa(aParserData, datarate, 10U);
		dataLen = strlen(aParserData);
		aParserData[dataLen ++] = ' ';
		ultoa(&aParserData[dataLen], freq, 10U);
		pParserCmdInfo->pReplyCmd = aParserData;
	}
    
}

void Parser_LoraSetLbt (parserCmdInfo_t* pParserCmdInfo)
{
	StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
	LorawanLBTParams_t lorawanLBTParams;
	lorawanLBTParams.lbtScanPeriod = atoi(pParserCmdInfo->pParam1);
	lorawanLBTParams.lbtThreshold = atoi(pParserCmdInfo->pParam2);
	lorawanLBTParams.maxRetryChannels = atoi(pParserCmdInfo->pParam3);
	lorawanLBTParams.lbtNumOfSamples = atoi(pParserCmdInfo->pParam4);
	lorawanLBTParams.lbtTransmitOn = atoi(pParserCmdInfo->pParam5);
	status = LORAWAN_SetAttr(LORAWAN_LBT_PARAMS, &lorawanLBTParams);
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetLbt (parserCmdInfo_t* pParserCmdInfo)
{
	StackRetStatus_t status;
	uint8_t dataLen;
	LorawanLBTParams_t lorawanLBTParams;
	status = LORAWAN_GetAttr(LORAWAN_LBT_PARAMS, NULL, &lorawanLBTParams);
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
	if (LORAWAN_SUCCESS == status)
	{
		ultoa(aParserData, lorawanLBTParams.lbtScanPeriod, 10U);
        dataLen = strlen(aParserData);
        aParserData[dataLen ++] = ' ';
		itoa(lorawanLBTParams.lbtThreshold, &aParserData[dataLen], 10U);
		dataLen = strlen(aParserData);
        aParserData[dataLen ++] = ' ';
        ultoa(&aParserData[dataLen], lorawanLBTParams.maxRetryChannels, 10U);
		dataLen = strlen(aParserData);
        aParserData[dataLen ++] = ' ';
        ultoa(&aParserData[dataLen], lorawanLBTParams.lbtNumOfSamples, 10U);
		dataLen = strlen(aParserData);
        aParserData[dataLen ++] = ' ';
        ultoa(&aParserData[dataLen], lorawanLBTParams.lbtTransmitOn, 10U);
        pParserCmdInfo->pReplyCmd = aParserData;
	}
}

void Parser_LoraForceEnable (parserCmdInfo_t* pParserCmdInfo)
{
    LORAWAN_ForceEnable();
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[LORAWAN_SUCCESS];
}


void Parser_LoraSetUplinkCounter(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t param1Value = (uint32_t)strtoul(pParserCmdInfo->pParam1, NULL, 10U);
	StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if (Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 10, UINT32_MAX))
    {
        status = LORAWAN_SetAttr(UPLINK_COUNTER, &param1Value);       
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraSetDownlinkCounter(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t param1Value = (uint32_t)strtoul(pParserCmdInfo->pParam1, NULL, 10U);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
	
    if (Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 10, UINT32_MAX))
    {
        status = LORAWAN_SetAttr(DOWNLINK_COUNTER, &param1Value);   
    }
     pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}


void Parser_LoraSetSyncWord(parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t asciiDataLen = strlen(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if (Validate_HexValue(pParserCmdInfo->pParam1) && (2 == strlen(pParserCmdInfo->pParam1)))
    {
        Parser_HexAsciiToInt(asciiDataLen, pParserCmdInfo->pParam1, (uint8_t *)aParserData);
        status = LORAWAN_SetAttr(SYNC_WORD,aParserData);    
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetSyncWord(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t syncWord;

    LORAWAN_GetAttr(SYNC_WORD, NULL, &syncWord);
    Parser_IntArrayToHexAscii(1, &syncWord, aParserData);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetUplinkCounter(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t ctr;

    LORAWAN_GetAttr(UPLINK_COUNTER, NULL, &ctr);
    ultoa(aParserData, ctr, 10U);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetDownlinkCounter(parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t ctr;

    LORAWAN_GetAttr(DOWNLINK_COUNTER, NULL, &ctr);
    ultoa(aParserData, ctr, 10U);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraLinkCheck (parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t period = strtoul(pParserCmdInfo->pParam1, NULL, 10);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 5, UINT16_MAX))
    {
        status = LORAWAN_SetAttr(LINK_CHECK_PERIOD,&period);      
    }
    
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetAggregatedDutyCycle (parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t aggregatedDutyCycle;

    LORAWAN_GetAttr(AGGREGATED_DUTYCYCLE, NULL, &aggregatedDutyCycle);
    utoa(aggregatedDutyCycle, aParserData,  10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetAggregatedDutyCycle (parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t aggregatedDutyCycle = atoi(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;


    if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 2, UINT8_MAX))
    {
		status = LORAWAN_SetAttr(AGGREGATED_DUTYCYCLE,&aggregatedDutyCycle);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
		
}



void Parser_LoraSetCryptoDevEnabled (parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t validationVal;
	uint8_t returnVal = LORAWAN_INVALID_PARAMETER;

	validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam1);
	if (validationVal < 2U)
	{
		returnVal = LORAWAN_SetAttr(CRYPTODEVICE_ENABLED, &validationVal);
	}
	
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[returnVal];
	
}

void Parser_LoraGetMacStatus (parserCmdInfo_t* pParserCmdInfo)
{
    uint32_t macStatusMask;
    uint8_t tempBuff[4];

    LORAWAN_GetAttr(LORAWAN_STATUS,NULL, &macStatusMask);

    tempBuff[3] = (uint8_t)macStatusMask;
    tempBuff[2] = (uint8_t)(macStatusMask >> 8);
    tempBuff[1] = (uint8_t)(macStatusMask >> 16);
    tempBuff[0] = (uint8_t)(macStatusMask >> 24);    

    Parser_IntArrayToHexAscii(4, tempBuff, aParserData);
    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetBatLevel (parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t batLevel = atoi(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 3, UINT8_MAX))
    {
        status = LORAWAN_SetAttr(BATTERY, &batLevel);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraSetReTxNb(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t reTxNb = atoi(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 3, UINT8_MAX))
    {
        status = LORAWAN_SetAttr(CNF_RETRANSMISSION_NUM,&reTxNb);
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraSetRepsNb(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t reTxNb = atoi(pParserCmdInfo->pParam1);
	StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

	if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 3, UINT8_MAX))
	{
		status = LORAWAN_SetAttr(UNCNF_REPETITION_NUM,&reTxNb);
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetReTxNb(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t reTxNb;

    LORAWAN_GetAttr(CNF_RETRANSMISSION_NUM, NULL, &reTxNb);
    utoa(reTxNb, aParserData, 10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetRepsNb(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t reTxNb;

	LORAWAN_GetAttr(UNCNF_REPETITION_NUM, NULL, &reTxNb);
	utoa(reTxNb, aParserData, 10);

	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetLinkCheckMargin(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t mrgn;

    LORAWAN_GetAttr(LINK_CHECK_MARGIN, NULL, &mrgn);
    utoa(mrgn, aParserData,  10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetLinkCheckGwCnt(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t gwCnt;

    LORAWAN_GetAttr(LINK_CHECK_GWCNT, NULL, &gwCnt);
    utoa(gwCnt, aParserData, 10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetAutoReply(parserCmdInfo_t* pParserCmdInfo)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    uint8_t validationVal;

    validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam1);
    if(validationVal < 2U)
    {
        status = LORAWAN_SetAttr(AUTOREPLY, &validationVal);      
    }
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetAutoReply(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t arEnabled;

    LORAWAN_GetAttr(AUTOREPLY, NULL, &arEnabled);

    pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[arEnabled];
}

void Parser_LoraSetRxDelay1(parserCmdInfo_t* pParserCmdInfo)
{
    //Delay1 in ms
    uint16_t rxDelay1 = atoi(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 5, UINT16_MAX))
    {
        status = LORAWAN_SetAttr(RX_DELAY1,&rxDelay1);        
    }

    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetRxDelay1(parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t rxDelay1;

    LORAWAN_GetAttr(RX_DELAY1,NULL,&rxDelay1);
    utoa(rxDelay1, aParserData, 10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetRxDelay2(parserCmdInfo_t* pParserCmdInfo)
{
    uint16_t rxDelay2;

    LORAWAN_GetAttr(RX_DELAY2, NULL, &rxDelay2);
    utoa(rxDelay2, aParserData, 10);

    pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraGetIsm(parserCmdInfo_t* pParserCmdInfo)
{
    uint8_t ismBand;

    LORAWAN_GetAttr(ISMBAND, NULL, &ismBand);
    pParserCmdInfo->pReplyCmd = (char*)gapParseIsmBand[ismBand];
}

void Parser_LoraSetClass(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	uint8_t edClass;

	if ((pParserCmdInfo->pParam1[0] == 'A') || (pParserCmdInfo->pParam1[0] == 'a'))
	{
		edClass = CLASS_A;
		statusIdx = LORAWAN_SetAttr(EDCLASS, &edClass);
	}
	else if ((pParserCmdInfo->pParam1[0] == 'C') || (pParserCmdInfo->pParam1[0] == 'c'))
	{
		edClass = CLASS_C;
		statusIdx = LORAWAN_SetAttr(EDCLASS, &edClass);
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetClass(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t edClass;
	uint8_t index;
	LORAWAN_GetAttr(EDCLASS, NULL, &edClass);
	switch (edClass)
	{
		case CLASS_A:
			index = 0;
		break;
		
		case CLASS_B:
			index = 1;
		break;
		
		case CLASS_C:
			index = 2;
		break;
		
		default:
			index = 3;
		break;
		
	}

	pParserCmdInfo->pReplyCmd = (char *)gapParserEdClass[index];
}

void Parser_LoraGetSupportedEdClass(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t edClassSupported;
	LORAWAN_GetAttr(EDCLASS_SUPPORTED, NULL, &edClassSupported);
	if ((CLASS_A | CLASS_B) == edClassSupported)
	{
		aParserData[0] = 'A';
		aParserData[1] = '&';
		aParserData[2] = 'B';
		aParserData[3] = '\0';
	}
	else if  ((CLASS_A | CLASS_C) == edClassSupported)
	{
		aParserData[0] = 'A';
		aParserData[1] = '&';
		aParserData[2] = 'C';
	    aParserData[3] = '\0';
		
	}
	else
	{
		aParserData[0] = 'A';
		aParserData[1] = '\0';
	}
 	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetMcast(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t validationVal;
	uint8_t returnVal = LORAWAN_INVALID_PARAMETER;
	LorawanMcastStatus_t mcastStatus;

	validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam1);
	if (validationVal < 2U)
	{
		mcastStatus.status = validationVal;
		mcastStatus.groupId = atoi(pParserCmdInfo->pParam2);
		returnVal = LORAWAN_SetMulticastParam(MCAST_ENABLE, &mcastStatus);
	}
	
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[returnVal];
}

void Parser_LoraGetMcast(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t mcastStatus ;
	StackRetStatus_t status;
	uint8_t groupId = atoi(pParserCmdInfo->pParam1);

	status = LORAWAN_GetAttr(MCAST_ENABLE, &groupId, &mcastStatus);
	if(status == LORAWAN_SUCCESS)
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[mcastStatus];
	}
	else
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
	}
}


void Parser_LoraGetMcastDownCounter(parserCmdInfo_t* pParserCmdInfo)
{
	uint32_t ctr;
	StackRetStatus_t status;
	uint8_t groupId = atoi(pParserCmdInfo->pParam1);
	status = LORAWAN_GetAttr(MCAST_FCNT_DOWN, &groupId, &ctr);
	ultoa(aParserData, ctr, 10U);
	if(status == LORAWAN_SUCCESS)
	{
		pParserCmdInfo->pReplyCmd = aParserData;
	}
	else
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
	}

}

void Parser_LoraSetMcastDevAddr(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t tempBuff[4];
	int8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	uint32_t devMultiAddr;
	LorawanMcastDevAddr_t addr;
	if(Validate_HexValue(pParserCmdInfo->pParam1))
	{
		if(Parser_HexAsciiToInt(sizeof(devMultiAddr) << 1, pParserCmdInfo->pParam1, tempBuff))
		{
			devMultiAddr = (uint32_t)tempBuff[3];
			devMultiAddr += ((uint32_t)tempBuff[2]) << 8;
			devMultiAddr += ((uint32_t)tempBuff[1]) << 16;
			devMultiAddr += ((uint32_t)tempBuff[0]) << 24;
			addr.mcast_dev_addr = devMultiAddr;
			addr.groupId = atoi(pParserCmdInfo->pParam2);
			statusIdx = LORAWAN_SetMulticastParam(MCAST_GROUP_ADDR, &addr);
			gParserConfiguredJoinParameters.flags.mcastdevaddr = 1;
		}
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetMcastDevAddr(parserCmdInfo_t* pParserCmdInfo)
{
	uint32_t devMultiAddr;
	uint8_t tempBuff[4];
	StackRetStatus_t status;
	uint8_t groupId = atoi(pParserCmdInfo->pParam1);
	status = LORAWAN_GetAttr(MCAST_GROUP_ADDR, &groupId, &devMultiAddr);

	tempBuff[3] = (uint8_t)devMultiAddr;
	tempBuff[2] = (uint8_t)(devMultiAddr >> 8);
	tempBuff[1] = (uint8_t)(devMultiAddr >> 16);
	tempBuff[0] = (uint8_t)(devMultiAddr >> 24);
	
	if (status == LORAWAN_SUCCESS)
	{
		Parser_IntArrayToHexAscii(4, tempBuff, aParserData);
		pParserCmdInfo->pReplyCmd = aParserData;
	}
	else
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
	}	
}

void Parser_LoraSetMcastNwksKey(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	LorawanMcastNwkSkey_t key;
	if(Validate_HexValue(pParserCmdInfo->pParam1))
	{
		if(Parser_HexAsciiToInt(32U, pParserCmdInfo->pParam1, (uint8_t *)(key.mcastNwkSKey)))
		{
			key.groupId = atoi(pParserCmdInfo->pParam2);
			statusIdx = LORAWAN_SetMulticastParam(MCAST_NWKS_KEY, &key);
			gParserConfiguredJoinParameters.flags.mcastnwkskey = 1;
		}
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraSetMcastAppsKey(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	LorawanMcastNwkSkey_t key;
	if(Validate_HexValue(pParserCmdInfo->pParam1))
	{
		if(Parser_HexAsciiToInt(32U, pParserCmdInfo->pParam1,(uint8_t *)(key.mcastNwkSKey)))
		{
			key.groupId = atoi(pParserCmdInfo->pParam2);			
			statusIdx = LORAWAN_SetMulticastParam(MCAST_APPS_KEY, &key);
			gParserConfiguredJoinParameters.flags.mcastappskey = 1;
		}
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraSetMcastFreq(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	LorawanMcastDlFreqeuncy_t key;
	key.dlFrequency = strtoul(pParserCmdInfo->pParam1, NULL, 10);
	  // The frequency is 10 ascii characters long
	  if(Validate_UintDecAsciiValue(pParserCmdInfo->pParam1, 10, UINT32_MAX))
	  {
		  key.groupId = atoi(pParserCmdInfo->pParam2);
		  statusIdx = LORAWAN_SetMulticastParam(MCAST_FREQUENCY,(void*)&key);
		  gParserConfiguredJoinParameters.flags.mcastfreq = 1;
	  }
	
	 pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetMcastFreq(parserCmdInfo_t* pParserCmdInfo)
{
  uint32_t freq; 
  StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;;
  uint8_t groupId = atoi(pParserCmdInfo->pParam1);
  status = LORAWAN_GetAttr(MCAST_FREQUENCY,&groupId,&freq);
  ultoa(aParserData, freq, 10U);
  if(status == LORAWAN_SUCCESS)
  {
	  pParserCmdInfo->pReplyCmd = aParserData;
  }
  else
  {
	  pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
  }
  
}

void Parser_LoraSetMcastDr(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t statusIdx = LORAWAN_INVALID_PARAMETER;
	LorawanMcastDatarate_t key;
	// Parameter validation
	if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &(key.datarate)))
	{
		key.groupId = atoi(pParserCmdInfo->pParam2);
		statusIdx = LORAWAN_SetMulticastParam(MCAST_DATARATE,&key);
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[statusIdx];
}

void Parser_LoraGetMcastDr(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t dr;
	StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;;
	uint8_t groupId = atoi(pParserCmdInfo->pParam1);
	status = LORAWAN_GetAttr(MCAST_DATARATE,&groupId,&dr);
	if(status == LORAWAN_SUCCESS)
	{
	   itoa(dr, aParserData, 10U);
	   pParserCmdInfo->pReplyCmd = aParserData;
	}
	else
	{
	  pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];	
	}
}

static void ParserAppData(void *appHandle, appCbParams_t *data)
{
    uint16_t dataLen;
    uint16_t maxDataLenToTx;

    if (data->evt == LORAWAN_EVT_RX_DATA_AVAILABLE)
    {
        uint8_t *pData = data->param.rxData.pData;
        uint8_t dataLength = data->param.rxData.dataLength;
        StackRetStatus_t status = data->param.rxData.status;
		
		//Added for future        
        switch(status)
        {
            case LORAWAN_SUCCESS:
                //Successful transmission
                if((dataLength > 0U) && (NULL != pData))
                {
                    // Data received
                    strcpy(aParserData, gapParserRxStatus[MAC_RX_DATA_STR_IDX]);
                    dataLen = strlen(aParserData);

                    //Handle port: the first byte from pData represents the port number
                    itoa( *pData, &aParserData[dataLen],10);
                    dataLen = strlen(aParserData);

                    aParserData[dataLen] = ' ';
                    dataLen ++;

                    // Skip port number (&pData[1]), process only data
                    maxDataLenToTx = ((dataLength - 1) <= ((uint16_t)((PARSER_MAX_DATA_LEN - dataLen) >> 1))) ? (dataLength - 1) : ((uint16_t)((PARSER_MAX_DATA_LEN - dataLen) >> 1));
                    Parser_IntArrayToHexAscii(maxDataLenToTx, &pData[1],  &aParserData[dataLen]);

                    Parser_TxAddReply(aParserData, strlen(aParserData));
                }
                else
                {
                    // ACK received for Confirmed data with No Payload
                    Parser_TxAddReply((char*)gapParserRxStatus[MAC_ACK_RXED_STR_IDX], strlen((char*)gapParserRxStatus[MAC_ACK_RXED_STR_IDX]));
                }
                break;
			case LORAWAN_NWK_NOT_JOINED:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[NOT_JOINED_STR_IDX], strlen((char*)gapParserRxStatus[RADIO_ERR_STR_IDX]));
				break;
			case LORAWAN_INVALID_PARAMETER:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_PARAM_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_PARAM_STR_IDX]));
				break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[KEYS_NOT_INIT_STR_IDX], strlen((char*)gapParserLorawanStatus[KEYS_NOT_INIT_STR_IDX]));
				break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[SILENT_STR_IDX], strlen((char*)gapParserLorawanStatus[SILENT_STR_IDX]));
				break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[FRAME_CNTR_ERR_REJOIN_NEEDED_STR_IDX], strlen((char*)gapParserLorawanStatus[FRAME_CNTR_ERR_REJOIN_NEEDED_STR_IDX]));
				break;
			case LORAWAN_FCNTR_ERROR:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_FCNTR_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_FCNTR_STR_IDX]));
				break;
			case LORAWAN_MIC_ERROR:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[MIC_ERROR_STR_IDX], strlen((char*)gapParserRxStatus[MIC_ERROR_STR_IDX]));
				break;
			case LORAWAN_INVALID_MTYPE:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_MTYPE_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_MTYPE_STR_IDX]));
				break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_BUFFER_LEN_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_BUFFER_LEN_STR_IDX]));
				break;
			case LORAWAN_MAC_PAUSED:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[MAC_PAUSED_STR_IDX], strlen((char*)gapParserLorawanStatus[MAC_PAUSED_STR_IDX]));
				break;
			case LORAWAN_MCAST_HDR_INVALID:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[MCAST_HDR_INVALID_STR_IDX], strlen((char*)gapParserRxStatus[MCAST_HDR_INVALID_STR_IDX]));
				break;
			case LORAWAN_NO_CHANNELS_FOUND:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[NO_FREE_CHANNEL_STR_IDX], strlen((char*)gapParserLorawanStatus[NO_FREE_CHANNEL_STR_IDX]));
				break;
			case LORAWAN_BUSY:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[BUSY_STR_IDX], strlen((char*)gapParserLorawanStatus[BUSY_STR_IDX]));
				break;
			case LORAWAN_NO_ACK:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[NO_ACK_STR_IDX], strlen((char*)gapParserRxStatus[NO_ACK_STR_IDX]));
				break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[JOIN_IN_PROGRESS_STR_IDX], strlen((char*)gapParserLorawanStatus[JOIN_IN_PROGRESS_STR_IDX]));
				break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RESOURCE_UNAVAILABLE_STR_IDX], strlen((char*)gapParserLorawanStatus[RESOURCE_UNAVAILABLE_STR_IDX]));
				break;
			case LORAWAN_INVALID_REQUEST:
				// Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_REQ_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_REQ_STR_IDX]));
				break;	
           	case LORAWAN_INVALID_PACKET:
           		Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_PACKET_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_PACKET_STR_IDX]));
           		break;
            default:
                break;
        }
    }
    else if(data->evt == LORAWAN_EVT_TRANSACTION_COMPLETE)
    {
        switch(data->param.transCmpl.status)
        {   
			case LORAWAN_SUCCESS:
	        {
		        Parser_TxAddReply((char*)gapParserTxStatus[MAC_TX_OK_STR_IDX], strlen((char*)gapParserTxStatus[MAC_TX_OK_STR_IDX]));
	        }
	        break;
	        
	        case LORAWAN_RADIO_SUCCESS:
	        {
		        Parser_TxAddReply((char*)gapParserTxStatus[RADIO_TX_OK_STR_IDX], strlen((char*)gapParserTxStatus[RADIO_TX_OK_STR_IDX]));
	        }
	        break;
            case LORAWAN_RADIO_NO_DATA:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_NO_DATA_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_NO_DATA_STR_IDX]));
			    break;
			case LORAWAN_RADIO_TX_TIMEOUT:
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_TX_TIMEOUT_IDX], strlen((char*)gapParserLorawanStatus[RADIO_TX_TIMEOUT_IDX]));
				break;
			case LORAWAN_TX_TIMEOUT:
				Parser_TxAddReply((char*)gapParserLorawanStatus[TX_TIMEOUT_IDX], strlen((char*)gapParserLorawanStatus[TX_TIMEOUT_IDX]));
				break;				
            case LORAWAN_RADIO_DATA_SIZE:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_DATA_SIZE_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_DATA_SIZE_STR_IDX]));
				break;
            case LORAWAN_RADIO_INVALID_REQ:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_INVALID_REQ_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_INVALID_REQ_STR_IDX]));
				break;
            case LORAWAN_RADIO_BUSY:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[LORAWAN_RADIO_BUSY_STR_IDX], strlen((char*)gapParserLorawanStatus[LORAWAN_RADIO_BUSY_STR_IDX]));
				break;
            case LORAWAN_RADIO_OUT_OF_RANGE:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_OUT_OF_RANGE_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_OUT_OF_RANGE_STR_IDX]));
				break;
            case LORAWAN_RADIO_UNSUPPORTED_ATTR:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_UNSUP_ATTR_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_UNSUP_ATTR_STR_IDX]));
				break;
            case LORAWAN_RADIO_CHANNEL_BUSY:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RADIO_CHANNEL_BUSY_STR_IDX], strlen((char*)gapParserLorawanStatus[RADIO_CHANNEL_BUSY_STR_IDX]));
				break;
            case LORAWAN_NWK_NOT_JOINED:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[NOT_JOINED_STR_IDX], strlen((char*)gapParserRxStatus[RADIO_ERR_STR_IDX]));
				break;
            case LORAWAN_INVALID_PARAMETER:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_PARAM_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_PARAM_STR_IDX]));
				break;
            case LORAWAN_KEYS_NOT_INITIALIZED:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[KEYS_NOT_INIT_STR_IDX], strlen((char*)gapParserLorawanStatus[KEYS_NOT_INIT_STR_IDX]));
				break;
            case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[SILENT_STR_IDX], strlen((char*)gapParserLorawanStatus[SILENT_STR_IDX]));
				break;
            case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[FRAME_CNTR_ERR_REJOIN_NEEDED_STR_IDX], strlen((char*)gapParserLorawanStatus[FRAME_CNTR_ERR_REJOIN_NEEDED_STR_IDX]));
				break;
            case LORAWAN_FCNTR_ERROR:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_FCNTR_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_FCNTR_STR_IDX]));
				break;
            case LORAWAN_MIC_ERROR:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[MIC_ERROR_STR_IDX], strlen((char*)gapParserRxStatus[MIC_ERROR_STR_IDX]));
				break;
            case LORAWAN_INVALID_MTYPE:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_MTYPE_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_MTYPE_STR_IDX]));
				break;
            case LORAWAN_INVALID_BUFFER_LENGTH:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[INVALID_BUFFER_LEN_STR_IDX], strlen((char*)gapParserRxStatus[INVALID_BUFFER_LEN_STR_IDX]));
				break;
            case LORAWAN_MAC_PAUSED:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[MAC_PAUSED_STR_IDX], strlen((char*)gapParserLorawanStatus[MAC_PAUSED_STR_IDX]));
				break;
            case LORAWAN_MCAST_HDR_INVALID:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[MCAST_HDR_INVALID_STR_IDX], strlen((char*)gapParserRxStatus[MCAST_HDR_INVALID_STR_IDX]));
				break;
            case LORAWAN_NO_CHANNELS_FOUND:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[NO_FREE_CHANNEL_STR_IDX], strlen((char*)gapParserLorawanStatus[NO_FREE_CHANNEL_STR_IDX]));
				break;
            case LORAWAN_BUSY:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[BUSY_STR_IDX], strlen((char*)gapParserLorawanStatus[BUSY_STR_IDX]));
				break;
            case LORAWAN_NO_ACK:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserRxStatus[NO_ACK_STR_IDX], strlen((char*)gapParserRxStatus[NO_ACK_STR_IDX]));
				break;
            case LORAWAN_NWK_JOIN_IN_PROGRESS:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[JOIN_IN_PROGRESS_STR_IDX], strlen((char*)gapParserLorawanStatus[JOIN_IN_PROGRESS_STR_IDX]));
				break;
            case LORAWAN_RESOURCE_UNAVAILABLE:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[RESOURCE_UNAVAILABLE_STR_IDX], strlen((char*)gapParserLorawanStatus[RESOURCE_UNAVAILABLE_STR_IDX]));
				break;
            case LORAWAN_INVALID_REQUEST:
            // Failed transmission
				Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_REQ_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_REQ_STR_IDX]));
				break;
			case LORAWAN_INVALID_PACKET:
				Parser_TxAddReply((char*)gapParserLorawanStatus[INVALID_PACKET_STR_IDX], strlen((char*)gapParserLorawanStatus[INVALID_PACKET_STR_IDX]));
				break;
            
            default:
            break;
            
							
        }
    }
	
	appHandle = NULL;
}

static void ParserJoinData(StackRetStatus_t status)
{
    uint8_t statusIdx = JOIN_DENY_STR_IDX;

    //This is called every time the join process is finished
    if(LORAWAN_SUCCESS == status)
    {
        //Sucessful join
        statusIdx = JOIN_ACCEPT_STR_IDX;
    }
	else if (LORAWAN_NO_CHANNELS_FOUND == status)
	{
		statusIdx = JOIN_NO_FREE_CHANNEL_STR_IDX;
	}
	else if (LORAWAN_TX_TIMEOUT == status)
	{
		statusIdx = JOIN_TX_TIMEOUT_STR_IDX;
	}
	else if (LORAWAN_MIC_ERROR == status)
	{
		statusIdx = JOIN_MIC_ERROR_STR_IDX;
	}
				
    Parser_TxAddReply((char*)gapParseJoinStatus[statusIdx], strlen((char*)gapParseJoinStatus[statusIdx]));
}

void Parser_LoraGetMacLastPacketRssi(parserCmdInfo_t* pParserCmdInfo)
{
	int16_t rssi;

	LORAWAN_GetAttr(LAST_PACKET_RSSI, NULL, &rssi);
	itoa(rssi,aParserData, 10U);

	pParserCmdInfo->pReplyCmd = aParserData;
}
void Parser_LoraGetIsFpending(parserCmdInfo_t* pParserCmdInfo)
{
	bool isFpending;

	LORAWAN_GetAttr(IS_FPENDING, NULL, &isFpending);
	pParserCmdInfo->pReplyCmd = (char*)gapParserBool[isFpending];
}
void Parser_LoraGetMacDlAckReqd(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t req;

	LORAWAN_GetAttr(DL_ACK_REQD, NULL, &req);
	pParserCmdInfo->pReplyCmd = (char*)gapParserBool[req];
	
}
void Parser_LoraGetMacLastChId(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t id;

	LORAWAN_GetAttr(LAST_CH_ID, NULL, &id);
	utoa(id, aParserData,  10U);

	pParserCmdInfo->pReplyCmd = aParserData;	
}
void Parser_LoraGetMacPendingDutyCycle(parserCmdInfo_t* pParserCmdInfo)
{
	uint32_t value;

	LORAWAN_GetAttr(PENDING_DUTY_CYCLE_TIME, NULL, &value);
	utoa(value, aParserData,  10U);

	pParserCmdInfo->pReplyCmd = aParserData;	
}
void Parser_LoraGetMacCnfRetryCnt(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t ctr;

	LORAWAN_GetAttr(RETRY_COUNTER_CNF, NULL, &ctr);
	utoa(ctr, aParserData,  10U);

	pParserCmdInfo->pReplyCmd = aParserData;	
}
void Parser_LoraGetMacUncnfRetryCnt(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t ctr;

	LORAWAN_GetAttr(RETRY_COUNTER_UNCNF, NULL, &ctr);
	utoa(ctr, aParserData,  10U);

	pParserCmdInfo->pReplyCmd = aParserData;	
}
void Parser_LoraGetMacNextPayloadSize(parserCmdInfo_t* pParserCmdInfo)
{
	uint16_t size;

	LORAWAN_GetAttr(NEXT_PAYLOAD_SIZE, NULL, &size);
	utoa(size, aParserData,  10U);

	pParserCmdInfo->pReplyCmd = aParserData;	
}

void Parser_LoraGetJoindutycycleremaining(parserCmdInfo_t* pParserCmdInfo)
{
	uint32_t remainingtime;
	LORAWAN_GetAttr(PENDING_JOIN_DUTY_CYCLE_TIME,NULL, &remainingtime);
	utoa(remainingtime, aParserData, 10U);
	pParserCmdInfo->pReplyCmd = aParserData;
}

void Parser_LoraSetJoinBackoff(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t validationVal;
	uint8_t returnVal = LORAWAN_INVALID_PARAMETER;
	bool joinBackoffEnable;

	validationVal = Validate_OnOffAsciiValue(pParserCmdInfo->pParam1);
	if (validationVal < 2U)
	{
		joinBackoffEnable = validationVal;
		returnVal = LORAWAN_SetAttr(JOIN_BACKOFF_ENABLE, &joinBackoffEnable);
	}
	
	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[returnVal];
}

void Parser_LoraSetMaxFcntPdsUpdtVal(parserCmdInfo_t* pParserCmdInfo)
{
	uint8_t status = LORAWAN_INVALID_PARAMETER;
	uint8_t fcnt;

	// Parameter validation
	if(Validate_Uint8DecAsciiValue(pParserCmdInfo->pParam1, &fcnt))
	{
		status = LORAWAN_SetAttr(MAX_FCNT_PDS_UPDATE_VAL,&fcnt);
	}

	pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetJoinBackoff(parserCmdInfo_t* pParserCmdInfo)
{
	bool joinBackoffEnable ;
	StackRetStatus_t status;

	status = LORAWAN_GetAttr(JOIN_BACKOFF_ENABLE, NULL, &joinBackoffEnable);
	if(status == LORAWAN_SUCCESS)
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParseOnOff[joinBackoffEnable];
	}
	else
	{
		pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
	}
}


void Parser_LoraSetJoinNonceType(parserCmdInfo_t* pParserCmdInfo)
{
    JoinNonceType_t jntype = atoi(pParserCmdInfo->pParam1);
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
    
    if ((jntype == JOIN_NONCE_INCREMENTAL) ||
            (jntype == JOIN_NONCE_RANDOM))
    {
        status = LORAWAN_SetAttr(JOIN_NONCE_TYPE, &jntype);
    }
    
    pParserCmdInfo->pReplyCmd = (char*)gapParserLorawanStatus[status];
}

void Parser_LoraGetJoinNonceType(parserCmdInfo_t* pParserCmdInfo)
{
    JoinNonceType_t jntype;
    LORAWAN_GetAttr(JOIN_NONCE_TYPE, NULL, &jntype);
    utoa(jntype, aParserData, 10);
    pParserCmdInfo->pReplyCmd = aParserData;
}
