/**
* \file  lorawan.c
*
* \brief LoRaWAN file
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
#include "conf_stack.h"
#include "lorawan_defs.h"
#include "lorawan_private.h"
#include "lorawan_radio.h"
#include "lorawan_mcast.h"
#include "aes_engine.h"
#include "radio_interface.h"
#include "sw_timer.h"
#include "lorawan_task_handler.h"
#include "lorawan_reg_params.h"
#include "radio_driver_hal.h"
#include "system_assert.h"
#include "pds_interface.h"
#include "sal.h"
#include <math.h>

/****************************** VARIABLES *************************************/

#define MICROCHIP_SAM_DEVICE_ID (0x1F)
#define AES_BLOCKSIZE			 16

LoRa_t loRa;

#include "lorawan_pds.h"

extern ItemMap_t pds_mac_fid1_item_list[];
extern ItemMap_t pds_mac_fid2_item_list[];

PdsOperations_t aMacPdsOps_Fid1[PDS_MAC_FID1_MAX_VALUE];
PdsOperations_t aMacPdsOps_Fid2[PDS_MAC_FID2_MAX_VALUE];

//CID = LinkCheckReq      =  2 | 0x02
//CID = LinkADRAns        =  3 | 0x03
//CID = DutyCycleAns      =  4 | 0x04
//CID = RX2SetupAns       =  5 | 0x05
//CID = DevStatusAns      =  6 | 0x06
//CID = NewChannelAns     =  7 | 0x07
//CID = RXTimingSetupAns  =  8 | 0x08
//CID = TxParamSetupAns   =  9 | 0x09
//CID = DlChannelSetupAns = 10 | 0x0A
//CID = DeviceTimeReq     = 13 | 0x0D
// Index in macEndDevCmdReplyLen = CID - 2
static const uint8_t macEndDevCmdReplyLen[] = {1, 2, 1, 2, 3, 2, 1, 1, 2, 0, 0, 1, 0, 0, 2, 2, 0, 2};
static const uint8_t macEndDevCmdInputLen[] = {2, 4, 1, 4, 0, 5, 1, 1, 4, 0, 0, 5};
uint8_t macBuffer[MAXIMUM_BUFFER_LENGTH];
static uint8_t aesBuffer[AES_BLOCKSIZE];
AppData_t AppPayload;
FHSSCallback_t fhssCallback;

volatile RadioCallbackID_t callbackBackup;
extern uint8_t radioBuffer[];
static const uint8_t FskSyncWordBuff[3] = {0xC1, 0x94, 0xC1};

/* LoRaWAN Spec 1.0.2 section 5.8 for TxParamSetupReq MAC command defines EIRP values. These values are stored in below array */	
static const uint8_t maxEIRPTable[] = {8,10,12,13,14,16,18,20,21,24,26,27,29,30,33,36};


/************************ PRIVATE FUNCTION PROTOTYPES *************************/

static StackRetStatus_t CreateAllSoftwareTimers (void);

static void StopAllSoftwareTimers (void);

static void UpdateReceiveDelays (uint8_t delay);

static void UpdateCfList (uint8_t bufferLength, JoinAccept_t *joinAccept);

static uint8_t* ExecuteLinkCheck (uint8_t *ptr);

static uint8_t* ExecuteRxTimingSetup (uint8_t *ptr);

static void PrepareSessionKeys (uint8_t* sessionKey, uint8_t* joinNonce, uint8_t* networkId);

static void ComputeSessionKeys (JoinAccept_t *joinAcceptBuffer);

static void IncludeMacCommandsResponse (uint8_t* macCommandsBuffer, uint16_t* pBufferIndex, uint8_t bIncludeInFopts );

static void CheckFlags (Hdr_t* hdr);

static void AssembleEncryptionBlock (uint8_t dir, uint32_t frameCounter, uint8_t blockId, uint8_t firstByte, uint32_t devAddr);

static uint32_t ExtractMic (uint8_t *buffer, uint8_t bufferLength);

static uint32_t ComputeMic ( uint8_t *key, uint8_t* buffer, uint8_t bufferLength);

static uint8_t* MacExecuteCommands (uint8_t *buffer, uint8_t fOptsLen);

static void MacClearCommands (void);

static void SetReceptionNotOkState (void);

static uint8_t CountfOptsLength (uint8_t *flag);

static uint8_t LorawanGetMaxPayloadSize (uint8_t dataRate);

static void FindSmallestDataRate (void);

static void TransmissionErrorCallback (void);

static void StopReceiveWindow2Timer(void);

static void handleTransmissionTimeoutCallback(void);

static uint32_t calcPacketTimeOnAir(uint8_t datarate, uint8_t preambleLen,
uint8_t impHdrMode, uint8_t crcOn, uint8_t cr, uint8_t length);

static void lorawanADR(FCtrl_t *fCtrl);

static StackRetStatus_t checkRxPacketPayloadLen(uint8_t bufferLength, Hdr_t *hdr);


/*********************************************************************//**
\brief	This function calls the respective callback function of the
		lorawan layer from radio layer.

\param	callback	- The callback that is propagated from the radio layer.
\param	param		- The callback parameters.
\return		- none.
*************************************************************************/
static void radioCallback(RadioCallbackID_t callback, void *param);


static void ConfigureRadio(radioConfig_t* radioConfig);

static void UpdateLinkAdrCommands(uint16_t channelMask,uint8_t chMaskCntl,uint8_t nbRep,uint8_t txPower,uint8_t dataRate);
/****************************** PUBLIC FUNCTIONS ******************************/

void LORAWAN_Init(AppDataCb_t appdata, JoinResponseCb_t joindata) // this function resets everything to the default values
{
	RadioError_t status;
    #if SAM0
    uint8_t pid;
    pid = (*((uint8_t *)0x41003FE4) >> 4) | ((*((uint8_t *)0x41003FE8) & 0x07) << 4);
    /* Check for Microchip Identifier of the device...
        If device is not Microchip, the init sequence enters a forever loop.
        This is to avoid other ARM device manufactures using our library */
    if (MICROCHIP_SAM_DEVICE_ID != pid)
    {
            while (1);
    }
    #endif
    	
    // Allocate software timers and their callbacks
    if (loRa.macInitialized == DISABLED)
    {
        if (LORAWAN_SUCCESS == CreateAllSoftwareTimers ())
		{
			loRa.macInitialized = ENABLED;
		}
    }
    else
    {
        StopAllSoftwareTimers ();
    }

	if (ENABLED == loRa.macInitialized)
	{
		AppPayload.AppData = appdata;
		AppPayload.JoinResponse = joindata;

		RADIO_Init();
		status = RADIO_SetAttr(RADIO_CALLBACK, (void *)&radioCallback);

		srand (RADIO_ReadRandom ());  // for the loRa random function we need a seed that is obtained from the radio

	}
	
	{
		
		/* Registering to PDS Module */
		PdsFileMarks_t mac_filemarks;
		
		/* File ID 1 - Register */
		//mac_filemarks.fileID = PDS_FILE_MAC_01_IDX;
		mac_filemarks.fileMarkListAddr = aMacPdsOps_Fid1;
		mac_filemarks.numItems = (uint8_t)(PDS_MAC_FID1_MAX_VALUE & 0x00FF);
		mac_filemarks.itemListAddr = pds_mac_fid1_item_list;
		mac_filemarks.fIDcb = Lorawan_Pds_fid1_CB;
		PDS_RegFile(PDS_FILE_MAC_01_IDX,mac_filemarks);
		/* File ID 2 - Register */
		//mac_filemarks.fileID = PDS_FILE_MAC_02_IDX;
		mac_filemarks.fileMarkListAddr = aMacPdsOps_Fid2;
		mac_filemarks.numItems = (uint8_t)(PDS_MAC_FID2_MAX_VALUE & 0x00FF);
		mac_filemarks.itemListAddr = pds_mac_fid2_item_list;
		mac_filemarks.fIDcb = Lorawan_Pds_fid2_CB;
		PDS_RegFile(PDS_FILE_MAC_02_IDX,mac_filemarks);	
	}

    {
        SwTimestamp_t invalidTime = UINT64_MAX;
        SwTimerWriteTimestamp(loRa.devTime.sysEpochTimeIndex, &invalidTime);
        loRa.devTime.gpsEpochTime.fractionalSecond = UINT8_MAX;
        loRa.devTime.gpsEpochTime.secondsSinceEpoch = UINT32_MAX;
        loRa.devTime.isDevTimeReqSent = false;
    }
	
	/* Keep compiler happy */
	status = status;
}

StackRetStatus_t LORAWAN_Reset (IsmBand_t ismBand)
{
	uint8_t paBoost;
	IsmBand_t prevBand = 0xff;
	
	StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

    if (loRa.macInitialized == ENABLED)
    {
        StopAllSoftwareTimers ();
    }
    LORAWAN_GetAttr(ISMBAND,NULL,&prevBand);
	if(prevBand != ismBand)
	{
	    LORAREG_UnInit();		
	}

    loRa.stackVersion.value = STACK_VERSION_VALUE;

    loRa.syncWord = MAC_LORA_MODULATION_SYNCWORD;
    RADIO_SetAttr(LORA_SYNC_WORD,(void *)&(loRa.syncWord));

    loRa.edClass = CLASS_A;
    loRa.edClassesSupported = LORAWAN_SUPPORTED_ED_CLASSES;
    loRa.macStatus.value = DEF_MACSTATUS;
    loRa.linkCheckMargin = MAC_LINK_CHECK_MARGIN; 
    loRa.linkCheckGwCnt = MAC_LINK_CHECK_GATEWAY_COUNT;
    loRa.lastPacketLength = 0;
    loRa.fCntDown.value = 0;
    loRa.fCntUp.value = 0;
    loRa.devNonce = MAC_DEVNONCE;
    loRa.joinNonce = MAC_JOINNONCE;
    loRa.joinNonceType = JOIN_NONCE_INCREMENTAL;
    loRa.aggregatedDutyCycle = MAC_AGGREGATED_DUTYCYCLE;
    loRa.adrAckCnt = 0;
    loRa.counterAdrAckDelay = 0;
    loRa.offset = 0;
    loRa.evtmask = (LORAWAN_EVT_RX_DATA_AVAILABLE |  LORAWAN_EVT_TRANSACTION_COMPLETE );
    loRa.appHandle = NULL;
	loRa.lbt.elapsedChannels = 0;
	loRa.lbt.maxRetryChannels = 0;
    memset(&loRa.cbPar, 0, sizeof(appCbParams_t));
    loRa.isTransactionDone = true;
    loRa.macStatus.macState = IDLE;
	memset(&loRa.linkAdrResp,0x00,sizeof(LinkAdrResp_t));
    // link check mechanism should be disabled
    loRa.macStatus.linkCheck = DISABLED;
	loRa.maxFcntPdsUpdateValue = 0;
	loRa.joinAcceptChMaskReceived = 0;

    //flags all 0-es
    loRa.macStatus.value = 0;
    loRa.lorawanMacStatus.value = 0;

    loRa.maxRepetitionsConfirmedUplink = MAC_CONFIRMABLE_UPLINK_REPITITIONS_MAX; // 7 retransmissions should occur for each confirmed frame sent until ACK is received
    loRa.maxRepetitionsUnconfirmedUplink = MAC_UNCONFIRMABLE_UPLINK_REPITITIONS_MAX; // 0 retransmissions should occur for each unconfirmed frame sent until a response is received
    loRa.counterRepetitionsConfirmedUplink = DEF_CNF_UL_REPT_CNT;
    loRa.counterRepetitionsUnconfirmedUplink = DEF_UNCNF_UL_REPT_CNT;
	
    status = LORAREG_Init(ismBand);
	if (status != LORAWAN_SUCCESS)
	{
		return LORAWAN_INVALID_PARAMETER;
	}

	LORAREG_GetAttr(DEFAULT_RX1_DATA_RATE,NULL,&(loRa.receiveWindow1Parameters.dataRate)); //LORAWAN_GetDefaultRx1DR();
	LORAREG_GetAttr(SUPPORTED_REGIONAL_FEATURES,NULL,&(loRa.featuresSupported));

    loRa.batteryLevel = EXTERNALLY_POWERED; // the end device was not able to measure the battery level

    // initialize default channels
    LORAREG_GetAttr(MAX_CHANNELS,NULL,&(loRa.maxChannels));

    RADIO_Init();

    loRa.ismBand	 = ismBand;
	PDS_STORE(PDS_MAC_ISM_BAND);
    if(loRa.featuresSupported & PA_SUPPORT)
    {
        paBoost = ENABLED;
        RADIO_SetAttr(PABOOST,(void *)&paBoost);
    }
	
	RADIO_GetAttr(RADIO_CLOCK_STABLE_DELAY, &loRa.radioClkStableDelay);

	if(loRa.featuresSupported & LBT_SUPPORT)
	{
		LorawanLBTParams_t LorawanLBTParams;
		
		/*Get the default Regional LBT Parameters and set it to MAC*/
		LORAREG_GetAttr(DEFAULT_LBT_PARAMS,NULL,&(LorawanLBTParams));

		LORAWAN_SetAttr(LORAWAN_LBT_PARAMS,&LorawanLBTParams);		
	}
	
    LORAREG_GetAttr(DEFAULT_RX2_DATA_RATE,NULL,&(loRa.receiveWindow2Parameters.dataRate));
    LORAREG_GetAttr(DEFAULT_RX2_FREQUENCY,NULL,&(loRa.receiveWindow2Parameters.frequency));

	LORAREG_GetAttr(REG_DEF_TX_POWER,NULL,&(loRa.txPower));
	LORAREG_GetAttr(REG_DEF_TX_DATARATE,NULL,&(loRa.currentDataRate));

    MinMaxDr_t minmaxDr;
    LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr)); 

    loRa.minDataRate = minmaxDr.minDr;
    loRa.maxDataRate = minmaxDr.maxDr;

    //keys will be filled with 0
    loRa.macKeys.value = 0;  //no keys are set
    memset (&loRa.activationParameters, 0, sizeof(loRa.activationParameters));

    //protocol parameters receive the default values
    loRa.protocolParameters.receiveDelay1 = RECEIVE_DELAY1;
    loRa.protocolParameters.receiveDelay2 = RECEIVE_DELAY2;
    loRa.protocolParameters.joinAcceptDelay1 = JOIN_ACCEPT_DELAY1;
    loRa.protocolParameters.joinAcceptDelay2 = JOIN_ACCEPT_DELAY2;
    loRa.protocolParameters.retransmitTimeout = RETRANSMIT_TIMEOUT;
	loRa.protocolParameters.adrAckDelay = ADR_ACK_DELAY;
    loRa.protocolParameters.adrAckLimit = ADR_ACK_LIMIT;
    LorawanLinkCheckConfigure (DISABLED); // disable the link check mechanism
    LorawanMcastInit();

	return status;
}

StackRetStatus_t LORAWAN_Join(ActivationType_t activationTypeNew)
{

    /* Any further transmissions or receptions cannot occur is macPaused is enabled */
    if (loRa.macStatus.macPause == ENABLED)
    {
        return LORAWAN_MAC_PAUSED;
    }
    /* The server decided that any further uplink transmission is not possible from this end device.*/
    if (loRa.macStatus.silentImmediately == ENABLED)
    {
        return LORAWAN_SILENT_IMMEDIATELY_ACTIVE;
    }

    /* If Joining is in Progress ,return busy*/
    if (loRa.lorawanMacStatus.joining == true)
    {
	    return LORAWAN_NWK_JOIN_IN_PROGRESS;
    }
	
	/*Join will only occur when device is ready in CLass C*/
	if ((CLASS_C == loRa.edClass) &&
	(LORAWAN_SUCCESS != LorawanClasscValidateSend()))
	{
		return LORAWAN_BUSY;
	}
	
	/* Transmission will only happen if MAC is not in IDLE state*/
	if ((CLASS_A == loRa.edClass) &&
	(loRa.macStatus.macState != IDLE))
	{
		return LORAWAN_BUSY;
	}

    loRa.activationParameters.activationType = activationTypeNew;
	PDS_STORE(PDS_MAC_ACTIVATION_TYPE);
	

    if (LORAWAN_OTAA == activationTypeNew)
    {
        //OTAA
        if (((loRa.macKeys.deviceEui == 0) || (loRa.macKeys.joinEui == 0) || (loRa.macKeys.applicationKey == 0)) && (loRa.cryptoDeviceEnabled != true))
        {
            return LORAWAN_KEYS_NOT_INITIALIZED;
        }
        else
        {
			/* If already joined, enable all default channels in RegParams */
			if(loRa.macStatus.networkJoined == ENABLED)
			{
				LORAREG_SetAttr(REG_JOIN_ENABLE_ALL,NULL);
			}
			/* set the states and flags accordingly */
			loRa.macStatus.networkJoined = 0;
			loRa.lorawanMacStatus.joining = true;
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
            /* Post task to MAC to initiate Join req frame transmission */
            LORAWAN_PostTask(LORAWAN_JOIN_TASK_ID);
            return LORAWAN_SUCCESS;
        }

    }
    else
    {
        //ABP
        if ( (loRa.macKeys.applicationSessionKey == 0) || (loRa.macKeys.networkSessionKey == 0) || (loRa.macKeys.deviceAddress == 0) )
        {
            return LORAWAN_KEYS_NOT_INITIALIZED;
        }
        else
        {
            UpdateJoinInProgress(ABP_DELAY);            
            SwTimerStart(loRa.abpJoinTimerId, MS_TO_US(ABP_TIMEOUT_MS), SW_TIMEOUT_RELATIVE, (void *)UpdateJoinSuccessState, NULL);
            return LORAWAN_SUCCESS;
        }
    }
}

StackRetStatus_t LORAWAN_Send (LorawanSendReq_t *lorasendreq)
{
	bool sendOnlyMacReply = false;
	StackRetStatus_t status = LORAWAN_SUCCESS;
	
    /* Any further transmissions or receptions cannot occur is macPaused is enabled*/
    if (loRa.macStatus.macPause == ENABLED)
    {
        return LORAWAN_MAC_PAUSED;
    }
    /* The server decided that any further uplink transmission is not possible from this end device.*/
    if (loRa.macStatus.silentImmediately == ENABLED)
    {
        return LORAWAN_SILENT_IMMEDIATELY_ACTIVE;
    }
    /* The network needs to be joined before sending */
	if (loRa.macStatus.networkJoined == DISABLED)
	{
		return LORAWAN_NWK_NOT_JOINED;
	}
	
	if (false == loRa.isTransactionDone)
	{
		return LORAWAN_BUSY;
	}

    if (NULL != lorasendreq)
    {
		/*Port number should be <= 1 if there is data to send.
         * If port number is 0, it indicates only Mac commands are inside FRM Payload */
	    if (((lorasendreq->port < FPORT_MIN) || (lorasendreq->port > LORAWAN_TEST_PORT )) &&  
			(lorasendreq->bufferLength != 0)) 
        {
            return LORAWAN_INVALID_PARAMETER;
        }
		uint8_t foptsFlag = false;
        /* validate data length using MaxPayloadSize */
		uint8_t macCmdReplyLen = CountfOptsLength(&foptsFlag);
		
		

		if (((lorasendreq->bufferLength + macCmdReplyLen ) > LorawanGetMaxPayloadSize (loRa.currentDataRate)) || (!foptsFlag))
        {
			if(macCmdReplyLen == 0 )
			{
				return LORAWAN_INVALID_BUFFER_LENGTH;
			}
			else
			{
				//send payload in FRMpayload
				sendOnlyMacReply = true;
			}
        }
    }

    if (loRa.fCntUp.value == FCNT_MAX)
    {
        /* Inform application about rejoin in status */
        loRa.macStatus.rejoinNeeded = 1;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);
        return LORAWAN_FCNTR_ERROR_REJOIN_NEEDED;
    }

	if ((CLASS_C == loRa.edClass) &&
        (LORAWAN_SUCCESS != LorawanClasscValidateSend()))
    {
        return LORAWAN_BUSY;
    }
    /* Transmission will only happen if MAC is not in IDLE state*/
	if ((CLASS_A == loRa.edClass) &&
        (loRa.macStatus.macState != IDLE))
	{
		return LORAWAN_BUSY;
	}

	/* Copy the application buffer pointer to global pointer */
	if(false == sendOnlyMacReply)
	{
		loRa.appHandle = lorasendreq;	
	}
	else
	{
		loRa.appHandle = NULL;
		status = LORAWAN_BUSY;
	}
	

	loRa.isTransactionDone = false;	

    /* Post task to LORAWAN handler to send Join req*/
    LORAWAN_PostTask(LORAWAN_TX_TASK_ID);

  /*
   * set the synchronization flag because one packet was sent
   * (this is a guard for the the RxAppData of the user)
   */
  loRa.lorawanMacStatus.syncronization = ENABLED;
  return status;

}

void LORAWAN_SetCallbackBitmask(uint32_t evtmask)
{
    if (evtmask < LORAWAN_EVT_UNSUPPORTED)
    {
        loRa.evtmask = evtmask;
		PDS_STORE(PDS_MAC_EVENT_MASK);
    }
}

/* This function is called when there is a need to send data outside the MAC layer
 It can be called when MAC is in idle, before RX1, between RX1 and RX2 or retransmission delay state
 It will return how much time other transmissions can occur*/
uint32_t LORAWAN_Pause (void)
{
    uint32_t timeToPause = 0;

    if ((CLASS_C == loRa.edClass) && (true == loRa.macStatus.networkJoined))
    {
        timeToPause = LorawanClasscPause();
    }
    else
    {
        switch (loRa.macStatus.macState)
        {
        case IDLE:
        {
            timeToPause = UINT32_MAX;
        }
        break;

        case BEFORE_RX1:
        {
            if (loRa.lorawanMacStatus.joining == true)
            {
                timeToPause = US_TO_MS(SwTimerReadValue (loRa.joinAccept1TimerId));
            }
            else if (loRa.macStatus.networkJoined == ENABLED)
            {
                timeToPause = US_TO_MS(SwTimerReadValue (loRa.receiveWindow1TimerId));
            }
        }
        break;

        case BETWEEN_RX1_RX2:
        {
            if (loRa.lorawanMacStatus.joining == true)
            {
                timeToPause = SwTimerReadValue (loRa.joinAccept2TimerId);
            }
            else if (loRa.macStatus.networkJoined == ENABLED)
            {
                timeToPause = SwTimerReadValue (loRa.receiveWindow2TimerId);
            }
            timeToPause = US_TO_MS(timeToPause);
        }
        break;

        case RETRANSMISSION_DELAY:
        {
            timeToPause = SwTimerReadValue (loRa.ackTimeoutTimerId);
            timeToPause = US_TO_MS(timeToPause);
        }
        break;

        default:
        {
            timeToPause = 0;
        }
        break;
        }
    }

    if (timeToPause >= 200)
    {
        timeToPause = timeToPause - 50; //this is a guard in case of non-syncronization
        loRa.macStatus.macPause = ENABLED;
    }
    else
    {
        timeToPause = 0;
        loRa.macStatus.macPause = DISABLED;
    }

    return timeToPause;
}

void LORAWAN_Resume (void)
{
    /* Stay in idle for class C, and wait for tx from app */
    loRa.macStatus.macPause = DISABLED;
}

// period will be in seconds
void LorawanLinkCheckConfigure (uint16_t period)
{
    uint8_t iCtr;

    loRa.periodForLinkCheck = period * 1000UL;
	PDS_STORE(PDS_MAC_PERIOD_FOR_LINK_CHK);
    // max link check period is 18 hours, period 0 means disabling the link check mechanism, default is disabled
    if (period == 0)
    {
        SwTimerStop(loRa.linkCheckTimerId); // stop the link check timer
        loRa.macStatus.linkCheck = DISABLED;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);
        for(iCtr = 0; iCtr < loRa.crtMacCmdIndex; iCtr ++)
        {
            if(loRa.macCommands[iCtr].receivedCid == LINK_CHECK_CID)
            {
                //disable the link check mechanism
                //Mark this CID as invalid
                loRa.macCommands[iCtr].receivedCid = INVALID_VALUE;
                loRa.crtMacCmdIndex --;
            }
        }
    }
    else
    {
        loRa.macStatus.linkCheck = ENABLED;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);

        // if network is joined, the timer can start, otherwise after the network is joined the link check timer will start counting automatially
        if (loRa.macStatus.networkJoined == ENABLED)
        {
            SwTimerStart(loRa.linkCheckTimerId, MS_TO_US(loRa.periodForLinkCheck), SW_TIMEOUT_RELATIVE, (void *)LorawanLinkCheckCallback, NULL);
        }
    }

}

StackRetStatus_t EncodeDeviceTimeReq(void)
{
    StackRetStatus_t status = LORAWAN_INVALID_REQUEST;
    if(loRa.crtMacCmdIndex < MAX_NB_CMD_TO_PROCESS)
    {
        loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = DEV_TIME_CID;
        loRa.crtMacCmdIndex ++;
        status = LORAWAN_SUCCESS;
    }
    return status;
}


StackRetStatus_t EncodeLinkCheckReq(void)
{
    StackRetStatus_t status = LORAWAN_INVALID_REQUEST;
    if(loRa.crtMacCmdIndex < MAX_NB_CMD_TO_PROCESS)
    {
        loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = LINK_CHECK_CID;
        loRa.crtMacCmdIndex ++;
        status = LORAWAN_SUCCESS;
    }
    return status;
}

// if LORAWAN_ForceEnable is sent, the Silent Immediately bit sent by the end device is discarded and transmission is possible again
void LORAWAN_ForceEnable (void)
{
    loRa.macStatus.silentImmediately = DISABLED;
}

void LorawanReceiveWindow1Callback (void)
{	

    if(loRa.macStatus.macPause == DISABLED)
    {
        if ((CLASS_C == loRa.edClass) && (loRa.macStatus.networkJoined == true))
        {
			loRa.enableRxcWindow = false;
            LorawanClasscReceiveWindowCallback();
        }

        RadioReceiveParam_t RadioReceiveParam;

        loRa.macStatus.macState = RX1_OPEN;
        ConfigureRadioRx(loRa.receiveWindow1Parameters.dataRate, loRa.receiveWindow1Parameters.frequency);

        RadioReceiveParam.action = RECEIVE_START;
        LORAREG_GetAttr(RX_WINDOW_SIZE,&(loRa.receiveWindow1Parameters.dataRate),&(RadioReceiveParam.rxWindowSize));
        if (ERR_NONE != RADIO_Receive(&RadioReceiveParam))
		{
			SYS_ASSERT_ERROR(ASSERT_MAC_RX1CALLBACK_RXFAIL);
		}
    }
    else
    {
        //This should not happen
        SYS_ASSERT_ERROR(0);
    }
}

void LorawanReceiveWindow2Callback(void)
{

    // Make sure the radio is not currently receiving (because a long packet is being received on RX window 1
    if (loRa.macStatus.macPause == DISABLED)
    {   
		 if ((CLASS_C == loRa.edClass) && (loRa.macStatus.networkJoined == true))
		 {
			 loRa.enableRxcWindow = false;
			 //MAC State RX1_OPEN means A packet is currently being received in RX1 Window
			 if (RX1_OPEN != loRa.macStatus.macState)
			 {
				 LorawanClasscReceiveWindowCallback();
			 }
		 }
		 
        if(RADIO_GetState() == RADIO_STATE_IDLE)
        {
            loRa.macStatus.macState = RX2_OPEN;
            LorawanConfigureRadioForRX2(true);
        }
        else
        {
            // A packet is currently being received in RX1 Window
            //This flag is used when the reception in RX1 is overlapping the opening of RX2
            loRa.rx2DelayExpired = 1;
        }
    }
    else
    {
        //Transceiver standalone
        //MAC transmission ended during radio standalone function
        if (loRa.lorawanMacStatus.joining == 1)
        {
            loRa.lorawanMacStatus.joining = 0;
            loRa.macStatus.networkJoined = 0;
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
        }

        ResetParametersForUnconfirmedTransmission();
        ResetParametersForConfirmedTransmission();
		MacClearCommands();
    }
}

void LorawanLinkCheckCallback (void)
{
    uint8_t iCtr = 0;

    if ((loRa.macStatus.macPause == DISABLED) && (loRa.macStatus.linkCheck == ENABLED))
    {
        // Check to see if there are other LINK_CHECK_CID added. In case there are, do nothing
        for(iCtr = 0; iCtr < loRa.crtMacCmdIndex; iCtr ++)
        {
            if(loRa.macCommands[iCtr].receivedCid == LINK_CHECK_CID)
            {
                break;
            }
        }

        if(iCtr == loRa.crtMacCmdIndex)
        {
            loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = LINK_CHECK_CID;

            if(loRa.crtMacCmdIndex < MAX_NB_CMD_TO_PROCESS)
            {
                loRa.crtMacCmdIndex ++;
            }
        }
    }

    //Set link check timeout to the configured interval
    if (loRa.macStatus.linkCheck == ENABLED)
    {
        SwTimerStart(loRa.linkCheckTimerId, MS_TO_US(loRa.periodForLinkCheck), SW_TIMEOUT_RELATIVE, (void *)LorawanLinkCheckCallback, NULL);
    }
}

void LorawanGetChAndInitiateRadioTransmit(void)
{
    radioConfig_t radioConfig;
    RadioTransmitParam_t RadioTransmitParam;
    NewTxChannelReq_t newTxChannelReq;

    newTxChannelReq.transmissionType = true;
    newTxChannelReq.txPwr = loRa.txPower;
    newTxChannelReq.currDr = loRa.currentDataRate;

    // if transmission was not possible, we must wait another ACK timeout seconds period of time to initiate a new transmission
    if (CLASS_A == loRa.edClass)
    {
        loRa.macStatus.macState = RETRANSMISSION_DELAY;
    }

    
    if (LORAREG_GetAttr (NEW_TX_CHANNEL_CONFIG,&newTxChannelReq,&radioConfig) == LORAWAN_SUCCESS)
    {
        if (CLASS_C == loRa.edClass)
        {
			loRa.enableRxcWindow = false;
            RadioReceiveParam_t RadioReceiveParam;
            
			RadioReceiveParam.action = RECEIVE_STOP;
			if (ERR_NONE != RADIO_Receive(&RadioReceiveParam))
			{
				SYS_ASSERT_ERROR(ASSERT_MAC_TXRETRY_RXSTOPFAIL);
			}
		}


        ConfigureRadioTx(radioConfig);
        RadioTransmitParam.bufferLen = loRa.lastPacketLength;
        RadioTransmitParam.bufferPtr = &macBuffer[16];
        //resend the last packet
        if (RADIO_Transmit (&RadioTransmitParam) == ERR_NONE)
        {
            loRa.macStatus.macState = TRANSMISSION_OCCURRING;
        }
        else
        {
            SwTimerStart(loRa.transmissionErrorTimerId, MS_TO_US(TRANSMISSION_ERROR_TIMEOUT - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)TransmissionErrorCallback, NULL);
        }
    }
    else
    {		
		uint32_t minim;
        // if transmission was not possible, we must wait another ACK timeout seconds period of time to initiate a new transmission
        if(loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
        {
            LORAREG_GetAttr(MIN_DUTY_CYCLE_TIMER,&loRa.currentDataRate,&minim);
            if(minim != UINT32_MAX)
            {
                minim = minim + 20;
            }            
            SwTimerStart (loRa.unconfirmedRetransmisionTimerId, MS_TO_US(minim - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)UnconfirmedTransmissionCallback, NULL);
			
        }
		else if(loRa.featuresSupported & LBT_SUPPORT)
		{
            LORAREG_GetAttr(MIN_LBT_CHANNEL_PAUSE_TIMER,&loRa.currentDataRate,&minim);
            if(minim != UINT32_MAX)
            {
	            minim = minim + 1;
            }
           SwTimerStart (loRa.unconfirmedRetransmisionTimerId, MS_TO_US(minim - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)UnconfirmedTransmissionCallback, NULL);
		}
        else
        {
            if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == DISABLED)
            {
				SwTimerStart (loRa.unconfirmedRetransmisionTimerId, MS_TO_US(TRANSMISSION_ERROR_TIMEOUT - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)UnconfirmedTransmissionCallback, NULL);
            }
            else
            {
                UpdateRetransmissionAckTimeoutState ();
            }
        }     
    }   
}

void AckRetransmissionCallback (void)
{
	uint16_t maximumPacketSize = 0;
    if (loRa.macStatus.macPause == DISABLED)
    {
        if (CLASS_C == loRa.edClass)
        {
            LorawanCheckAndDoRetryOnTimeout();
        }
        else if (CLASS_A == loRa.edClass)
        {
            if ((loRa.counterRepetitionsConfirmedUplink <= loRa.maxRepetitionsConfirmedUplink) && (loRa.retransmission == ENABLED))
            {
				  maximumPacketSize = LorawanGetMaxPayloadSize (loRa.currentDataRate) + HDRS_MIC_PORT_MIN_SIZE;
				  // after changing the dataRate, we must check if the size of the packet is still valid
				  if (loRa.lastPacketLength <= maximumPacketSize)
				  {
				       LorawanGetChAndInitiateRadioTransmit();
				  }
				  else
				  {
					  ResetParametersForConfirmedTransmission ();
					  MacClearCommands();
					  UpdateTransactionCompleteCbParams(LORAWAN_INVALID_BUFFER_LENGTH);
				  }
            }
            else 
            {
                ResetParametersForConfirmedTransmission ();
				MacClearCommands();
                UpdateTransactionCompleteCbParams(LORAWAN_NO_ACK); //received  retry failure
            }
        }
    }
    else
    {
        ResetParametersForConfirmedTransmission ();
		MacClearCommands();
    }
}

void UnconfirmedTransmissionCallback (void)
{
    RadioTransmitParam_t RadioTransmitParam;
    radioConfig_t radioConfig;
    RadioTransmitParam.bufferLen = loRa.lastPacketLength;
    RadioTransmitParam.bufferPtr = &macBuffer[16];
    NewTxChannelReq_t newTxChannelReq;

    newTxChannelReq.transmissionType = true;
    newTxChannelReq.txPwr = loRa.txPower;
    newTxChannelReq.currDr = loRa.currentDataRate;
		
    //resend the last packet if the radio transmit function succeeds
    if (LORAREG_GetAttr (NEW_TX_CHANNEL_CONFIG,&newTxChannelReq,&radioConfig) == LORAWAN_SUCCESS)
    {
		RadioReceiveParam_t RadioReceiveParam;
	    /* Stop radio receive before transmission */
	    RadioReceiveParam.action = RECEIVE_STOP;
		RADIO_Receive(&RadioReceiveParam);
		
        ConfigureRadioTx(radioConfig);
        if (RADIO_Transmit (&RadioTransmitParam) != ERR_NONE)
        {
            if (CLASS_A == loRa.edClass)
            {
                loRa.macStatus.macState = RETRANSMISSION_DELAY;
            }

            SwTimerStart(loRa.transmissionErrorTimerId, MS_TO_US(TRANSMISSION_ERROR_TIMEOUT - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)TransmissionErrorCallback, NULL);
        }
    }
    else
    {
        uint32_t minim = RETRANSMIT_TIMEOUT;

        if (loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
        {
            LORAREG_GetAttr(MIN_DUTY_CYCLE_TIMER,&loRa.currentDataRate,&minim);
            if(minim != UINT32_MAX)
            {
                minim = minim + 20;
            }			
        }
		else if(loRa.featuresSupported & LBT_SUPPORT)
		{
			LORAREG_GetAttr(MIN_LBT_CHANNEL_PAUSE_TIMER,&loRa.currentDataRate,&minim);
			if(minim != UINT32_MAX)
			{
				minim = minim + 1;
			}			
		}		

		SwTimerStart(loRa.unconfirmedRetransmisionTimerId, MS_TO_US(minim - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)UnconfirmedTransmissionCallback, NULL);

	}
}

void AutomaticReplyCallback (void)
{

    if (CLASS_A == loRa.edClass)
    {
        loRa.macStatus.macState = IDLE;
    }
    else if (CLASS_C == loRa.edClass)
    {
        if (RADIO_STATE_RX != RADIO_GetState())
        {
            SYS_ASSERT_ERROR(ASSERT_MAC_AUTOREPLY_RXSTATEFAIL);
        }
    }

	LORAWAN_Send (NULL);  // send an empty unconfirmed packet
	loRa.lorawanMacStatus.fPending = DISABLED; //clear the fPending flag
}

void UpdateCurrentDataRate (uint8_t valueNew)
{
    loRa.currentDataRate = valueNew;
	PDS_STORE(PDS_MAC_CURR_DR);
}

void UpdateDLSettings(uint8_t dlRx2Dr, uint8_t dlRx1DrOffset)
{
    if (LORAREG_ValidateAttr(RX2_DATARATE,&dlRx2Dr) == LORAWAN_SUCCESS)
    {
        loRa.receiveWindow2Parameters.dataRate = dlRx2Dr;
		loRa.receiveWindowCParameters.dataRate = dlRx2Dr;

		PDS_STORE(PDS_MAC_RX2_PARAMS); 
		PDS_STORE(PDS_MAC_RXC_PARAMS); 
    }

    if (LORAREG_ValidateAttr(RX1_DATARATE_OFFSET,&dlRx1DrOffset) == LORAWAN_SUCCESS)
    {
        // update the offset between the uplink data rate and the downlink data rate used to communicate with the end device on the first reception slot
        loRa.offset = dlRx1DrOffset;
		PDS_STORE(PDS_MAC_RX1_OFFSET);  
    }
}


void UpdateTxPower (uint8_t txPowerNew)
{
    loRa.txPower = txPowerNew;
	PDS_STORE(PDS_MAC_TX_POWER);
}

void UpdateRetransmissionAckTimeoutState (void)
{
    if (CLASS_A == loRa.edClass)
    {
        loRa.macStatus.macState = RETRANSMISSION_DELAY;
    }
    SwTimerStart(loRa.ackTimeoutTimerId, MS_TO_US(loRa.protocolParameters.retransmitTimeout - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)AckRetransmissionCallback, NULL);
}

void UpdateJoinSuccessState(void)
{
    loRa.lorawanMacStatus.joining = 0;  //join was done
    loRa.macStatus.networkJoined = 1;   //network is joined
	PDS_STORE(PDS_MAC_LORAWAN_STATUS);
    loRa.fCntUp.value = 0;   // uplink counter becomes 0
	PDS_STORE(PDS_MAC_FCNT_UP);
	if(loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
	{
	loRa.joinreqinfo.isFirstJoinReq=false;
	loRa.joinreqinfo.firstJoinReqTimestamp= 0;
	/* to joinreq add*/
	UpdateJoinDutyCycleTimer_t  UpdateJoinDutyCycleTimer ;
	UpdateJoinDutyCycleTimer.startJoinDutyCycleTimer = false;
	LORAREG_SetAttr(JOIN_DUTY_CYCLE_TIMER,&UpdateJoinDutyCycleTimer);
	bool startJoinBackOffTimer = false;
	LORAREG_SetAttr(JOIN_BACK_OFF_TIMER,&startJoinBackOffTimer);
	}
    loRa.fCntDown.value = 0; // downlink counter becomes 0
	PDS_STORE(PDS_MAC_FCNT_DOWN);
    loRa.adrAckCnt = 0;  // adr ack counter becomes 0, it increments only for ADR set
    loRa.counterAdrAckDelay = 0;

	loRa.macStatus.macState = IDLE;
	if (!loRa.joinAcceptChMaskReceived)
	{
		LORAREG_SetAttr(REG_JOIN_SUCCESS,NULL);
	}
	
	// if the link check mechanism was enabled, then its timer will begin counting
    if (loRa.macStatus.linkCheck == ENABLED)
    {
        SwTimerStart(loRa.linkCheckTimerId, MS_TO_US(loRa.periodForLinkCheck), SW_TIMEOUT_RELATIVE, (void *)LorawanLinkCheckCallback, NULL);
    }
    if (AppPayload.JoinResponse != NULL)
    {
        AppPayload.JoinResponse(LORAWAN_SUCCESS); // inform the application layer that join was successful via a callback
    }
}


void UpdateReceiveWindow2Parameters (uint32_t frequency, uint8_t dataRate)
{
    loRa.receiveWindow2Parameters.dataRate = dataRate;
    loRa.receiveWindow2Parameters.frequency = frequency;
   if ((loRa.edClass == CLASS_C) || (loRa.edClass == CLASS_B))
   {
      UpdateReceiveWindowCParameters(frequency , dataRate);
   }
	PDS_STORE(PDS_MAC_RX2_PARAMS); 
}

void UpdateReceiveWindowCParameters(uint32_t frequency , uint8_t dataRate)
{
	loRa.receiveWindowCParameters.dataRate = dataRate;
	loRa.receiveWindowCParameters.frequency = frequency;
	PDS_STORE(PDS_MAC_RXC_PARAMS); 
}

void ResetParametersForConfirmedTransmission (void)
{
    if (CLASS_A  == loRa.edClass)
    {
        loRa.macStatus.macState = IDLE;
    }

    loRa.counterRepetitionsConfirmedUplink = DEF_CNF_UL_REPT_CNT;
    loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage = DISABLED;
 }


void ResetParametersForUnconfirmedTransmission (void)
{
    if (CLASS_A  == loRa.edClass)
    {
        loRa.macStatus.macState = IDLE;
    }

    loRa.counterRepetitionsUnconfirmedUplink = DEF_UNCNF_UL_REPT_CNT;
}

void MacClearCommands(void)
{
  uint8_t i = 0;

  for(i = 0; i < loRa.crtMacCmdIndex ; i++)
  {
	  if(loRa.macCommands[i].receivedCid != INVALID_VALUE)
	  { 
		  switch (loRa.macCommands[i].receivedCid)
		  {
			  case RX2_SETUP_CID:
			  case DL_CHANNEL_CID:
			  case RX_TIMING_SETUP_CID: 
			  {
				  //Do Nothing.The MAC command needs to be processed again until we receive a DL
			  }
			  break;

			  default:
			  {
				  //Other Mac commands can be cleared.The complete list will be cleared when we receive a DL again
				  loRa.macCommands[i].receivedCid = INVALID_VALUE;
			  }
			  break;
			}

	  }
  }
}

void SetJoinFailState(StackRetStatus_t status)
{
    loRa.macStatus.networkJoined = 0;
    loRa.lorawanMacStatus.joining = 0;
    loRa.macStatus.macState = IDLE;
	if(loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
	{
		loRa.joinreqinfo.isFirstJoinReq = false;
	}
	
    if (AppPayload.JoinResponse != NULL)
    {
        
		AppPayload.JoinResponse(status); // inform the application layer that join failed via callback
		
    }
    loRa.rx2DelayExpired = 0;
	PDS_STORE(PDS_MAC_LORAWAN_STATUS);
}

//Generates a 16-bit random number based on some measurements made by the radio (seed is initialized inside mac Reset)
uint16_t Random (uint16_t max)
{
    return (rand () % max);
}

/*********************************************************************//**
\brief	This function handles transmitting ack.
*************************************************************************/
static void HandleAutoReplyFromRxDone(void)
{
    NewFreeChannelReq_t newFreeChannelReq;
    uint8_t channelIndex;

    newFreeChannelReq.maxChannels = loRa.maxChannels;
    newFreeChannelReq.transmissionType = true;
    newFreeChannelReq.currDr = loRa.currentDataRate;
    if (LORAREG_GetAttr (FREE_CHANNEL,&newFreeChannelReq, &channelIndex) == LORAWAN_SUCCESS)
    {
        LORAWAN_Send (NULL);  // send an empty unconfirmed packet
        loRa.lorawanMacStatus.fPending = DISABLED; //clear the fPending flag
    }
    else
    {		
		uint32_t minim;
        //searches for the minimum channel timer and starts a software timer for it
        if(loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
        {
            LORAREG_GetAttr(MIN_DUTY_CYCLE_TIMER,&loRa.currentDataRate,&minim);
			if(minim != UINT32_MAX)
			{
				minim = minim + 20;
			}
            loRa.macStatus.macState = RETRANSMISSION_DELAY;
            SwTimerStart (loRa.automaticReplyTimerId, MS_TO_US(minim - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)AutomaticReplyCallback, NULL);

        }
		else if(loRa.featuresSupported & LBT_SUPPORT)
		{
			LORAREG_GetAttr(MIN_LBT_CHANNEL_PAUSE_TIMER,&loRa.currentDataRate,&minim);			
			loRa.macStatus.macState = RETRANSMISSION_DELAY;
			if(minim != UINT32_MAX)
			{
				minim = minim + 1;
			}
			SwTimerStart (loRa.unconfirmedRetransmisionTimerId, MS_TO_US(minim - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)UnconfirmedTransmissionCallback, NULL);
					
		}
    }
}

static void LorawanNotifyAppOnRxdone(Hdr_t *hdr, uint8_t *packet, uint8_t frmPayloadLength)
{
	if (CLASS_A == loRa.edClass)
	{
		loRa.macStatus.macState = IDLE;
	}
	else if (CLASS_C == loRa.edClass)
	{
	    // ACK Timeout timer will run in case of Class C device
		SwTimerStop(loRa.ackTimeoutTimerId);
	}

	if (AppPayload.AppData != NULL)
	{
		loRa.lorawanMacStatus.syncronization = 0; //clear the syncronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
		if (CLASS_C == loRa.edClass)
		{
			LorawanClasscNotifyAppOnReceive(hdr->members.devAddr.value, packet, frmPayloadLength, LORAWAN_SUCCESS);
		}
		else
		{
			/* Send RX Available Event */
			UpdateRxDataAvailableCbParams(hdr->members.devAddr.value, packet, frmPayloadLength, LORAWAN_SUCCESS);
		}

		if (!loRa.isTransactionDone)
		{
			/* Transaction complete Event */
			UpdateTransactionCompleteCbParams(LORAWAN_SUCCESS);
		}
	}
}

static StackRetStatus_t ProcessUnicastRxPacket(uint8_t* buffer, uint8_t bufferLength, Hdr_t *hdr)
{
    uint8_t frmPayloadLength;
    uint8_t bufferIndex;
    uint8_t fPort = 0;
    uint8_t *appskey = loRa.activationParameters.applicationSessionKeyRam;
	uint8_t *nwkskey = loRa.activationParameters.networkSessionKeyRam;
    uint32_t extractedMic;
    uint8_t *packet;
	bool macCommandReceived = false;
	SalStatus_t sal_status = SAL_SUCCESS;

    loRa.counterRepetitionsUnconfirmedUplink = DEF_UNCNF_UL_REPT_CNT; // this is a guard for LORAWAN_RxTimeout, for any packet that is received, the last uplink packet should not be retransmitted

    CheckFlags (hdr);

    if (hdr->members.fCtrl.fOptsLen != 0)
    {
        if (0 != *(uint8_t *)(buffer + 8 + hdr->members.fCtrl.fOptsLen)) 
        {
	        buffer = MacExecuteCommands(hdr->members.MacCommands, hdr->members.fCtrl.fOptsLen);
	        macCommandReceived = true;
        }
        else
        {			
			 /* Stop RX2 Window Timer if it is running 
				since the packet already received in RX1 state */
			if ((loRa.macStatus.macState == RX1_OPEN))
			{
				StopReceiveWindow2Timer();
			}
	        //Reject the payload.
	        // Since the MAC commands are present in both fOpts and Frame payload.
	        // LoRaWAN Specification 1.0.1 section 4.3.1.6
	        // Also clear loRa.crtMacCmdIndex, which would have been processes in if //(hdr->members.fCtrl.fOptsLen != 0)
			if ((AppPayload.AppData != NULL))
			{
				loRa.lorawanMacStatus.syncronization = 0; //clear the syncronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
				/* Transaction complete Event */
				UpdateTransactionCompleteCbParams(LORAWAN_INVALID_PACKET);// invalid frame ctr.

			}
			loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = DISABLED;
	        loRa.crtMacCmdIndex = 0;
	        SetReceptionNotOkState();
	        return LORAWAN_INVALID_PARAMETER;
        }
    }
    else
    {
        buffer = buffer + 8;  // 8 bytes for size of header without mac commands
    }
    if ( (sizeof(extractedMic) + hdr->members.fCtrl.fOptsLen + 8) != bufferLength)     //we have port and FRM Payload in the reception packet
    {
        fPort = *(buffer++);

        frmPayloadLength = bufferLength - 8 - hdr->members.fCtrl.fOptsLen - sizeof (extractedMic); //frmPayloadLength includes port
        bufferIndex = 16 + 8 + hdr->members.fCtrl.fOptsLen + sizeof (fPort);

        if (fPort != 0)
        {
            sal_status = EncryptFRMPayload (buffer, frmPayloadLength - 1, 1, loRa.fCntDown.value, appskey, SAL_APPS_KEY, bufferIndex, radioBuffer, loRa.activationParameters.deviceAddress.value);
            if (SAL_SUCCESS != sal_status)
			{
				SetReceptionNotOkState();
				/* Transaction complete Event */
				UpdateTransactionCompleteCbParams(LORAWAN_RXPKT_ENCRYPTION_FAILED);
			}
			packet = buffer - 1;
        }
        else if ((fPort == 0) && (frmPayloadLength > 0))
        {
			if(hdr->members.fCtrl.fOptsLen == 0)
			{
                // Decrypt port 0 payload
                sal_status = EncryptFRMPayload (buffer, frmPayloadLength - 1, 1, loRa.fCntDown.value, nwkskey, SAL_NWKS_KEY, bufferIndex, radioBuffer, loRa.activationParameters.deviceAddress.value);
                if (SAL_SUCCESS != sal_status)
                {
	                SetReceptionNotOkState();
	                /* Transaction complete Event */
	                UpdateTransactionCompleteCbParams(LORAWAN_RXPKT_ENCRYPTION_FAILED);
                }
				buffer = MacExecuteCommands(buffer, frmPayloadLength - 1 );
				macCommandReceived = true;
								
			}
			else 
			{
				/* Stop RX2 Window Timer if it is running 
				   since the packet already received in RX1 state */
				if ((loRa.macStatus.macState == RX1_OPEN))
				{
					StopReceiveWindow2Timer();
				} 

				//Reject the payload.
				// Since the MAC commands are present in both fOpts and Frame payload.
				// LoRaWAN Specification 1.0.1 section 4.3.1.6
				// Also clear loRa.crtMacCmdIndex, which would have been processes in if //(hdr->members.fCtrl.fOptsLen != 0)
				if ((AppPayload.AppData != NULL))
				{
					loRa.lorawanMacStatus.syncronization = 0; //clear the syncronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
					/* Transaction complete Event */
					UpdateTransactionCompleteCbParams(LORAWAN_INVALID_PACKET);// invalid frame ctr.

				}
				loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = DISABLED;
				loRa.crtMacCmdIndex = 0;
				SetReceptionNotOkState();
				return LORAWAN_INVALID_PARAMETER;
			}

            frmPayloadLength = 0;  // we have only MAC commands, so no application payload
            packet = NULL;
        }
		else
		{
			//fport==0 and frmPayloadLength =0 -> Mostly Wont happen
			frmPayloadLength = 0;  
            packet = NULL;
		}
    }
    else
    {
        frmPayloadLength = 0;
        packet = NULL;
    }

    loRa.adrAckCnt = 0; // if a packet comes and is correct after device address and MIC, the counter will start counting again from 0 (any received downlink frame following an uplink frame resets the ADR_ACK_CNT counter)
    loRa.counterAdrAckDelay = 0; // if a packet was received, the counter for adr ack limit will become 0
    loRa.lorawanMacStatus.adrAckRequest = 0; //reset the flag for ADR ACK request

    loRa.macStatus.rxDone = 1;  //packet is ready for reception for the application layer

    // if the downlink message was received during receive window 1, receive window 2 should not open any more, so its timer will be stopped
    if ((loRa.macStatus.macState == RX1_OPEN))
    {
        SwTimerStop (loRa.receiveWindow2TimerId);
    }

    if ( loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == 1 ) //if last uplink packet was confirmed;
    {
        /* if ACK was received */
        if (hdr->members.fCtrl.ack == ENABLED) 
        {
            ResetParametersForConfirmedTransmission();
            loRa.macStatus.rxDone = 0;
			if(fPort != 0)
			{
				LorawanNotifyAppOnRxdone(hdr, packet, frmPayloadLength);
			}			
			else
			{
				loRa.lorawanMacStatus.syncronization = 0;
				if (CLASS_A  == loRa.edClass)
				{
					loRa.macStatus.macState = IDLE;
				}
				else if (CLASS_C == loRa.edClass)
				{
					// ACK Timeout timer will run in case of Class C device
					SwTimerStop(loRa.ackTimeoutTimerId);
				}
				if (!loRa.isTransactionDone)
				{
					/* Transaction complete Event */
					UpdateTransactionCompleteCbParams(LORAWAN_SUCCESS);
				}
			}
        }
		/* If MAC commands were received in either fopts/framepayload field without ACK bit, then process the packet*/
		else if ((hdr->members.fCtrl.ack == DISABLED) && (true == macCommandReceived) && (CLASS_A == loRa.edClass))
		{
			ResetParametersForConfirmedTransmission();
			//Resetting the flags
			loRa.macStatus.rxDone = 0;
			loRa.lorawanMacStatus.syncronization = 0;
			macCommandReceived = false;
			if (!loRa.isTransactionDone)
			{
				/* Transaction complete Event */
				UpdateTransactionCompleteCbParams(LORAWAN_SUCCESS);
			}
			
		}
        /* if ACK was required, but not received, then retransmission will happen for Class C device alone.*/
        else
        {
            UpdateRetransmissionAckTimeoutState ();
        }
    }
    else
    {
		if(fPort != 0)
		{
			LorawanNotifyAppOnRxdone(hdr, packet, frmPayloadLength);
		}
		else
		{
			loRa.lorawanMacStatus.syncronization = 0;
			if (CLASS_A  == loRa.edClass)
			{
				loRa.macStatus.macState = IDLE;
			}	
			else if (CLASS_C == loRa.edClass)
			{
				// ACK Timeout timer will run in case of Class C device
				SwTimerStop(loRa.ackTimeoutTimerId);
			}			
			if (!loRa.isTransactionDone)
			{
				/* Transaction complete Event */
				UpdateTransactionCompleteCbParams(LORAWAN_SUCCESS);
			}
		}
        /* Reset rxDone flag as the application is notified of the reception.
          (if it had AppData() callback) */
        loRa.macStatus.rxDone = 0;
    }
	
	if ( (loRa.macStatus.automaticReply == 1) && (loRa.lorawanMacStatus.syncronization == 0) && ( (loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage == 1) || (loRa.lorawanMacStatus.fPending == ENABLED) ) )
	{
		HandleAutoReplyFromRxDone();
	}
	else
	{
		if (CLASS_C == loRa.edClass)
		{
			LorawanClasscRxDone(hdr);
		}
	}

	return LORAWAN_SUCCESS;
}

StackRetStatus_t LorawanProcessFcntDown(Hdr_t *hdr, bool isMulticast)
{
	if (hdr->members.fCnt >= loRa.fCntDown.members.valueLow)
	{
		//  frame counter check, frame counter received should be less than last frame counter received, otherwise it  was an overflow
		
		    if(hdr->members.fCnt == loRa.fCntDown.members.valueLow)
		    {
			    loRa.lorawanMacStatus.retransmittedownlink = true;
		    }
			loRa.fCntDown.members.valueLow = hdr->members.fCnt;  //frame counter received is OK, so the value received from the server is kept in sync with the value stored in the end device
					/* If maxFcntPdsUpdateValue is '0', means every-time the Frame counter will be updated in PDS */
            if((0 == loRa.maxFcntPdsUpdateValue) || (0 == (loRa.fCntDown.value & ((1 << loRa.maxFcntPdsUpdateValue) - 1))))
			{
				PDS_STORE(PDS_MAC_FCNT_DOWN);
			}
		
	}
	else
	{		
			//Frame counter rolled over
			loRa.fCntDown.members.valueLow = hdr->members.fCnt;
			loRa.fCntDown.members.valueHigh ++;
			if((0 == loRa.maxFcntPdsUpdateValue) || (0 == (loRa.fCntDown.value & ((1 << loRa.maxFcntPdsUpdateValue) - 1))))
			{
				PDS_STORE(PDS_MAC_FCNT_DOWN);
			}			
		
		
	    if (loRa.fCntDown.value == FCNT_MAX)
	    {
		    // Inform application about rejoin in status
		    loRa.macStatus.rejoinNeeded = 1;
		    SetReceptionNotOkState();
		    PDS_STORE(PDS_MAC_LORAWAN_STATUS);
		    return LORAWAN_FCNTR_ERROR_REJOIN_NEEDED;
	    }        
    }
    return LORAWAN_SUCCESS;
}

StackRetStatus_t LORAWAN_RxDone (uint8_t *buffer, uint8_t bufferLength)
{
    uint32_t computedMic, extractedMic;
    Mhdr_t mhdr;
    uint8_t temp;
	uint8_t groupId;
    uint32_t fcntDown_temp;
	SalStatus_t sal_status = SAL_SUCCESS;
    uint32_t jNonce;

    if (loRa.macStatus.macPause == DISABLED)
    {
        mhdr.value = buffer[0];
        if ((mhdr.bits.mType == FRAME_TYPE_JOIN_ACCEPT) && (loRa.activationParameters.activationType == 0) && (loRa.lorawanMacStatus.joining == 1))
        {
             temp = bufferLength - 1; //MHDR not encrypted
             while (temp > 0)
             {
	             //Decode message
	             sal_status = SAL_AESEncode (&buffer[bufferLength - temp], SAL_APP_KEY, loRa.activationParameters.applicationKey);
				 if (SAL_SUCCESS != sal_status)
				 {
					 SetJoinFailState(sal_status);
					 SetReceptionNotOkState();
					 return LORAWAN_RXPKT_ENCRYPTION_FAILED;
				 }
				 
	             if (temp > AES_BLOCKSIZE)
	             {
		             temp -= AES_BLOCKSIZE;
	             }
	             else
	             {
		             temp = 0;
	             }
             }
         
            //verify MIC
            computedMic = ComputeMic (loRa.activationParameters.applicationKey, buffer, bufferLength - sizeof(extractedMic));
            extractedMic = ExtractMic (buffer, bufferLength);
            if (extractedMic != computedMic)
            {
                if ((loRa.macStatus.macState == RX2_OPEN) || ((loRa.macStatus.macState == RX1_OPEN) && (loRa.rx2DelayExpired)))
                {
                    SetJoinFailState(LORAWAN_MIC_ERROR);
                }
				SetReceptionNotOkState();
                return LORAWAN_INVALID_PARAMETER;
            }

            // if the join request message was received during receive window 1, receive window 2 should not open any more, so its timer will be stopped
            if (loRa.macStatus.macState == RX1_OPEN)
            {
                SwTimerStop (loRa.joinAccept2TimerId);
            }

            JoinAccept_t *joinAccept;
            joinAccept = (JoinAccept_t*)buffer;
            
            if (loRa.joinNonceType == JOIN_NONCE_INCREMENTAL)
            {
                jNonce = 0x00000000 | ((uint32_t) joinAccept->members.joinNonce[0]);
                jNonce |= (uint32_t) ((uint32_t) joinAccept->members.joinNonce[1]) << 8;
                jNonce |= (uint32_t) ((uint32_t) joinAccept->members.joinNonce[2]) << 16;

                if (MAC_JOINNONCE != loRa.joinNonce)
                {
                    if (jNonce <= loRa.joinNonce)
                    {
                        SetJoinFailState(LORAWAN_JOIN_NONCE_ERROR);
                        return LORAWAN_JOIN_NONCE_ERROR;
                    }
                }
                loRa.joinNonce = jNonce;
                PDS_STORE(PDS_MAC_JOIN_NONCE);
            }

            loRa.activationParameters.deviceAddress.value = joinAccept->members.deviceAddress.value; //device address is saved
			PDS_STORE(PDS_MAC_DEV_ADDR);
            UpdateReceiveDelays (joinAccept->members.rxDelay & LAST_NIBBLE); //receive delay 1 and receive delay 2 are updated according to the rxDelay field from the join accept message

            UpdateDLSettings(joinAccept->members.DLSettings.bits.rx2DataRate, joinAccept->members.DLSettings.bits.rx1DROffset);
			
			/* Reset the flag before checking whether CFList contains CHMask */
			loRa.joinAcceptChMaskReceived = false;

            UpdateCfList (bufferLength, joinAccept);

            ComputeSessionKeys (joinAccept); //for activation by personalization, the network and application session keys are computed
			
			if (loRa.cryptoDeviceEnabled)
			{
				sal_status = SAL_Read(SAL_NWKS_KEY, (uint8_t *)&loRa.activationParameters.networkSessionKeyRam);
				if (SAL_SUCCESS != sal_status)
				{
					SetJoinFailState(sal_status);
					SetReceptionNotOkState();
					return LORAWAN_SKEY_READ_FAILED;
				}
				sal_status = SAL_Read(SAL_APPS_KEY, (uint8_t *)&loRa.activationParameters.applicationSessionKeyRam);
				if (SAL_SUCCESS != sal_status)
				{
					SetJoinFailState(sal_status);
					SetReceptionNotOkState();
					return LORAWAN_SKEY_READ_FAILED;
				}
			}
			else
			{
				memcpy(loRa.activationParameters.applicationSessionKeyRam, loRa.activationParameters.applicationSessionKeyRom, 16);
				memcpy(loRa.activationParameters.networkSessionKeyRam, loRa.activationParameters.networkSessionKeyRom, 16);
			}
            UpdateJoinSuccessState();

            return LORAWAN_SUCCESS;
        }
        else if (((mhdr.bits.mType == FRAME_TYPE_DATA_UNCONFIRMED_DOWN) || (mhdr.bits.mType == FRAME_TYPE_DATA_CONFIRMED_DOWN)) && (loRa.macStatus.networkJoined == 1))
        {
            bool isMcastpkt = false;
            uint8_t *nwkskey = loRa.activationParameters.networkSessionKeyRam;
            uint32_t devAddr = loRa.activationParameters.deviceAddress.value;

            loRa.crtMacCmdIndex = 0;   // clear the macCommands requests list

            Hdr_t *hdr;
            hdr=(Hdr_t*)buffer;
			
			if (checkRxPacketPayloadLen(bufferLength, hdr) != LORAWAN_SUCCESS)
			{
				//Received a oversized packet, Reject it.
				/* Stop RX2 Window Timer if it is running 
					since the packet already received in RX1 state */
				if ((loRa.macStatus.macState == RX1_OPEN))
				{
					StopReceiveWindow2Timer();
				} 

				//Reject the payload.
				// Since the MAC commands are present in both fOpts and Frame payload.
				// LoRaWAN Specification 1.0.1 section 4.3.1.6
				// Also clear loRa.crtMacCmdIndex, which would have been processes in if //(hdr->members.fCtrl.fOptsLen != 0)
				if ((AppPayload.AppData != NULL))
				{
					loRa.lorawanMacStatus.syncronization = 0; //clear the syncronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
					/* Transaction complete Event */
					UpdateTransactionCompleteCbParams(LORAWAN_INVALID_PACKET);// invalid frame ctr.

				}
			loRa.crtMacCmdIndex = 0;
			SetReceptionNotOkState();
			return LORAWAN_INVALID_PARAMETER;			
			
			}

            //  verify if the device address stored in the activation parameters is the same with the device address piggybacked in the received packet, if not ignore packet
            if (hdr->members.devAddr.value != loRa.activationParameters.deviceAddress.value)
            {
                if (LORAWAN_SUCCESS == LorawanMcastValidateHdr(hdr, mhdr.bits.mType, *((uint8_t *)(buffer + LORAWAN_FHDR_SIZE_WITHOUT_FOPTS)),&groupId))
                {
                    isMcastpkt = true;
                    nwkskey = loRa.mcastParams.activationParams[groupId].mcastNwkSKey;
                    devAddr = loRa.mcastParams.activationParams[groupId].mcastDevAddr.value;
                }
                else
                {
					if ((loRa.macStatus.macState == RX2_OPEN) || ( (loRa.macStatus.macState == RX1_OPEN) && (loRa.rx2DelayExpired)))
					{						
						loRa.lorawanMacStatus.syncronization = 0; //clear the syncronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
						/* Transaction complete Event */
						UpdateTransactionCompleteCbParams(LORAWAN_INVALID_PACKET);// invalid frame ctr.
						SetReceptionNotOkState();
					}
                    
                    return LORAWAN_INVALID_PARAMETER;
                }
            }
			
            if ( false == isMcastpkt )
            {   // fcntDn validation            
                fcntDown_temp = loRa.fCntDown.value;
                StackRetStatus_t fCntStatus = LorawanProcessFcntDown(hdr, isMcastpkt);
                if (LORAWAN_SUCCESS != fCntStatus)
                {
                    return fCntStatus;
                }
                AssembleEncryptionBlock (1,(uint32_t)loRa.fCntDown.value, bufferLength - sizeof (computedMic), 0x49, devAddr);
            }
            else
            {
                FCnt_t * mcastfcnt = & loRa.mcastParams.activationParams[groupId].mcastFCntDown;

                fcntDown_temp = mcastfcnt->value;

                if ( hdr->members.fCnt >= mcastfcnt->members.valueLow )
                {
                    /*
                    * 16-bit fcnt received in frame is >= the mcastfcnt.valueLow
                    */
                    mcastfcnt->members.valueLow = hdr->members.fCnt;

                    // update PDS
                }
                else
                {
                    /*
                    * 16-bit fcnt received in frame is < mcastfcnt.valueHigh
                    */
                    mcastfcnt->members.valueHigh++;
                    mcastfcnt->members.valueLow = hdr->members.fCnt;

                    // update PDS
                }
                AssembleEncryptionBlock (1, mcastfcnt->value, bufferLength - sizeof (computedMic), 0x49, devAddr);
            }
			
            memcpy (&radioBuffer[0], aesBuffer, sizeof (aesBuffer));
            memcpy (&radioBuffer[16], buffer, bufferLength-sizeof(computedMic));
			if(isMcastpkt)
			{
				SAL_AESCmac(nwkskey, SAL_MCAST_NWKS_KEY, aesBuffer, &radioBuffer[0], bufferLength - sizeof(computedMic) + sizeof (aesBuffer));
			}
			else
			{
				SAL_AESCmac(nwkskey, SAL_NWKS_KEY, aesBuffer, &radioBuffer[0], bufferLength - sizeof(computedMic) + sizeof (aesBuffer));
			}

            memcpy(&computedMic, aesBuffer, sizeof(computedMic));
            extractedMic = ExtractMic (&buffer[0], bufferLength);

            // verify if the computed MIC is the same with the MIC piggybacked in the packet, if not ignore packet
            if (computedMic != extractedMic)
            {  
				/* Stop RX2 Window Timer if it is running 
				   since the packet already received in RX1 state */
				if ((isMcastpkt == false) && (loRa.macStatus.macState == RX1_OPEN)) //RX2 timer should be stopped if not multicast packet(In rare case of parameter match with RXC window)
				{
					StopReceiveWindow2Timer();
				}
	
                if (AppPayload.AppData != NULL)
                {
	                loRa.lorawanMacStatus.syncronization = 0; //clear the synchronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
					if ( true == isMcastpkt )
                    {
                        loRa.mcastParams.activationParams[groupId].mcastFCntDown.value = fcntDown_temp;
                    }
                    else
                    {
                        loRa.fCntDown.value = fcntDown_temp;
                    }
	                /* Transaction complete Event */
	                UpdateTransactionCompleteCbParams(LORAWAN_MIC_ERROR);
	                 
                }
			    SetReceptionNotOkState();//mic error
                return LORAWAN_INVALID_PARAMETER;
            }

            if (false == isMcastpkt)
            {
				ProcessUnicastRxPacket(buffer, bufferLength, hdr);   
            }
            else
            {
				if ((true == isMcastpkt)  && (loRa.enableRxcWindow == true))
				{
                  LorawanMcastProcessPkt(buffer, bufferLength, hdr, groupId);
				}
				else
				{
					loRa.lorawanMacStatus.syncronization = 0; //clear the synchronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
					/* Transaction complete Event */
					UpdateTransactionCompleteCbParams(LORAWAN_INVALID_PACKET);
					//if the mType is incorrect, set reception not OK state
					SetReceptionNotOkState (); //mtype incorrect
				}
            }
        }
        else
        {
			if ((loRa.macStatus.macState == RX2_OPEN) || ( (loRa.macStatus.macState == RX1_OPEN) && (loRa.rx2DelayExpired)))
			{
				 loRa.lorawanMacStatus.syncronization = 0; //clear the synchronization flag, because if the user will send a packet in the callback there is no need to send an empty packet
				 /* Transaction complete Event */
				 UpdateTransactionCompleteCbParams(LORAWAN_INVALID_MTYPE);
				 //if the mType is incorrect, set reception not OK state
				 SetReceptionNotOkState (); //mtype incorrect
			}			
            return LORAWAN_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Standalone radio reception OK, using the same callback as in LoRaWAN rx */
        if ( AppPayload.AppData != NULL )
        {
            /* RX DATA Available Event */
            if (callbackBackup == RADIO_RX_ERROR_CALLBACK)
            {
                UpdateRxDataAvailableCbParams(0, buffer, bufferLength, LORAWAN_RADIO_BUSY);
            }
            else
            {
                UpdateRxDataAvailableCbParams(0, buffer, bufferLength, LORAWAN_RADIO_SUCCESS);
            }
        }
    }
    return 1;
}


static uint8_t* MacExecuteCommands (uint8_t *buffer, uint8_t fOptsLen)
{
    bool done = false;
    uint8_t *ptr;
    uint8_t *end;
    uint8_t cmdId;
    ptr = buffer;
    end = buffer + fOptsLen;
    while ( (ptr < ( buffer + fOptsLen )) && (done == false) )
    {
        /* Clean structure before using it */
        loRa.macCommands[loRa.crtMacCmdIndex].channelMaskAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].dataRateAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].powerAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].channelAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].dataRateReceiveWindowAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].rx1DROffestAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].dataRateRangeAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].channelFrequencyAck = 0;
        loRa.macCommands[loRa.crtMacCmdIndex].uplinkFreqExistsAck = 0;

        /* Reply has the same value as request */
        loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = *ptr;
        
        cmdId = *ptr;
        ptr++;

        if ((ptr + macEndDevCmdInputLen[cmdId - 2]) <= end)
        {
            switch (cmdId)
            {
                case LINK_CHECK_CID:
                {
                    ptr = ExecuteLinkCheck (ptr );
                    /* No reply to server is needed */
                    loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = INVALID_VALUE;
                }
                break;

                case LINK_ADR_CID:
                {
                    ptr = ExecuteLinkAdr (ptr );
                }
                break;

                case DUTY_CYCLE_CID:
                {
                    ptr = ExecuteDutyCycle(ptr);
                }
                break;

                case RX2_SETUP_CID:
                {
                    ptr = ExecuteRxParamSetupReq (ptr);
                }
                break;

                case DEV_STATUS_CID:
                {
                    ptr = ExecuteDevStatus (ptr);
                }
                break;

                case NEW_CHANNEL_CID:
                {
                    ptr = ExecuteNewChannel (ptr);

                }
                break;

                case RX_TIMING_SETUP_CID:
                {
                    ptr = ExecuteRxTimingSetup (ptr);
                }
                break;
        
		        case TX_PARAM_SETUP_CID:
		        {
		            ptr = ExecuteTxParamSetup (ptr);
		        }
		        break;
		
                case DL_CHANNEL_CID:
                {
                    ptr = ExecuteDlChannel(ptr);
                }
                break;

                case DEV_TIME_CID:
		        {
			        ptr = ExecuteDevTimeAns(ptr);
			        loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = INVALID_VALUE;
		        }
		        break;

                default:
                {
                    done = true;  // Unknown MAC commands cannot be skipped and the first unknown MAC command terminates the processing of the MAC command sequence.
                    ptr = buffer + fOptsLen;
                    loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = INVALID_VALUE;
                } 
                break;

            }
        }
        else
        { // Invalid payload in MAC cmd will terminate the MAC command processing sequence
            done = true;
            ptr = buffer + fOptsLen;
            loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = INVALID_VALUE;
        }

        if((loRa.macCommands[loRa.crtMacCmdIndex].receivedCid != INVALID_VALUE) &&
                (loRa.crtMacCmdIndex < MAX_NB_CMD_TO_PROCESS))
        {
            loRa.crtMacCmdIndex ++;
        }
    }
	
	if(loRa.linkAdrResp.count)
	{
		if ( (loRa.linkAdrResp.powerAck == 1) && (loRa.linkAdrResp.dataRateAck == 1) && (loRa.linkAdrResp.channelMaskAck == 1) )
		{
			
			UpdateLinkAdrCommands(loRa.linkAdrResp.channelMask,loRa.linkAdrResp.redundancy.chMaskCntl,loRa.linkAdrResp.redundancy.nbRep,loRa.linkAdrResp.txPower,loRa.linkAdrResp.dataRate);
		}
				
	}
				
    return ptr;
}

static uint8_t* ExecuteLinkCheck (uint8_t *ptr)
{
    loRa.linkCheckMargin = *(ptr++);
    loRa.linkCheckGwCnt = *(ptr++);
    return ptr;
}

static uint8_t* ExecuteRxTimingSetup (uint8_t *ptr)
{
    uint8_t delay;
    uint8_t rfuBits = 0;
    
    rfuBits = (*ptr) & FIRST_NIBBLE;
    delay = (*ptr) & LAST_NIBBLE;
    ptr++;

    if (0 == rfuBits)
    { /* Process only if RFU bits are non-zero */
        UpdateReceiveDelays (delay);
        loRa.macStatus.rxTimingSetup = ENABLED;
        PDS_STORE(PDS_MAC_LORAWAN_STATUS);
    }   
	
    return ptr;
}

uint8_t* ExecuteRxParamSetupReq (uint8_t *ptr)
{
    DlSettings_t dlSettings;
    uint32_t frequency = 0;
    uint8_t rx2Dr,rx1Offset;
    //In the status field (response) we have to include the following: channel ACK, RX2 data rate ACK, RX1DoffsetACK

    dlSettings.value = *(ptr++);

    memcpy(&frequency, ptr, sizeof(frequency));
    frequency = frequency & 0x00FFFFFF;
    frequency = frequency * 100;
    ptr = ptr + 3; //3 bytes for frequency

    rx2Dr = dlSettings.bits.rx2DataRate;
    rx1Offset = dlSettings.bits.rx1DROffset;

    if (LORAREG_ValidateAttr (RX_FREQUENCY,&frequency) == LORAWAN_SUCCESS)
    {
        loRa.macCommands[loRa.crtMacCmdIndex].channelAck = 1;
    }

    if (LORAREG_ValidateAttr (RX_DATARATE,&rx2Dr) == LORAWAN_SUCCESS)
    {
        loRa.macCommands[loRa.crtMacCmdIndex].dataRateReceiveWindowAck = 1;
    }

    if (LORAREG_ValidateAttr (RX1_DATARATE_OFFSET,&rx1Offset) == LORAWAN_SUCCESS)
    {
        loRa.macCommands[loRa.crtMacCmdIndex].rx1DROffestAck = 1;
    }

    if ( (loRa.macCommands[loRa.crtMacCmdIndex].dataRateReceiveWindowAck == 1) && (loRa.macCommands[loRa.crtMacCmdIndex].channelAck == 1) && (loRa.macCommands[loRa.crtMacCmdIndex].rx1DROffestAck == 1))
    {
        loRa.offset = dlSettings.bits.rx1DROffset;
		PDS_STORE(PDS_MAC_RX1_OFFSET);
        UpdateReceiveWindow2Parameters (frequency, dlSettings.bits.rx2DataRate);
        loRa.macStatus.secondReceiveWindowModified = 1;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);
    }	
	
	
    return ptr;
}


uint8_t* ExecuteDevStatus (uint8_t *ptr)
{
    return ptr;
}

uint8_t* ExecuteDutyCycle (uint8_t *ptr)
{
    uint8_t maxDCycle;

    maxDCycle = *(ptr++);
    if (maxDCycle <= 15)
    {
        loRa.aggregatedDutyCycle =  maxDCycle; // Assign the pre-scalar value
		PDS_STORE(PDS_MAC_PRESCALR);
        loRa.macStatus.prescalerModified = ENABLED;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);
    }

    return ptr;
}



uint8_t* ExecuteNewChannel (uint8_t *ptr)
{
    uint8_t channelIndex;
    DataRange_t drRange;
    uint32_t frequency = 0;
    uint16_t dutyCycle;
    channelIndex = *(ptr++);
    ValChId_t val_chid_req;
    UpdateChId_t  update_chid;
    ValUpdateDrange_t val_update_drange_req;
    ValUpdateFreqTx_t val_update_freqTx_req;
    memcpy(&frequency, ptr, sizeof(frequency));
    frequency = frequency & 0x00FFFFFF;
    frequency = frequency * 100;
    ptr = ptr + 3;  // 3 bytes for frequency

    drRange.value = *(ptr++);
 	if( loRa.ismBand != ISM_AU915 && loRa.ismBand != ISM_NA915)
	{
	    val_chid_req.channelIndex = channelIndex;
	    val_chid_req.allowedForDefaultChannels = WITHOUT_DEFAULT_CHANNELS;
	    if (LORAREG_ValidateAttr (CHANNEL_ID,&val_chid_req) == LORAWAN_SUCCESS)
	    {
	        val_update_drange_req.channelIndex = channelIndex;
	        val_update_drange_req.dataRangeNew = drRange.value;

	        val_update_freqTx_req.channelIndex = channelIndex;
	        val_update_freqTx_req.frequencyNew = frequency;

	        if ( (LORAREG_ValidateAttr (TX_FREQUENCY,&val_update_freqTx_req) == LORAWAN_SUCCESS) || (frequency == 0) )
	        {
	            loRa.macCommands[loRa.crtMacCmdIndex].channelFrequencyAck = 1;
	        }

	        if (LORAREG_ValidateAttr (DATA_RANGE,&val_update_drange_req) == LORAWAN_SUCCESS)
	        {
	            loRa.macCommands[loRa.crtMacCmdIndex].dataRateRangeAck = 1;
	        }
	    }

	    if ( (loRa.macCommands[loRa.crtMacCmdIndex].channelFrequencyAck == 1) && (loRa.macCommands[loRa.crtMacCmdIndex].dataRateRangeAck == 1) )
	    {
	        //if (loRa.lastUsedChannelIndex < 16) Commented to understand if +16 is required
	        {
	            if (frequency != 0)
	            {
	                LORAREG_SetAttr(DATA_RANGE,&val_update_drange_req);
	                LORAREG_SetAttr(FREQUENCY,&val_update_freqTx_req);

	                // after updating the status of a channel we need to check if the minimum dataRange has changed or not.

	                MinMaxDr_t minmaxDr;
	                LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr)); 

	                loRa.minDataRate = minmaxDr.minDr;
	                loRa.maxDataRate = minmaxDr.maxDr;

	                // If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
	                UpdateCurrentDataRateAfterDataRangeChanges ();

	                if(loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
	                {

	                    LORAREG_GetAttr(DUTY_CYCLE,&channelIndex,&(dutyCycle));

	                    UpdateDutyCycle_t update_dCycle;
	                    update_dCycle.channelIndex = channelIndex;
	                    update_dCycle.dutyCycleNew = dutyCycle;
					
	                    LORAREG_SetAttr (DUTY_CYCLE,&update_dCycle);
	                }
	                update_chid.channelIndex = channelIndex;
	                update_chid.statusNew = ENABLED;
	                LORAREG_SetAttr (CHANNEL_ID_STATUS,&update_chid);

	            }
	            else
	            {
	                update_chid.channelIndex = channelIndex;
	                update_chid.statusNew = DISABLED;
	                LORAREG_SetAttr (CHANNEL_ID_STATUS,&update_chid); // according to the spec, a frequency value of 0 disables the channel
					LORAREG_SetAttr(FREQUENCY,&val_update_freqTx_req);// Update the channel frequency to 0 in respective channel parameters
					
	            }

	            // after updating the status of a channel we need to check if the minimum dataRange has changed or not.

	            MinMaxDr_t minmaxDr;
	            LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr));

	            loRa.minDataRate = minmaxDr.minDr;
	            loRa.maxDataRate = minmaxDr.maxDr;

	            // If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
	            UpdateCurrentDataRateAfterDataRangeChanges ();

	        }

	        loRa.macStatus.channelsModified = 1; // a new channel was added, so the flag is set to inform the user
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
		}
    }
    return ptr;
}

uint8_t* ExecuteDlChannel (uint8_t *ptr)
{
    uint8_t channelIndex;
    uint32_t frequency = 0;
	ValChId_t val_chid_req;
	ValUpdateFreqTx_t val_update_freqDl_req;

	channelIndex = *(ptr++);

	memcpy(&frequency, ptr, sizeof(frequency));
	frequency = MAC_CMD_FREQ_VALUE(frequency);
	frequency = MAC_CMD_FREQ_IN_HZ(frequency);
	ptr = ptr + FREQ_VALUE_SIZE_IN_BYTES;

	val_chid_req.channelIndex = channelIndex;
	val_chid_req.allowedForDefaultChannels = ALL_CHANNELS;
	if (LORAREG_ValidateAttr (CHANNEL_ID,&val_chid_req) == LORAWAN_SUCCESS)
	{
		uint8_t ch_status = DISABLED;
		StackRetStatus_t result = LORAWAN_SUCCESS;
		
		val_update_freqDl_req.channelIndex = channelIndex;
		val_update_freqDl_req.frequencyNew = frequency;
		result = LORAREG_GetAttr(CHANNEL_ID_STATUS,&channelIndex,&ch_status);
		if((LORAWAN_INVALID_PARAMETER != result)   && (ENABLED == ch_status) )
		{
			loRa.macCommands[loRa.crtMacCmdIndex].uplinkFreqExistsAck = 1;

		}
		if ( LORAWAN_SUCCESS == LORAREG_SetAttr(DL_FREQUENCY,&val_update_freqDl_req))
		{
			loRa.macCommands[loRa.crtMacCmdIndex].channelFrequencyAck = 1;
			
		}		

	}
    return ptr;
}

uint8_t* ExecuteTxParamSetup (uint8_t *ptr)
{
	TxParams_t txParams = {0};
	uint8_t eirpIndex;
	uint8_t EIRP_DwellTime = 0;
	
	EIRP_DwellTime = *(ptr);
	eirpIndex = EIRP_DwellTime & LAST_NIBBLE;
	txParams.maxEIRP = maxEIRPTable[eirpIndex];
	/* Assign the bit[5] as uplinkDwellTime */
	txParams.uplinkDwellTime =  ((EIRP_DwellTime & (0x01 << SHIFT4)) >> SHIFT4);
	/* Assign the bit[6] as downlinkDwellTime */
	txParams.downlinkDwellTime =  ((EIRP_DwellTime & (0x01 << SHIFT5)) >> SHIFT5);
	ptr++;
	
	if(LORAWAN_SUCCESS != LORAREG_SetAttr(TX_PARAMS,&txParams))
	{
		/* No status needs to be sent for this MAC command. Still adding this if check, to enable future LOGGING/ASSERT to be placed here */
		loRa.macCommands[loRa.crtMacCmdIndex].receivedCid = INVALID_VALUE;
	}
	return ptr;
}
uint8_t* ExecuteLinkAdr (uint8_t *ptr)
{
    uint8_t txPower, dataRate;
    uint16_t channelMask;

    txPower = *(ptr) & LAST_NIBBLE;
    dataRate = ( *(ptr) & FIRST_NIBBLE ) >> SHIFT4;
    ptr++;
    memcpy((uint8_t *)&channelMask, ptr, sizeof(uint16_t));
    ptr = ptr + sizeof (channelMask);
    Redundancy_t *redundancy;
    redundancy = (Redundancy_t*)(ptr++);
    DataRange_t bandDr;
    BandDrReq_t bandDrReq;
	ValChMaskCntl_t chMaskChCntl;

    bandDrReq.chnlMask = channelMask;
    bandDrReq.chnlMaskCntl = redundancy->chMaskCntl;
    uint8_t chMaskCntl = redundancy->chMaskCntl;
	
	loRa.linkAdrResp.channelMaskAck = 0;
	loRa.linkAdrResp.dataRateAck = 0;
	loRa.linkAdrResp.powerAck = 0;

	chMaskChCntl.chnlMask = channelMask;
	chMaskChCntl.chnlMaskCntl = chMaskCntl;
	
	/*Validate Channel Mask and Control Values*/
	if(LORAREG_ValidateAttr(CHMASK_CHCNTL,&chMaskChCntl) == LORAWAN_SUCCESS)
	{
		UpdateNewCh_t update_newCh;
        update_newCh.channelMask = channelMask;
        update_newCh.channelMaskCntl = chMaskCntl;

        if(LORAREG_SetAttr(NEW_CHANNELS,&update_newCh)== LORAWAN_SUCCESS)
        {
	        loRa.linkAdrResp.channelMaskAck = 1;
        }
        

		/*Get the Min and Max DR supported by the new list of channels as per the Mask/Cntl*/
		LORAREG_GetAttr(DATA_RANGE_CH_BAND,&bandDrReq,&bandDr);

		/*Validate the Data rate and check if the New Data rate is within the range supported by the mask/cntl*/
		if ((LORAREG_ValidateAttr (TX_DATARATE,&dataRate) == LORAWAN_SUCCESS) && ((dataRate == 0x0F) ||
			((dataRate >= bandDr.min) && (dataRate <= bandDr.max))))
		{
			loRa.linkAdrResp.dataRateAck = 1;
		}

	}
    if (LORAREG_ValidateAttr (TX_PWR,&txPower) == LORAWAN_SUCCESS)
    {
		loRa.linkAdrResp.powerAck = 1;
    }   
  
    /*
    * The value (decimal 15) of either DataRate or TXPower means that
    * the end-device SHALL ignore that field and keep the current parameter values.
    */
	if ( (loRa.linkAdrResp.powerAck == 1) && (loRa.linkAdrResp.dataRateAck == 1) && (loRa.linkAdrResp.channelMaskAck == 1) )
	{
		loRa.linkAdrResp.channelMask = channelMask;       
		loRa.linkAdrResp.dataRate = (0xf == dataRate) ? loRa.currentDataRate : dataRate;
		loRa.linkAdrResp.redundancy.chMaskCntl = redundancy->chMaskCntl;
		loRa.linkAdrResp.redundancy.nbRep = redundancy->nbRep;
		loRa.linkAdrResp.txPower = (0xf == txPower) ? loRa.txPower : txPower;
	}
	loRa.linkAdrResp.count++;
    return ptr;
}

/**
 * @Summary
    Decode Device Time Answer MAC Command.
 * @Description
    This function is used for decoding Device Time Answer MAC Command
 * @Preconditions
    None
 * @Param
    ptr - MAC Payload buffer pointer with index pointing to MAC Command
 * @Returns
    MAC payload pointer after decoding the MAC Command
*/
 uint8_t* ExecuteDevTimeAns (uint8_t *ptr)
 {
    /*
    * 1st 4 bytes contains seconds part of GPS Epoch. Copy that to Epoch Secs variable
    * Last bytes contains fractional second in (1/2)^8 seconds
    */
    memcpy(&(loRa.devTime.gpsEpochTime.secondsSinceEpoch), ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    loRa.devTime.gpsEpochTime.fractionalSecond = *ptr;
    ptr ++;
    return ptr;
 }

static void UpdateLinkAdrCommands(uint16_t channelMask,uint8_t chMaskCntl,uint8_t nbRep,uint8_t txPower,uint8_t dataRate)
{
	
    // after updating the status of a channel we need to check if the minimum dataRange has changed or not.
    MinMaxDr_t minmaxDr;
    LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr));

    loRa.minDataRate = minmaxDr.minDr;
    loRa.maxDataRate = minmaxDr.maxDr;

    // If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
    UpdateCurrentDataRateAfterDataRangeChanges ();

    UpdateTxPower (txPower);

    loRa.macStatus.txPowerModified = ENABLED; // the current tx power was modified, so the user is informed about the change via this flag
	
    UpdateCurrentDataRate (dataRate);

    if (nbRep == 0)
    {		
        loRa.maxRepetitionsUnconfirmedUplink = 0;
    }
    else
    {
        loRa.maxRepetitionsUnconfirmedUplink = nbRep - 1;
		loRa.maxRepetitionsConfirmedUplink = nbRep-1;
    }
	PDS_STORE(PDS_MAC_MAX_REP_UNCNF_UPLINK);
    loRa.macStatus.nbRepModified = 1;
	PDS_STORE(PDS_MAC_LORAWAN_STATUS);	
	
}

void AssemblePacket (bool confirmed, uint8_t port, uint8_t *buffer, uint16_t bufferLength)
{
    Mhdr_t mhdr;
    uint16_t bufferIndex = 16;
    FCtrl_t fCtrl;
    uint16_t macCmdIdx = 0;
	SalStatus_t sal_status = SAL_SUCCESS;
	
    memset (&mhdr, 0, sizeof (mhdr) );    //clear the header structure Mac header
    memset (&macBuffer[0], 0, sizeof (macBuffer) ); //clear the mac buffer
    memset (aesBuffer, 0, sizeof (aesBuffer) );  //clear the transmission buffer

    if (confirmed == 1)
    {
        mhdr.bits.mType = FRAME_TYPE_DATA_CONFIRMED_UP;
        loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage = 1;
    }
    else
    {
        mhdr.bits.mType = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    }
    mhdr.bits.major = 0;
    mhdr.bits.rfu = 0;
    macBuffer[bufferIndex++] = mhdr.value;

    memcpy (&macBuffer[bufferIndex], loRa.activationParameters.deviceAddress.buffer, sizeof (loRa.activationParameters.deviceAddress.buffer) );
    bufferIndex = bufferIndex + sizeof(loRa.activationParameters.deviceAddress.buffer);

    fCtrl.value = 0; //clear the fCtrl value
    
    if (ENABLED == loRa.macStatus.adr)
    {
        if (loRa.currentDataRate >= loRa.minDataRate)
        {
            fCtrl.adr = ENABLED;
            lorawanADR(&fCtrl);
        }
    }
    else
    {
        fCtrl.adr = DISABLED;
        loRa.lorawanMacStatus.adrAckRequest = DISABLED;        
    }

    if (loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage == ENABLED)
    {
        fCtrl.ack = ENABLED;
        loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = DISABLED;
    }

    fCtrl.fPending = RESERVED_FOR_FUTURE_USE;  //fPending bit is ignored for uplink packets

    if ( (loRa.crtMacCmdIndex == 0) || (bufferLength == 0) ) // there is no MAC command in the queue or there are MAC commands to respond, but the packet does not include application payload (in this case the response to MAC commands will be included inside FRM payload)
    {
        fCtrl.fOptsLen = 0;         // fOpts field is absent
    }
    else
    {	
        uint8_t foptsFlag = false;
        fCtrl.fOptsLen = CountfOptsLength(&foptsFlag);
    }
    macBuffer[bufferIndex++] = fCtrl.value;

    memcpy (&macBuffer[bufferIndex], &loRa.fCntUp.members.valueLow, sizeof (loRa.fCntUp.members.valueLow) );

    bufferIndex = bufferIndex + sizeof(loRa.fCntUp.members.valueLow);

    if ( (loRa.crtMacCmdIndex != 0) && (bufferLength != 0) ) // the response to MAC commands will be included inside FOpts
    {
        IncludeMacCommandsResponse (macBuffer, &bufferIndex, 1);
    }
	if((bufferLength != 0) || ((loRa.crtMacCmdIndex != 0)) )
	{
		macBuffer[bufferIndex++] = port;     // the port field is present if the frame payload field is not empty
	}

    if (bufferLength != 0)
    {
        memcpy (&macBuffer[bufferIndex], buffer, bufferLength);
        sal_status = EncryptFRMPayload (buffer, bufferLength, 0, loRa.fCntUp.value, loRa.activationParameters.applicationSessionKeyRam, SAL_APPS_KEY,  bufferIndex, macBuffer, loRa.activationParameters.deviceAddress.value);
        if (SAL_SUCCESS != sal_status)
        {
	        /* Transaction complete Event */
	        UpdateTransactionCompleteCbParams(LORAWAN_TXPKT_ENCRYPTION_FAILED);
        }
		bufferIndex = bufferIndex + bufferLength;
    }
    else if ( (loRa.crtMacCmdIndex > 0) ) // if answer is needed to MAC commands, include the answer here because there is no app payload
    {
        // Use networkSessionKey for port 0 data
        //Use radioBuffer as a temporary buffer. The encrypted result is found in macBuffer
        IncludeMacCommandsResponse (radioBuffer, &macCmdIdx, 0 );
        sal_status = EncryptFRMPayload (radioBuffer, macCmdIdx, 0, loRa.fCntUp.value, loRa.activationParameters.networkSessionKeyRam, SAL_NWKS_KEY, bufferIndex, macBuffer, loRa.activationParameters.deviceAddress.value);
        if (SAL_SUCCESS != sal_status)
        {
	        /* Transaction complete Event */
	        UpdateTransactionCompleteCbParams(LORAWAN_TXPKT_ENCRYPTION_FAILED);
        }
		bufferIndex = bufferIndex + macCmdIdx;
    }

    AssembleEncryptionBlock (0, loRa.fCntUp.value, bufferIndex - 16, 0x49, loRa.activationParameters.deviceAddress.value);
    memcpy (&macBuffer[0], aesBuffer, sizeof (aesBuffer));

    SAL_AESCmac (loRa.activationParameters.networkSessionKeyRam, SAL_NWKS_KEY, aesBuffer, macBuffer, bufferIndex  );

    memcpy (&macBuffer[bufferIndex], aesBuffer, 4);
    bufferIndex = bufferIndex + 4; // 4 is the size of MIC

    loRa.lastPacketLength = bufferIndex - 16;
}

uint8_t PrepareJoinRequestFrame (void)
{
    uint8_t bufferIndex = 0, iCtr;
    Mhdr_t mhdr;
    uint32_t mic;

    memset (macBuffer, 0, sizeof(macBuffer) );  // clear the mac buffer

    mhdr.bits.mType = FRAME_TYPE_JOIN_REQ;  //prepare the mac header to include mtype as frame type join request
    mhdr.bits.major = MAJOR_VERSION3;
    mhdr.bits.rfu = RESERVED_FOR_FUTURE_USE;

    macBuffer[bufferIndex++] = mhdr.value;  // add the mac header to the buffer
    if (true == loRa.cryptoDeviceEnabled)
	{
		SAL_Read(SAL_JOIN_EUI,(uint8_t *) &loRa.activationParameters.joinEui.buffer);
	    SAL_Read(SAL_DEV_EUI,(uint8_t *) &loRa.activationParameters.deviceEui.buffer);
	}	
	
    for(iCtr = 0; iCtr < 8; iCtr ++)
    {
        macBuffer[bufferIndex + iCtr] = loRa.activationParameters.joinEui.buffer[7 - iCtr];
    }
    bufferIndex = bufferIndex + sizeof(loRa.activationParameters.joinEui);

    for (iCtr = 0; iCtr < 8; iCtr ++)
    {
        macBuffer[bufferIndex + iCtr] = loRa.activationParameters.deviceEui.buffer[7 - iCtr];
    }
    bufferIndex = bufferIndex + sizeof( loRa.activationParameters.deviceEui );

    if (loRa.joinNonceType == JOIN_NONCE_RANDOM) /* Deprecate if not needed */
        loRa.devNonce = Random(UINT16_MAX);
    else /* This shall be the default choice */
        loRa.devNonce++;
    PDS_STORE(PDS_MAC_DEV_NONCE);
    memcpy (&macBuffer[bufferIndex], &loRa.devNonce, sizeof (loRa.devNonce) );
    bufferIndex = bufferIndex + sizeof( loRa.devNonce );

    mic = ComputeMic (loRa.activationParameters.applicationKey, macBuffer, bufferIndex);

    memcpy ( &macBuffer[bufferIndex], &mic, sizeof (mic));
    bufferIndex = bufferIndex + sizeof(mic);

    return bufferIndex;
}


static void IncludeMacCommandsResponse (uint8_t* macCommandsBuffer, uint16_t* pBufferIndex, uint8_t bIncludeInFopts )
{
    uint8_t i = 0;
    uint16_t bufferIndex = *pBufferIndex;
	
	uint8_t foptsFlag = false;
    /* validate data length using MaxPayloadSize */
	uint8_t macCmdReplyLen = CountfOptsLength(&foptsFlag);
	uint16_t responseLength;
	//if (foptsFlag	== false || macCmdReplyLen > LorawanGetMaxPayloadSize())
    if (macCmdReplyLen > MAX_FOPTS_LEN || LorawanGetMaxPayloadSize(loRa.currentDataRate) < MAX_FOPTS_LEN)	{
	    responseLength = LorawanGetMaxPayloadSize(loRa.currentDataRate);
    }
	else{
		responseLength = MAX_FOPTS_LEN;
	}
	
	
    for(i = 0; i < loRa.crtMacCmdIndex ; i++)
    {
        if(loRa.macCommands[i].receivedCid != INVALID_VALUE)
        {
            if((bufferIndex - (*pBufferIndex) + macEndDevCmdReplyLen[loRa.macCommands[i].receivedCid - 2]) > responseLength)
            {
                break;
            }
        }

        switch (loRa.macCommands[i].receivedCid)
        {
        case LINK_ADR_CID:
        {
            macCommandsBuffer[bufferIndex++] = LINK_ADR_CID;
            macCommandsBuffer[bufferIndex] = 0x00;
            if (loRa.linkAdrResp.channelMaskAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= CHANNEL_MASK_ACK;
            }

            if (loRa.linkAdrResp.dataRateAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= DATA_RATE_ACK;
            }

            if (loRa.linkAdrResp.powerAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= POWER_ACK;
            }
			
            bufferIndex ++;
        }
        break;

        case RX2_SETUP_CID:
        {
            macCommandsBuffer[bufferIndex++] = RX2_SETUP_CID;
            macCommandsBuffer[bufferIndex] = 0x00;
            if (loRa.macCommands[i].channelAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= CHANNEL_MASK_ACK;
            }

            if (loRa.macCommands[i].dataRateReceiveWindowAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= DATA_RATE_ACK;
            }

            if (loRa.macCommands[i].rx1DROffestAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= RX1_DR_OFFSET_ACK;
            }
			
            bufferIndex ++;
        }
        break;

        case DEV_STATUS_CID:
        {
            int8_t packetSNR;
            RADIO_GetAttr(PACKET_SNR,(void *)&packetSNR);
            macCommandsBuffer[bufferIndex++] = DEV_STATUS_CID;
            macCommandsBuffer[bufferIndex++] = loRa.batteryLevel;
            if ((packetSNR < -32) || (packetSNR > 31))
            {
                macCommandsBuffer[bufferIndex++] = 0x20;  //if the value returned by the radio is out of range, send the minimum (-32)
            }
            else
            {
                macCommandsBuffer[bufferIndex++] = ((uint8_t)packetSNR & 0x3F);  //bits 7 and 6 are RFU, bits 5-0 are  SNR  information;
            }
        }
        break;

        case NEW_CHANNEL_CID:
        {
            macCommandsBuffer[bufferIndex++] = NEW_CHANNEL_CID;
            macCommandsBuffer[bufferIndex] = 0x00;
            if (loRa.macCommands[i].channelFrequencyAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= CHANNEL_MASK_ACK;
            }

            if (loRa.macCommands[i].dataRateRangeAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= DATA_RATE_ACK;
            }
            bufferIndex ++;
        }
        break;
		case TX_PARAM_SETUP_CID:
		{
			macCommandsBuffer[bufferIndex++] = loRa.macCommands[i].receivedCid;
		}
		break;
		case DL_CHANNEL_CID:
		{
			macCommandsBuffer[bufferIndex++] = DL_CHANNEL_CID;
           	macCommandsBuffer[bufferIndex] = 0x00;
            if (loRa.macCommands[i].channelFrequencyAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= CHANNEL_MASK_ACK;
            }

            if (loRa.macCommands[i].uplinkFreqExistsAck == 1)
            {
                macCommandsBuffer[bufferIndex] |= UPLINK_FREQ_EXISTS_ACK;
            }
            bufferIndex ++;					
		}
        break;
        case LINK_CHECK_CID:
        {
            loRa.linkCheckMargin = 255; // reserved
            loRa.linkCheckGwCnt = 0;
            macCommandsBuffer[bufferIndex++] = loRa.macCommands[i].receivedCid;
        }
        break;

        case RX_TIMING_SETUP_CID: //Fallthrough
        case DUTY_CYCLE_CID:      //Fallthrough
        {
            macCommandsBuffer[bufferIndex++] = loRa.macCommands[i].receivedCid;
        }
        break;

		case DEV_TIME_CID:
		{
            SwTimestamp_t stamp = UINT64_MAX;
    		SwTimerWriteTimestamp(loRa.devTime.sysEpochTimeIndex, &stamp);
    		loRa.devTime.gpsEpochTime.secondsSinceEpoch = UINT32_MAX;
    		loRa.devTime.gpsEpochTime.fractionalSecond = UINT8_MAX;
    		loRa.devTime.isDevTimeReqSent = true;
    		macCommandsBuffer[bufferIndex++] = loRa.macCommands[i].receivedCid;
		}
		break;

        default:
            //CID = 0xFF
            break;

        }
    }

	memset(&loRa.linkAdrResp,0x00,sizeof(LinkAdrResp_t));
    *pBufferIndex = bufferIndex;
}


static void SetReceptionNotOkState (void)
{
	if (CLASS_C == loRa.edClass)
	{	// just drop the frame in class C, wait for timeout to notify application
		loRa.isTransactionDone = true;
        loRa.enableRxcWindow = true;
		loRa.macStatus.macState = RX2_OPEN;
		//Continue to be in Receive mode with RX2
		LorawanConfigureRadioForRX2(false);
	}
	else
    /* just drop the frame in class C, wait for timeout to notify application */
    {
        loRa.macStatus.macState = RX2_OPEN;
        //Continue to be in Receive mode with RX2
        LorawanConfigureRadioForRX2(false);
    }
}

void ConfigureRadioRx(uint8_t dataRate, uint32_t freq)
{
    uint8_t crcEnabled,iqInverted;
    crcEnabled = DISABLED;
    iqInverted = ENABLED;

    radioConfig_t radioConfig;
    radioConfig.freq_hop_period = DISABLED;

    LORAREG_GetAttr(MODULATION_ATTR,&(dataRate),&(radioConfig.modulation));

    LORAREG_GetAttr(BANDWIDTH_ATTR,&(dataRate),&(radioConfig.bandwidth));

    LORAREG_GetAttr(SPREADING_FACTOR_ATTR,&(dataRate),&(radioConfig.sf));

    /*Currently this is disabled for both EU and NA,might be required later in NA*/
    radioConfig.freq_hop_period = DISABLED ;

    radioConfig.frequency = freq;

    ConfigureRadio(&radioConfig);

    if (MODULATION_FSK == radioConfig.modulation)
    {
        crcEnabled = ENABLED;
    }
    RADIO_SetAttr(CRC_ON,(void *)&crcEnabled);
    RADIO_SetAttr(IQINVERTED,(void *)&iqInverted);
}

void ConfigureRadioTx(radioConfig_t radioConfig)
{
    uint8_t crcEnabled,iqInverted;
    crcEnabled = ENABLED;
    iqInverted = DISABLED;

    ConfigureRadio(&radioConfig);

	if(radioConfig.ecrConfig.override == true)
	{
		loRa.ecrConfig.override = true;
		RADIO_GetAttr(ERROR_CODING_RATE,&(loRa.ecrConfig.ecr));
		RADIO_SetAttr(ERROR_CODING_RATE,(void *)&(radioConfig.ecrConfig.ecr));
	}

	RADIO_SetAttr(OUTPUT_POWER,(void *)&radioConfig.txPower);
	RADIO_SetAttr(CRC_ON,(void *)&crcEnabled);
	RADIO_SetAttr(IQINVERTED,(void *)&iqInverted);
}

static void ConfigureRadio(radioConfig_t* radioConfig)
{

    uint8_t syncword_len;

    RADIO_SetAttr(MODULATION,(void *)&(radioConfig->modulation));
    RADIO_SetAttr(CHANNEL_FREQUENCY,(void *)&(radioConfig->frequency));
    RADIO_SetAttr(FREQUENCY_HOP_PERIOD,(void *)&(radioConfig->freq_hop_period));

    if (radioConfig->modulation == MODULATION_LORA)
    {
        //LoRa modulation
        RADIO_SetAttr(SPREADING_FACTOR,(void *)&(radioConfig->sf));
        RADIO_SetAttr(BANDWIDTH,(void *)(&(radioConfig->bandwidth)));
        RADIO_SetAttr(LORA_SYNC_WORD,(void *)&(loRa.syncWord));
    }
    else
    {
        //FSK modulation
        syncword_len = sizeof(FskSyncWordBuff) / sizeof(FskSyncWordBuff[0]);
        RADIO_SetAttr(FSK_SYNC_WORD_LEN,(void *)&syncword_len);
        RADIO_SetAttr(FSK_SYNC_WORD,(void *)FskSyncWordBuff);
    }

}

static void UpdateReceiveDelays (uint8_t delay)
{
    loRa.protocolParameters.receiveDelay1 = 1000 * delay ;
    if (delay == 0)
    {
        loRa.protocolParameters.receiveDelay1 = 1000;
    }
	PDS_STORE(PDS_MAC_RX_DELAY_1);
    loRa.protocolParameters.receiveDelay2 = loRa.protocolParameters.receiveDelay1 + 1000;
	PDS_STORE(PDS_MAC_RX_DELAY_2);
}

void UpdateJoinInProgress(uint8_t state)
{
    /* set the states and flags accordingly */
    loRa.macStatus.networkJoined = 0; //last join (if any) is not considered any more, a new join is requested
    loRa.lorawanMacStatus.joining = true;
    loRa.macStatus.macState = state;
	PDS_STORE(PDS_MAC_LORAWAN_STATUS);
}

static void UpdateCfList (uint8_t bufferLength, JoinAccept_t *joinAccept)
{
    uint8_t i;
    uint32_t frequency;
    uint8_t minChannelIndex,channelIndex;
    DataRange_t dataRange;
    uint16_t dutyCycleNew;
    ValUpdateFreqTx_t val_update_freqTx;
    ValUpdateDrange_t val_update_drange;
	uint8_t cfList[16];
	uint16_t chMask;
	uint8_t startIndx, endIndx;
	
	/* Restore all the Channels to default state before updating CFList */
	LORAREG_SetAttr(CHLIST_DEFAULTS,NULL);
    if ( (bufferLength == SIZE_JOIN_ACCEPT_WITH_CFLIST) )
    {

		LORAREG_GetAttr(MIN_NEW_CH_INDEX,&(loRa.currentDataRate),&(minChannelIndex));
		if(minChannelIndex == 0xFF)
		{
			startIndx = 0; endIndx = 15;
			memcpy(cfList, joinAccept->members.cfList, sizeof(cfList));
			//Current Region doesn't support Updating New channel list
			if (cfList[15] == CFLIST_TYPE_1)// Received Chmask for 72channels
			{
				loRa.joinAcceptChMaskReceived = true;
				for (i = 0; i < NUMBER_CFLIST_CHMASK; i++)
				{
					memcpy(&chMask, &cfList[2*i], sizeof(chMask));
					
					for(uint8_t j = startIndx; j <= endIndx; j++)
					{
						if((chMask & (0x0001)) == 0x0001)
						{
							LorawanSetChannelIdStatus(j, ENABLED);
						}
						else
						{
							LorawanSetChannelIdStatus(j, DISABLED);
						}
						chMask = chMask >> SHIFT1;
					}
					
					startIndx = startIndx + 16;
					endIndx = endIndx + 16;
				}
				
			}
			//Current Region doesn't support Updating New channel list
			return;
		}
		
		else
		{
			memcpy(cfList, joinAccept->members.cfList, sizeof(cfList));
			
			if (cfList[15] == CFLIST_TYPE_0)
			{
				dataRange.max = DR5;
				dataRange.min = DR0;

				for (i = 0; i < NUMBER_CFLIST_FREQUENCIES; i++ )
				{
					frequency = 0;
					memcpy (&frequency, joinAccept->members.cfList + 3*i, 3);
					frequency *= 100;
					channelIndex = minChannelIndex+i;
					if (frequency != 0)
					{
						val_update_freqTx.channelIndex = channelIndex;
						val_update_freqTx.frequencyNew = frequency;
						val_update_drange.channelIndex = channelIndex;
						val_update_drange.dataRangeNew = dataRange.value;

						if (LORAREG_ValidateAttr (TX_FREQUENCY,&val_update_freqTx) == LORAWAN_SUCCESS)
						{
							LORAREG_SetAttr(FREQUENCY,&val_update_freqTx);

							LORAREG_SetAttr(DATA_RANGE,&val_update_drange);

							// after updating the status of a channel we need to check if the minimum dataRange has changed or not.
							//UpdateMinMaxChDataRate ();

							MinMaxDr_t minmaxDr;
							LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr));

							loRa.minDataRate = minmaxDr.minDr;
							loRa.maxDataRate = minmaxDr.maxDr;

							// If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
							UpdateCurrentDataRateAfterDataRangeChanges();

							LORAREG_GetAttr(DUTY_CYCLE,&channelIndex,&(dutyCycleNew));

							if(loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
							{
								UpdateDutyCycle_t update_dCycle;
								update_dCycle.channelIndex = channelIndex;
								update_dCycle.dutyCycleNew = dutyCycleNew;

								LORAREG_SetAttr (DUTY_CYCLE,&update_dCycle);
							}

							LorawanSetChannelIdStatus(channelIndex, ENABLED);

							loRa.macStatus.channelsModified = ENABLED; // a new channel was added, so the flag is set to inform the user
							PDS_STORE(PDS_MAC_LORAWAN_STATUS);
						}
					}
					else
					{
						LorawanSetChannelIdStatus(channelIndex, DISABLED);
					}
				}

				loRa.macStatus.channelsModified = ENABLED;
				PDS_STORE(PDS_MAC_LORAWAN_STATUS);
			}
			
		}
	
	}
	
}





static void PrepareSessionKeys (uint8_t* sessionKey, uint8_t* joinNonce, uint8_t* networkId)
{
    uint8_t index = 0;

    memset (&sessionKey[index], 0, sizeof(aesBuffer));  //appends 0-es to the buffer so that pad16 is done
    index ++; // 1 byte for 0x01 or 0x02 depending on the key type

    memcpy(&sessionKey[index], joinNonce, JA_JOIN_NONCE_SIZE);
    index = index + JA_JOIN_NONCE_SIZE;

    memcpy(&sessionKey[index], networkId, JA_NET_ID_SIZE);
    index = index + JA_NET_ID_SIZE;

    memcpy(&sessionKey[index], &loRa.devNonce, sizeof(loRa.devNonce) );

}

static void ComputeSessionKeys (JoinAccept_t *joinAcceptBuffer)
{
	SalStatus_t sal_status = SAL_SUCCESS;
    PrepareSessionKeys(loRa.activationParameters.applicationSessionKeyRom, joinAcceptBuffer->members.joinNonce, joinAcceptBuffer->members.networkId);
    loRa.activationParameters.applicationSessionKeyRom[0] = 0x02; // used for Application Session Key
    sal_status = SAL_DeriveSessionKey(loRa.activationParameters.applicationSessionKeyRom, SAL_APP_KEY, loRa.activationParameters.applicationKey, SAL_APPS_KEY);	
	if (SAL_SUCCESS != sal_status)
	{
		SetJoinFailState(sal_status);
		SetReceptionNotOkState();
	}
	PDS_STORE(PDS_MAC_APP_SKEY);
	
    PrepareSessionKeys(loRa.activationParameters.networkSessionKeyRom, joinAcceptBuffer->members.joinNonce, joinAcceptBuffer->members.networkId);
    loRa.activationParameters.networkSessionKeyRom[0] = 0x01; // used for Network Session Key
    sal_status = SAL_DeriveSessionKey(loRa.activationParameters.networkSessionKeyRom, SAL_APP_KEY, loRa.activationParameters.applicationKey, SAL_NWKS_KEY);
	if (SAL_SUCCESS != sal_status)
	{
		SetJoinFailState(sal_status);
		SetReceptionNotOkState();
	}
	PDS_STORE(PDS_MAC_NWK_SKEY);
}

//Based on the last packet received, this function checks the flags and updates the state accordingly
static void CheckFlags (Hdr_t* hdr)
{
    if (hdr->members.fCtrl.adr == ENABLED)
    {
        loRa.macStatus.adr = ENABLED;
		PDS_STORE(PDS_MAC_LORAWAN_STATUS);
    }

    if (hdr->members.fCtrl.fPending == ENABLED)
    {
        loRa.lorawanMacStatus.fPending = ENABLED;
    }

    if (hdr->members.fCtrl.adrAckReq == ENABLED)
    {
        loRa.lorawanMacStatus.adrAckRequest = ENABLED;
    }

    if (hdr->members.mhdr.bits.mType == FRAME_TYPE_DATA_CONFIRMED_DOWN)
    {
		if(loRa.lorawanMacStatus.retransmittedownlink == true)
		{
			loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = DISABLED;
			loRa.lorawanMacStatus.retransmittedownlink = false;
		}
		else
		{
			loRa.lorawanMacStatus.ackRequiredFromNextUplinkMessage = ENABLED;  //next uplink packet should have the ACK bit set
		}
    }
}

uint8_t CountfOptsLength (uint8_t* fOptsFlag)
{
    uint8_t i, macCommandLength=0;
	//*fOptsFlag = true;

    for (i = 0; i < loRa.crtMacCmdIndex; i++)
    {
        if(loRa.macCommands[i].receivedCid != INVALID_VALUE)
        {
           /* if((macCommandLength + macEndDevCmdReplyLen[loRa.macCommands[i].receivedCid - 2]) <= MAX_FOPTS_LEN)
            {
                macCommandLength += macEndDevCmdReplyLen[loRa.macCommands[i].receivedCid - 2];
            }
            else
            {
                break;
            }*/
		    macCommandLength += macEndDevCmdReplyLen[loRa.macCommands[i].receivedCid - 2];
        }
    }
	if(MAX_FOPTS_LEN > macCommandLength){
		*fOptsFlag = true;
	}
    return macCommandLength;
}

static void AssembleEncryptionBlock (uint8_t dir, uint32_t frameCounter, uint8_t blockId, uint8_t firstByte, uint32_t devAddr)
{
    uint8_t bufferIndex = 0;

    memset (aesBuffer, 0, AES_BLOCKSIZE); //clear the aesBuffer

    aesBuffer[bufferIndex] = firstByte;

    bufferIndex = bufferIndex + 5;  // 4 bytes of 0x00 (done with memset at the beginning of the function)

    aesBuffer[bufferIndex++] = dir;

    memcpy (&aesBuffer[bufferIndex], &devAddr, sizeof (loRa.activationParameters.deviceAddress));
    bufferIndex = bufferIndex + sizeof (loRa.activationParameters.deviceAddress);

    memcpy (&aesBuffer[bufferIndex], &frameCounter, sizeof (frameCounter));
    bufferIndex = bufferIndex + sizeof (frameCounter) ;

    bufferIndex ++;   // 1 byte of 0x00 (done with memset at the beginning of the function)

    aesBuffer[bufferIndex] = blockId;
}

static uint32_t ExtractMic (uint8_t *buffer, uint8_t bufferLength)
{
    uint32_t mic = 0;
    memcpy (&mic, &buffer[bufferLength - 4], sizeof (mic));
    return mic;
}

static uint32_t ComputeMic ( uint8_t *key, uint8_t* buffer, uint8_t bufferLength)  // micType is 0 for join request and 1 for data packet
{
    uint32_t mic = 0;

    SAL_AESCmac(key, SAL_APP_KEY, aesBuffer, buffer, bufferLength); //if micType is 0, bufferLength the same, but for data messages bufferLength increases with 16 because a block is added

    memcpy(&mic, aesBuffer, sizeof( mic ));

    return mic;
}

SalStatus_t EncryptFRMPayload (uint8_t* buffer, uint8_t bufferLength, uint8_t dir, uint32_t frameCounter, uint8_t* key, uint8_t key_type, uint16_t macBufferIndex, uint8_t* bufferToBeEncrypted, uint32_t devAddr)
{
	SalStatus_t sal_status = SAL_SUCCESS;
    uint8_t k = 0, i = 0, j = 0;

    k = bufferLength / AES_BLOCKSIZE;
    for (i = 1; i <= k; i++)
    {
	    AssembleEncryptionBlock (dir, frameCounter, i, 0x01, devAddr);
	    sal_status = SAL_AESEncode(aesBuffer, SAL_APPS_KEY, key);
		if (SAL_SUCCESS != sal_status )
		{
			return sal_status;
		}
	    for (j = 0; j < AES_BLOCKSIZE; j++)
	    {
		    bufferToBeEncrypted[macBufferIndex++] = aesBuffer[j] ^ buffer[AES_BLOCKSIZE*(i-1) + j];
	    }
    }

    if ( (bufferLength % AES_BLOCKSIZE) != 0 )
    {
	    AssembleEncryptionBlock (dir, frameCounter, i, 0x01, devAddr);
	    sal_status = SAL_AESEncode(aesBuffer, SAL_APPS_KEY, key);
		if (SAL_SUCCESS != sal_status )
		{
			return sal_status;
		}
	    for (j = 0; j < (bufferLength % AES_BLOCKSIZE); j++)
	    {
		    bufferToBeEncrypted[macBufferIndex++] = aesBuffer[j] ^ buffer[(AES_BLOCKSIZE*k) + j];
	    }
    }
     
	return sal_status;
}


void UpdateTransactionCompleteCbParams(StackRetStatus_t status)
{	
	 loRa.isTransactionDone = true;
	 
    if ((AppPayload.AppData != NULL) && (loRa.evtmask & LORAWAN_EVT_TRANSACTION_COMPLETE) && (loRa.appHandle != NULL))
    {       
		loRa.cbPar.evt = LORAWAN_EVT_TRANSACTION_COMPLETE;
		loRa.cbPar.param.transCmpl.status = status;
		AppPayload.AppData (loRa.appHandle, &loRa.cbPar);		
    }
	 /*Clear the appHandle only when new requests are not posted from App Callback*/
	 if(loRa.isTransactionDone)
	 {
		loRa.appHandle = NULL;	 
	 }
	
		  
}

void UpdateRxDataAvailableCbParams(uint32_t devAddr, uint8_t *pData,uint8_t dataLength,StackRetStatus_t status)
{

    if ((AppPayload.AppData != NULL) && (loRa.evtmask & LORAWAN_EVT_RX_DATA_AVAILABLE))
    {
		loRa.isTransactionDone = true;	
        loRa.cbPar.evt = LORAWAN_EVT_RX_DATA_AVAILABLE;
        loRa.cbPar.param.rxData.devAddr = devAddr;
        loRa.cbPar.param.rxData.pData = pData;
        loRa.cbPar.param.rxData.dataLength = dataLength;
        loRa.cbPar.param.rxData.status = status;
        AppPayload.AppData (loRa.appHandle, &loRa.cbPar);
		loRa.isTransactionDone = false;
    }
}


StackRetStatus_t LorawanSetReceiveWindow2Parameters (uint32_t frequency, uint8_t dataRate)
{
    StackRetStatus_t result = LORAWAN_SUCCESS;

    if ( (LORAREG_ValidateAttr (RX_FREQUENCY,&frequency) == LORAWAN_SUCCESS) && (LORAREG_ValidateAttr (RX_DATARATE,&dataRate) == LORAWAN_SUCCESS) )
    {
        UpdateReceiveWindow2Parameters (frequency, dataRate);
    }
    else
    {
        result = LORAWAN_INVALID_PARAMETER;
    }
    return result;
}

StackRetStatus_t LorawanSetReceiveWindowCParameters(uint32_t frequency, uint8_t dataRate)
{
	StackRetStatus_t result = LORAWAN_SUCCESS;
    if ( (LORAREG_ValidateAttr (RX_FREQUENCY,&frequency) == LORAWAN_SUCCESS) && (LORAREG_ValidateAttr (RX_DATARATE,&dataRate) == LORAWAN_SUCCESS) )	 
	   {
		    UpdateReceiveWindowCParameters (frequency, dataRate);
	   }
	else
	{
	    result = LORAWAN_INVALID_PARAMETER;
	}
	return result;
}


StackRetStatus_t LorawanSetDataRange (uint8_t channelId, uint8_t dataRangeNew)
{
	StackRetStatus_t result = LORAWAN_SUCCESS;
	ValChId_t val_chid;
	ValUpdateDrange_t val_update_drange;

	val_chid.channelIndex = channelId;
	val_chid.allowedForDefaultChannels = ALL_CHANNELS;

    val_update_drange.channelIndex = channelId;
    val_update_drange.dataRangeNew = dataRangeNew;

    if ( (LORAREG_ValidateAttr (CHANNEL_ID,&val_chid) != LORAWAN_SUCCESS) || (LORAREG_ValidateAttr (DATA_RANGE,&val_update_drange) != LORAWAN_SUCCESS) )
    {
        result = LORAWAN_INVALID_PARAMETER;
    }
    else
    {
        LORAREG_SetAttr(DATA_RANGE,&val_update_drange);

        // after updating the status of a channel we need to check if the minimum dataRange has changed or not.
        //UpdateMinMaxChDataRate ();
        MinMaxDr_t minmaxDr;
        LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr)); //LORAWAN_GetDefaultRx1DR();

        loRa.minDataRate = minmaxDr.minDr;
        loRa.maxDataRate = minmaxDr.maxDr;

        // If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
        UpdateCurrentDataRateAfterDataRangeChanges ();
    }

    return result;
}

StackRetStatus_t LorawanSetChannelIdStatus (uint8_t channelId, bool statusNew)
{
    StackRetStatus_t result = LORAWAN_SUCCESS;
	UpdateChId_t  update_chid;

	update_chid.channelIndex = channelId;
	update_chid.statusNew = statusNew;

    if (LORAREG_SetAttr(CHANNEL_ID_STATUS,&update_chid) == LORAWAN_SUCCESS)
    {

        // after updating the status of a channel we need to check if the minimum dataRange has changed or not.

        MinMaxDr_t minmaxDr;
        LORAREG_GetAttr(MIN_MAX_DR,NULL,&(minmaxDr)); //LORAWAN_GetDefaultRx1DR();

        loRa.minDataRate = minmaxDr.minDr;
        loRa.maxDataRate = minmaxDr.maxDr;

        // If the min/max data rate is changed and the current data rate is outside spec, then update the current data rate accordingly
        UpdateCurrentDataRateAfterDataRangeChanges ();

    }
    else
    {
        result = LORAWAN_INVALID_PARAMETER;
    }

    return result;
}

StackRetStatus_t LorawanSetFrequency (uint8_t channelId, uint32_t frequencyNew)
{
    StackRetStatus_t result = LORAWAN_SUCCESS;
    ValUpdateFreqTx_t  val_update_freqTx;

    val_update_freqTx.channelIndex = channelId;
    val_update_freqTx.frequencyNew = frequencyNew;

    result = LORAREG_SetAttr(FREQUENCY,&val_update_freqTx);

    return result;
}

uint8_t LorawanGetIsmBand(void) //returns the ISM band
{
    return loRa.ismBand;
}

void LorawanGetReceiveWindow2Parameters (ReceiveWindow2Params_t *rx2Params)
{
    rx2Params->dataRate = loRa.receiveWindow2Parameters.dataRate;
    rx2Params->frequency = loRa.receiveWindow2Parameters.frequency;
}

void LorawanGetReceiveWindowCParameters (ReceiveWindowParameters_t *rxcParams)
{
	rxcParams->dataRate = loRa.receiveWindowCParameters.dataRate;
	rxcParams->frequency = loRa.receiveWindowCParameters.frequency;
}


static uint8_t LorawanGetMaxPayloadSize (uint8_t dataRate)
{
    uint8_t result = 0;
	
    LORAREG_GetAttr(MAX_PAYLOAD_SIZE,&(dataRate),&(result));
	
    result -= FHDR_FPORT_SIZE;
    return result;
}

StackRetStatus_t LORAWAN_SetMulticastParam(LorawanAttributes_t attrType, void *attrValue)
{
	StackRetStatus_t result;
	
	result = LORAWAN_SetAttr(attrType , attrValue);
	
	return result;
}

StackRetStatus_t LORAWAN_SetAttr(LorawanAttributes_t attrType, void *attrValue)
{
    StackRetStatus_t result = LORAWAN_INVALID_PARAMETER;
	
	if (true == loRa.isTransactionDone)
	{
		switch(attrType)
		{

		case DEV_EUI:
		{
			if (attrValue != NULL)
			{
				memcpy(loRa.activationParameters.deviceEui.buffer, attrValue, sizeof(loRa.activationParameters.deviceEui));
				PDS_STORE(PDS_MAC_DEV_EUI);
				loRa.macKeys.deviceEui = 1;
				PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
				loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
				PDS_STORE(PDS_MAC_LORAWAN_STATUS);
				result = LORAWAN_SUCCESS;
			}
		}
		break;

		case LORAWAN_LBT_PARAMS:
		{
			if (loRa.featuresSupported & LBT_SUPPORT)
			{
				RadioError_t errorLocal;
				LorawanLBTParams_t lorawanLBTParamsLocal = *(LorawanLBTParams_t *)attrValue;
				RadioLBTParams_t radioLBTParamsLocal;
				if (false == lorawanLBTParamsLocal.lbtTransmitOn)
				{
					radioLBTParamsLocal.lbtTransmitOn = false;
					errorLocal = RADIO_SetAttr(RADIO_LBT_PARAMS, (void *)&radioLBTParamsLocal);
					if (ERR_NONE == errorLocal)
					{
						loRa.lbt.maxRetryChannels = 0;
						loRa.lbt.elapsedChannels = 0;
						PDS_STORE(PDS_MAC_LBT_PARAMS);
						result = LORAWAN_SUCCESS;
					}
					else
					{
						result = LORAWAN_INVALID_PARAMETER;
					}
				}
				else
				{
					radioLBTParamsLocal.lbtNumOfSamples = lorawanLBTParamsLocal.lbtNumOfSamples;
					radioLBTParamsLocal.lbtScanPeriod = lorawanLBTParamsLocal.lbtScanPeriod;
					radioLBTParamsLocal.lbtThreshold = lorawanLBTParamsLocal.lbtThreshold;
					radioLBTParamsLocal.lbtTransmitOn = lorawanLBTParamsLocal.lbtTransmitOn;
					errorLocal = RADIO_SetAttr(RADIO_LBT_PARAMS, (void *)&radioLBTParamsLocal);
					if (ERR_NONE == errorLocal)
					{
						loRa.lbt.maxRetryChannels = lorawanLBTParamsLocal.maxRetryChannels;
						result = LORAWAN_SUCCESS;
					}
					else
					{
						result = LORAWAN_INVALID_PARAMETER;
					}
				}
			}
			else
			{
				result = LORAWAN_INVALID_PARAMETER;
			}
		}
		break;

		case JOIN_EUI:
		{
			if (attrValue != NULL)
			{
				memcpy(loRa.activationParameters.joinEui.buffer, attrValue, sizeof(loRa.activationParameters.joinEui));
				PDS_STORE(PDS_MAC_JOIN_EUI);
				loRa.macKeys.joinEui = 1;
				PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
				loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
				PDS_STORE(PDS_MAC_LORAWAN_STATUS);
				result = LORAWAN_SUCCESS;
			}

		}
		break;

		case DEV_ADDR:
		{
			loRa.activationParameters.deviceAddress.value = *(uint32_t *)attrValue;
			PDS_STORE(PDS_MAC_DEV_ADDR);
			loRa.macKeys.deviceAddress = 1;
			PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
			loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
			result = LORAWAN_SUCCESS;
		}
		break;

		case NWKS_KEY:
		{
			if (attrValue != NULL)
			{
				memcpy(loRa.activationParameters.networkSessionKeyRom, attrValue, 16);
				memcpy(loRa.activationParameters.networkSessionKeyRam, attrValue, 16);
				PDS_STORE(PDS_MAC_NWK_SKEY);
				loRa.macKeys.networkSessionKey = 1;
				PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
				loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
				PDS_STORE(PDS_MAC_LORAWAN_STATUS);
				result = LORAWAN_SUCCESS;
			}
		}
		break;

		case APPS_KEY:
		{
			if (attrValue != NULL)
			{
				memcpy( loRa.activationParameters.applicationSessionKeyRom, attrValue, 16);
				memcpy(loRa.activationParameters.applicationSessionKeyRam, attrValue, 16);
				PDS_STORE(PDS_MAC_APP_SKEY);
				loRa.macKeys.applicationSessionKey = 1;
				PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
				loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
				PDS_STORE(PDS_MAC_LORAWAN_STATUS);
				result = LORAWAN_SUCCESS;
			}

		}
		break;

		case APP_KEY:
		{
			if (attrValue != NULL)
			{
				if (true != loRa.cryptoDeviceEnabled)
				{
					memcpy( loRa.activationParameters.applicationKey, attrValue, 16);
					PDS_STORE(PDS_MAC_APP_KEY);
					loRa.macKeys.applicationKey = 1;
					PDS_STORE(PDS_MAC_LORAWAN_MAC_KEYS);
					loRa.macStatus.networkJoined = DISABLED; // this is a guard against overwriting any of the addresses after one join was already done. If any of the addresses change, rejoin is needed
					PDS_STORE(PDS_MAC_LORAWAN_STATUS);
					result = LORAWAN_SUCCESS;
				}
				else
				{
					//AppKey is already provisioned inside ECC608
					result = LORAWAN_SUCCESS;
				}

			}

		}
		break;

		case ADR:
		{
			loRa.macStatus.adr = *(bool *)attrValue;
			loRa.adrAckCnt = DISABLED;
			loRa.lorawanMacStatus.adrAckRequest = DISABLED; // this flag should only be on when ADR is set and the adr ack counter is bigger than adr ack limit
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
			result = LORAWAN_SUCCESS;
		}
		break;

		case CURRENT_DATARATE:
		{
			uint8_t value;
			value = *(uint8_t *)attrValue;
			if ( (value >= loRa.minDataRate) && (value <= loRa.maxDataRate) && (LORAREG_ValidateAttr(TX_DATARATE,&value) == LORAWAN_SUCCESS) )
			{
				loRa.currentDataRate = value;
				PDS_STORE(PDS_MAC_CURR_DR);
				result = LORAWAN_SUCCESS;
			}

		}
		break;

		case TX_POWER:
		{
			uint8_t txPower = *(uint8_t *)attrValue;
			if (LORAREG_ValidateAttr (TX_PWR,&txPower) == LORAWAN_SUCCESS)
			{
				loRa.txPower = txPower;
				PDS_STORE(PDS_MAC_TX_POWER);
				result = LORAWAN_SUCCESS;
			}

		}
		break;

		case SYNC_WORD:
		{
			loRa.syncWord = *(uint8_t *)attrValue;
			PDS_STORE(PDS_MAC_SYNC_WORD);
			result = LORAWAN_SUCCESS;
		}
		break;

		case UPLINK_COUNTER:
		{
			uint32_t fcntUp = *(uint32_t *)attrValue;
			if(fcntUp < FCNT_MAX)
			{
				loRa.fCntUp.value = fcntUp;
				PDS_STORE(PDS_MAC_FCNT_UP);
				result = LORAWAN_SUCCESS;
			}
		}
		break;

		case DOWNLINK_COUNTER:
		{
			uint32_t fcntDown = *(uint32_t *)attrValue;
			if(fcntDown < FCNT_MAX)
			{
				loRa.fCntDown.value = fcntDown;
				PDS_STORE(PDS_MAC_FCNT_DOWN);
				result = LORAWAN_SUCCESS;
			}
		}
		break;

		case RX_DELAY1:
		{
			loRa.protocolParameters.receiveDelay1 = *(uint16_t *)attrValue;
			PDS_STORE(PDS_MAC_RX_DELAY_1);
			loRa.protocolParameters.receiveDelay2 = loRa.protocolParameters.receiveDelay1 + 1000;
			PDS_STORE(PDS_MAC_RX_DELAY_2);
			result = LORAWAN_SUCCESS;
		}
		break;
		
		case AGGREGATED_DUTYCYCLE:
		{
			 uint8_t maxDCycle = *(uint8_t *)attrValue;
			 if (maxDCycle <= 15)
			 {
				 loRa.aggregatedDutyCycle = maxDCycle;
				 result = LORAWAN_SUCCESS;
			 }			 
		}
		break;
		
		case JOINACCEPT_DELAY1:
		{
			loRa.protocolParameters.joinAcceptDelay1 = *(uint16_t *)attrValue;
			PDS_STORE(PDS_MAC_JOIN_ACCEPT_DELAY_1);
			result = LORAWAN_SUCCESS;
		}
		break;

		case JOINACCEPT_DELAY2:
		{
			loRa.protocolParameters.joinAcceptDelay2 = *(uint16_t *)attrValue;
			PDS_STORE(PDS_MAC_JOIN_ACCEPT_DELAY_2);
			result = LORAWAN_SUCCESS;
		}
		break;

		case ADR_ACKLIMIT:
		{
			loRa.protocolParameters.adrAckLimit = *(uint8_t *)attrValue;
			PDS_STORE(PDS_MAC_ADR_ACK_LIMIT);
			result = LORAWAN_SUCCESS;
		}
		break;

		case ADR_ACKDELAY:
		{
			loRa.protocolParameters.adrAckDelay = *(uint8_t *)attrValue;
			PDS_STORE(PDS_MAC_ADR_ACK_DELAY);
			result = LORAWAN_SUCCESS;
		}
		break;

		case RETRANSMITTIMEOUT:
		{
			loRa.protocolParameters.retransmitTimeout = *(uint16_t *)attrValue;
			PDS_STORE(PDS_MAC_RETRANSMIT_TIMEOUT);
			result = LORAWAN_SUCCESS;
		}
		break;

		case CNF_RETRANSMISSION_NUM:
		{
			loRa.maxRepetitionsConfirmedUplink = *(uint8_t *)attrValue;
			PDS_STORE(PDS_MAC_MAX_REP_CNF_UPLINK);

			result = LORAWAN_SUCCESS;
		}
		break;
		
		case UNCNF_REPETITION_NUM:
		{
			loRa.maxRepetitionsUnconfirmedUplink = *(uint8_t *)attrValue;
			result = LORAWAN_SUCCESS;
		}
		break;

		case BATTERY:
		{
			loRa.batteryLevel = *(uint8_t *)attrValue;
			result = LORAWAN_SUCCESS;
		}
		break;

		case AUTOREPLY:
		{
			loRa.macStatus.automaticReply = *(bool *)attrValue;
			PDS_STORE(PDS_MAC_LORAWAN_STATUS);
			result = LORAWAN_SUCCESS;
		}
		break;

		case CH_PARAM_STATUS:
		{
			ChannelParameters_t *ch_param;
			ch_param = (ChannelParameters_t *)attrValue;
			result = LorawanSetChannelIdStatus(ch_param->channelId,ch_param->channelAttr.status);
		}
		break;

		case CH_PARAM_DR_RANGE:
		{
			ChannelParameters_t *ch_param;
			ch_param = (ChannelParameters_t *)attrValue;
			result = LorawanSetDataRange(ch_param->channelId,ch_param->channelAttr.dataRange);
		}
		break;

		case CH_PARAM_FREQUENCY:
		{
			ChannelParameters_t *ch_param;
			ch_param = (ChannelParameters_t *)attrValue;
			result = LorawanSetFrequency(ch_param->channelId,ch_param->channelAttr.frequency);
		}
		break;

		case RX2_WINDOW_PARAMS:
		{
			ReceiveWindow2Params_t *rx2_params;
			rx2_params = (ReceiveWindow2Params_t *)attrValue;
			result = LorawanSetReceiveWindow2Parameters(rx2_params->frequency,rx2_params->dataRate);
		}
		break;
		
		case RXC_WINDOW_PARAMS:
		{
			ReceiveWindowParameters_t *rxc_params;
			rxc_params = (ReceiveWindowParameters_t *)attrValue;
			result = LorawanSetReceiveWindowCParameters(rxc_params->frequency , rxc_params->dataRate);	
		}
        break;
		
		case EDCLASS:
		{
			EdClass_t ed_class;
			ed_class = *(EdClass_t *)attrValue;
			result = LorawanSetEdClass(ed_class);
		}
		break;

		case LINK_CHECK_PERIOD:
		{
			LorawanLinkCheckConfigure(*(uint16_t *)attrValue);
			result = LORAWAN_SUCCESS;
		}
		break;

		case FHSS_CALLBACK:
			fhssCallback = (FHSSCallback_t)attrValue;
		break;

		case MCAST_ENABLE:
		{
			LorawanMcastStatus_t *mcaststatus;
			 mcaststatus= (LorawanMcastStatus_t *)attrValue;
			result = LorawanMcastEnable(mcaststatus->status,mcaststatus->groupId);
		}
		break;

		case MCAST_APPS_KEY:
		{
			LorawanMcastAppSkey_t *appskey;
			appskey = (LorawanMcastAppSkey_t *)attrValue;
			result = LorawanAddMcastAppskey((appskey->mcastAppSKey),appskey->groupId);
		}
		break;

		case MCAST_NWKS_KEY:
		{
			LorawanMcastNwkSkey_t *nwkskey;
			nwkskey = (LorawanMcastNwkSkey_t *)attrValue;
			result = LorawanAddMcastNwkskey((nwkskey->mcastNwkSKey),nwkskey->groupId);
		}
		break;

		case MCAST_GROUP_ADDR:
		{
			LorawanMcastDevAddr_t *devaddr;
			devaddr = (LorawanMcastDevAddr_t *)attrValue;
			result = LorawanAddMcastAddr(devaddr->mcast_dev_addr,devaddr->groupId);
		}
		break;
        case MCAST_FCNT_DOWN_MIN:
        {
            LorawanMcastFcnt_t *fcnt = (LorawanMcastFcnt_t *)attrValue;
            result = LorawanAddMcastFcntMin(fcnt->fcntValue, fcnt->groupId);
        }
        break;
        case MCAST_FCNT_DOWN_MAX:
        {
            LorawanMcastFcnt_t *fcnt = (LorawanMcastFcnt_t *)attrValue;
            result = LorawanAddMcastFcntMax(fcnt->fcntValue, fcnt->groupId);
        }
        break;
        case MCAST_FREQUENCY:
        {
            LorawanMcastDlFreqeuncy_t *dlFreq = (LorawanMcastDlFreqeuncy_t *)attrValue;
            result = LorawanAddMcastDlFrequency(dlFreq->dlFrequency, dlFreq->groupId);
        }
        break;
        case MCAST_DATARATE:
        {
            LorawanMcastDatarate_t *dr = (LorawanMcastDatarate_t *)attrValue;
            result = LorawanAddMcastDatarate(dr->datarate, dr->groupId);
        }
        break;
        case MCAST_PERIODICITY:
        {
            LorawanMcastPeriodicity_t *per = (LorawanMcastPeriodicity_t *)attrValue;
            result = LorawanAddMcastPeriodicity(per->periodicity, per->groupId);
        }
        break;
		case TEST_MODE_ENABLE:
		{
			bool testModeEnable;
			testModeEnable = *(bool *)attrValue;
			bool joinBackoffEnable = false;
			if(testModeEnable)
			{
				if(loRa.featuresSupported & DUTY_CYCLE_SUPPORT)
				{
					loRa.featuresSupported &= ~DUTY_CYCLE_SUPPORT;
				}
				if(loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
				{
					loRa.featuresSupported &= ~JOIN_BACKOFF_SUPPORT;
					LORAREG_SetAttr(JOINBACKOFF_CNTL,&joinBackoffEnable);
				}	
			}
			else
			{
				LORAREG_GetAttr(SUPPORTED_REGIONAL_FEATURES,NULL,&(loRa.featuresSupported));				
			}
			result = LORAWAN_SUCCESS;
		}
		break;	
		case JOIN_BACKOFF_ENABLE:
		{
			bool joinBackoffEnable;
			joinBackoffEnable = *(bool *)attrValue;
			if(joinBackoffEnable)
			{
				loRa.featuresSupported |= JOIN_BACKOFF_SUPPORT;
				result = LORAREG_SetAttr(JOINBACKOFF_CNTL,&joinBackoffEnable);
			}
			else
			{
				if(loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
				{
					loRa.featuresSupported &= ~JOIN_BACKOFF_SUPPORT;
					result = LORAREG_SetAttr(JOINBACKOFF_CNTL,&joinBackoffEnable);
				}
			}
		}
		break;
		case MAX_FCNT_PDS_UPDATE_VAL:
		{
			/* This value is used in terms of power of 2. The max value is 256 (2 ^ 8) */
			if((8 >= *(uint8_t *)attrValue))
			{
				loRa.maxFcntPdsUpdateValue = *(uint8_t *)attrValue;
				PDS_STORE(PDS_MAC_MAX_FCNT_INC);
				result = LORAWAN_SUCCESS;
			}
			else
			{
				result = LORAWAN_INVALID_PARAMETER;
			}
		}
		break;
		case CRYPTODEVICE_ENABLED:
		{
			loRa.cryptoDeviceEnabled = *(bool *)attrValue;
			result = LORAWAN_SUCCESS;
		}
		break;
        case SEND_DEVICE_TIME_CMD:
        {
            result = EncodeDeviceTimeReq();
        }
        break;
        case SEND_LINK_CHECK_CMD:
        {
            result = EncodeLinkCheckReq();
        }
        break;
        case REGIONAL_DUTY_CYCLE:
        {
            if (*(bool *)attrValue)
            {
                loRa.featuresSupported |= DUTY_CYCLE_SUPPORT;
            }
            else
            {
                loRa.featuresSupported &= ~DUTY_CYCLE_SUPPORT;
            }
            result = LORAWAN_SUCCESS;
        }
        break;
            case JOIN_NONCE_TYPE:
            {
                loRa.joinNonceType = *(JoinNonceType_t *) attrValue;
                result = LORAWAN_SUCCESS;
            }
                break;
		default:
			result = LORAWAN_INVALID_PARAMETER;
		break;
		}
	}
	else
	{
		result = LORAWAN_BUSY;
	}
	
    return result;
}

StackRetStatus_t LORAWAN_GetAttr(LorawanAttributes_t attrType, void *attrInput, void *attrOutput)
{
    StackRetStatus_t result = LORAWAN_SUCCESS;
    switch(attrType)
    {

    case DEV_EUI:
        memcpy(attrOutput, loRa.activationParameters.deviceEui.buffer, sizeof(loRa.activationParameters.deviceEui));
    break;


	case LORAWAN_LBT_PARAMS:
	{
			 RadioLBTParams_t radioLBTParamsLocal;
			 LorawanLBTParams_t lorawanLBTParamsLocal;
			 RADIO_GetAttr(RADIO_LBT_PARAMS, (void *)&radioLBTParamsLocal);
			 lorawanLBTParamsLocal.maxRetryChannels = loRa.lbt.maxRetryChannels;
			 lorawanLBTParamsLocal.lbtNumOfSamples = radioLBTParamsLocal.lbtNumOfSamples;
			 lorawanLBTParamsLocal.lbtScanPeriod = radioLBTParamsLocal.lbtScanPeriod;
			 lorawanLBTParamsLocal.lbtThreshold = radioLBTParamsLocal.lbtThreshold;
			 lorawanLBTParamsLocal.lbtTransmitOn = radioLBTParamsLocal.lbtTransmitOn;
			 memcpy(attrOutput, (void *)&lorawanLBTParamsLocal, sizeof(LorawanLBTParams_t));
	}
	break;


    case JOIN_EUI:
        memcpy(attrOutput, loRa.activationParameters.joinEui.buffer, sizeof(loRa.activationParameters.joinEui));
    break;

    case DEV_ADDR:
        *(uint32_t *)attrOutput = loRa.activationParameters.deviceAddress.value;
    break;

    case NWKS_KEY:
        memcpy(attrOutput, loRa.activationParameters.networkSessionKeyRom, sizeof(loRa.activationParameters.networkSessionKeyRom));
    break;

    case APPS_KEY:
        memcpy(attrOutput, loRa.activationParameters.applicationSessionKeyRom, sizeof(loRa.activationParameters.applicationSessionKeyRom));
    break;

    case APP_KEY:
        memcpy(attrOutput, loRa.activationParameters.applicationKey, sizeof(loRa.activationParameters.applicationKey));
    break;

    case ADR:
        *(bool *)attrOutput = loRa.macStatus.adr;
    break;

    case CURRENT_DATARATE:
        *(uint8_t *)attrOutput = loRa.currentDataRate;
    break;

    case TX_POWER:
        *(uint8_t *)attrOutput = loRa.txPower;
    break;

    case SYNC_WORD:
        *(uint8_t *)attrOutput = loRa.syncWord;
    break;

    case UPLINK_COUNTER:
        *(uint32_t *)attrOutput = loRa.fCntUp.value;
    break;

    case DOWNLINK_COUNTER:
        *(uint32_t *)attrOutput = loRa.fCntDown.value;
    break;

    case RX_DELAY1:
        *(uint16_t *)attrOutput = loRa.protocolParameters.receiveDelay1;
    break;

    case RX_DELAY2:
        *(uint16_t *)attrOutput = loRa.protocolParameters.receiveDelay2;
    break;

    case JOINACCEPT_DELAY1:
        *(uint16_t *)attrOutput = loRa.protocolParameters.joinAcceptDelay1;
    break;

    case JOINACCEPT_DELAY2:
        *(uint16_t *)attrOutput = loRa.protocolParameters.joinAcceptDelay2;
    break;

    case MAX_FCOUNT_GAP:
        *(uint8_t *)attrOutput = loRa.protocolParameters.maxFcntGap;
    break;

    case ADR_ACKLIMIT:
        *(uint8_t *)attrOutput = loRa.protocolParameters.adrAckLimit;
    break;

    case ADR_ACKDELAY:
        *(uint8_t *)attrOutput = loRa.protocolParameters.adrAckDelay;
    break;

    case RETRANSMITTIMEOUT:
		*(uint16_t *)attrOutput = loRa.protocolParameters.retransmitTimeout;
    break;

    case CNF_RETRANSMISSION_NUM:
        *(uint8_t *)attrOutput = loRa.maxRepetitionsConfirmedUplink;
    break;
	
	case UNCNF_REPETITION_NUM:
	*(uint8_t *)attrOutput = loRa.maxRepetitionsUnconfirmedUplink;
	break;

    case BATTERY:
        *(uint8_t *)attrOutput = loRa.batteryLevel;
    break;

    case AUTOREPLY:
        *(bool *)attrOutput = loRa.macStatus.automaticReply;
    break;

    case LINK_CHECK_GWCNT:
        *(uint8_t *)attrOutput = loRa.linkCheckGwCnt;
    break;

    case LINK_CHECK_MARGIN:
        *(uint8_t *)attrOutput = loRa.linkCheckMargin;
    break;

    case AGGREGATED_DUTYCYCLE:
        *(uint16_t *)attrOutput = loRa.aggregatedDutyCycle;
    break;

    case LORAWAN_STATUS:
        *(uint32_t *)attrOutput = loRa.macStatus.value;
    break;

    case CH_PARAM_STATUS:
    {
        ChannelAttr_t *ch_param;
        ChannelId_t ch_idx;
        ch_param = (ChannelAttr_t *)attrOutput;
        ch_idx = *(ChannelId_t *)attrInput;
        LORAREG_GetAttr(CHANNEL_ID_STATUS,&(ch_idx),&(ch_param->status));
    }
    break;

    case CH_PARAM_DR_RANGE:
    {
        ChannelAttr_t *ch_param;
        ChannelId_t ch_idx;

        ch_param = (ChannelAttr_t *)attrOutput;
        ch_idx = *(ChannelId_t *)attrInput;
        LORAREG_GetAttr(DATA_RANGE,&(ch_idx),&(ch_param->dataRange));
    }
    break;

    case CH_PARAM_FREQUENCY:
    {
        ChannelAttr_t *ch_param;
        ChannelId_t ch_idx;

        ch_param = (ChannelAttr_t *)attrOutput;
        ch_idx = *(ChannelId_t *)attrInput;
        LORAREG_GetAttr(FREQUENCY,&(ch_idx),&(ch_param->frequency));
    }
    break;

    case RX2_WINDOW_PARAMS:
        LorawanGetReceiveWindow2Parameters((ReceiveWindow2Params_t *)attrOutput);
    break;
	
	case RXC_WINDOW_PARAMS:
	    LorawanGetReceiveWindowCParameters((ReceiveWindowParameters_t *)attrOutput);
	break;

    case ISMBAND:
        *(uint8_t *)attrOutput = loRa.ismBand;
    break;

    case EDCLASS:
        *(EdClass_t *)attrOutput = loRa.edClass;
    break;

    case EDCLASS_SUPPORTED:
        *(uint8_t *)attrOutput = loRa.edClassesSupported;
    break;

    case MACVERSION:
        //TODO:
    break;

    case LINK_CHECK_PERIOD:
        *(uint16_t *)attrOutput = (loRa.periodForLinkCheck)/1000UL;
    break;


    case FHSS_CALLBACK:
        attrOutput = (void *)fhssCallback;
    break;

    case MCAST_ENABLE:
	{
		uint8_t groupId = *(uint8_t *)attrInput;
		if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
		{
			*(bool *)attrOutput = (loRa.mcastParams.mcastGroupMask & ( 0x01 << (groupId))) ;
		}
		else
		{
			result = LORAWAN_INVALID_PARAMETER;
		}
	}	
        
	break;

    case MCAST_APPS_KEY:
	{
		uint8_t groupId = *(uint8_t *)attrInput;
		if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
		{
			memcpy((uint8_t *)attrOutput, loRa.mcastParams.activationParams[groupId].mcastAppSKey, LORAWAN_SESSIONKEY_LENGTH);
		}
		else
		{
			result = LORAWAN_INVALID_PARAMETER;
		}
			
	}
    break;

    case MCAST_NWKS_KEY:
	{
		uint8_t groupId = *(uint8_t *)attrInput;
		if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
		{		
			memcpy((uint8_t *)attrOutput, loRa.mcastParams.activationParams[groupId].mcastNwkSKey, LORAWAN_SESSIONKEY_LENGTH);
		}
		else
		{
			result = LORAWAN_INVALID_PARAMETER;
		}
					
	}	
    break;

    case MCAST_GROUP_ADDR:
	{
		uint8_t groupId = *(uint8_t *)attrInput;
		if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
		{
			memcpy((uint8_t *)attrOutput, &(loRa.mcastParams.activationParams[groupId].mcastDevAddr.value), sizeof(uint32_t));
		}
		else
		{
			result = LORAWAN_INVALID_PARAMETER;
		}
			
	}	
    break;

    case MCAST_FCNT_DOWN:
    {
        uint8_t groupId = *(uint8_t *)attrInput;
        if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
        {
            *(uint32_t *)attrOutput = loRa.mcastParams.activationParams[groupId].mcastFCntDown.value;
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }
    }
    break;

    case MCAST_FCNT_DOWN_MIN:
	{
		uint8_t groupId = *(uint8_t *)attrInput;
		if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
		{
			*(uint32_t *)attrOutput = loRa.mcastParams.activationParams[groupId].mcastFCntDownMin.value;
		}
		else
		{
			result = LORAWAN_INVALID_PARAMETER;
		}
		
	}        
	break;

    case MCAST_FCNT_DOWN_MAX:
    {
        uint8_t groupId = *(uint8_t *)attrInput;
        if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
        {
            *(uint32_t *)attrOutput = loRa.mcastParams.activationParams[groupId].mcastFCntDownMax.value;
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }
        
    }
    break;

    case MCAST_FREQUENCY:
    {
        uint8_t groupId = *(uint8_t *) attrInput;
        if ( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId )
        {
            *(uint32_t *)attrOutput = loRa.mcastParams.activationParams[groupId].dlFrequency;
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }
    }
    break;

    case MCAST_DATARATE:
    {
        uint8_t groupId = *(uint8_t *) attrInput;
        if ( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId )
        {
            *(uint8_t *)attrOutput = loRa.mcastParams.activationParams[groupId].datarate;
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }
    }
    break;

    case MCAST_PERIODICITY:
    {
        uint8_t groupId = *(uint8_t *) attrInput;
        if ( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId )
        {
            *(uint8_t *)attrOutput = loRa.mcastParams.activationParams[groupId].periodicity;
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }
    }
    break;

	case SUPPORTED_BANDS:
        result = LORAREG_SupportedBands((uint16_t *)attrOutput);
	break;
	case LAST_PACKET_RSSI:
	{
		RADIO_GetAttr(PACKET_RSSI_VALUE, attrOutput);
	}
	break;
	case IS_FPENDING:
	{
		*((bool *)attrOutput) = loRa.lorawanMacStatus.fPending;
	}
	break;
	case DL_ACK_REQD:
	{
		*((uint8_t *)attrOutput) = loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage;
	}
	break;
	case LAST_CH_ID:
	{
		result = LORAREG_GetAttr (CURRENT_CHANNEL_INDEX,attrInput,attrOutput);
	}
	break;
	case PENDING_DUTY_CYCLE_TIME:
	{
		result = LORAREG_GetAttr (MIN_DUTY_CYCLE_TIMER,&(loRa.currentDataRate), attrOutput);
	}
	break;
	case RETRY_COUNTER_CNF:
	{
		*(uint8_t *)attrOutput = loRa.counterRepetitionsConfirmedUplink;
	}
	break;
		case RETRY_COUNTER_UNCNF:
	{
		*(uint8_t *)attrOutput = loRa.counterRepetitionsUnconfirmedUplink;
	}
	break;
	case NEXT_PAYLOAD_SIZE:
	{
		uint8_t foptsFlag = false;
		uint8_t macReplyLen = CountfOptsLength(&foptsFlag);
		uint8_t macPayloadLen = LorawanGetMaxPayloadSize(loRa.currentDataRate);
	
		if (foptsFlag) {
		*(uint8_t *)attrOutput = macPayloadLen - macReplyLen;			
		}
		else{
		*(uint8_t *)attrOutput = 0;		
		}
	}
	break;
	case PENDING_JOIN_DUTY_CYCLE_TIME:
	{
		result = LORAREG_GetAttr(JOIN_DUTY_CYCLE_TIMER,NULL,attrOutput);
	}
	break;
	case JOIN_BACKOFF_ENABLE:
	{
		bool joinBackoffEnable = false;
		if (loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
		{
			joinBackoffEnable = true;
		}
		*(uint8_t *)attrOutput = joinBackoffEnable;
	}
	break;
    case STACK_VERSION:
    {
        *(uint32_t *)attrOutput = loRa.stackVersion.value;
    }
    break;
    case DEVICE_GPS_EPOCH_TIME:
    {
        uint64_t timeDiff, nowGps, thenGps;
        uint32_t sse = loRa.devTime.gpsEpochTime.secondsSinceEpoch;
        if ( sse == UINT32_MAX)
        {
            result = LORAWAN_INVALID_REQUEST;
        }
        else
        {
            nowGps = UINT64_MAX;
            SwTimerReadTimestamp(loRa.devTime.sysEpochTimeIndex, &timeDiff);
            timeDiff = SwTimerGetTime() - timeDiff;
            timeDiff = US_TO_MS(timeDiff);
            thenGps = (uint64_t) loRa.devTime.gpsEpochTime.secondsSinceEpoch;
            thenGps = thenGps * (uint64_t)1000uL /* to millis */;
            thenGps += (4u /* each frac_sec is ~3.99ms */ * loRa.devTime.gpsEpochTime.fractionalSecond);
            nowGps = thenGps + timeDiff;
            *(uint64_t *)attrOutput = nowGps;
        }       
    }
    break;
    case PACKET_TIME_ON_AIR:
    {
        TimeOnAirParams_t *toaParams = (TimeOnAirParams_t *)attrInput;
        *(uint32_t *) attrOutput = calcPacketTimeOnAir(toaParams->dr, toaParams->preambleLen, toaParams->impHdrMode, toaParams->crcOn, toaParams->cr, toaParams->pktLen);
    }
    break;
    case REGIONAL_DUTY_CYCLE:
    {
      *(bool *)attrOutput = (bool)(loRa.featuresSupported & DUTY_CYCLE_SUPPORT);
    }
    break;
        case JOIN_NONCE_TYPE:
        {
            *(JoinNonceType_t *) attrOutput = loRa.joinNonceType;
        }
            break;
    default:
        result = LORAWAN_INVALID_PARAMETER;
    break;
    }

    return result;

}

// This Function will try to retransmit the packet in the buffer which is already encrypted.
static void retransmitPacketInBuffer(void)
{
    radioConfig_t radioConfig;
    RadioTransmitParam_t RadioTransmitParam;
    NewTxChannelReq_t newTxChannelReq;
    RadioError_t status = 0;
    newTxChannelReq.transmissionType = true;
    newTxChannelReq.txPwr = loRa.txPower;
    newTxChannelReq.currDr = loRa.currentDataRate;

	if (LORAREG_GetAttr (NEW_TX_CHANNEL_CONFIG,&newTxChannelReq,&radioConfig) == LORAWAN_SUCCESS)
	{
		RadioReceiveParam_t RadioReceiveParam;
		/* Stop radio receive before transmission */
		RadioReceiveParam.action = RECEIVE_STOP;
		RADIO_Receive(&RadioReceiveParam);
			
		ConfigureRadioTx(radioConfig);
		RadioTransmitParam.bufferLen = loRa.lastPacketLength;
		RadioTransmitParam.bufferPtr = &macBuffer[16];
		//resend the last packet		
		status = RADIO_Transmit (&RadioTransmitParam);
		if (status == ERR_NONE)
		{
			loRa.macStatus.macState = TRANSMISSION_OCCURRING;
		}
		else
		{
			if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == ENABLED)
			{
				loRa.lbt.elapsedChannels = 0;
				PDS_STORE(PDS_MAC_LBT_PARAMS);
				ResetParametersForConfirmedTransmission ();
				MacClearCommands();
				UpdateTransactionCompleteCbParams((StackRetStatus_t)status);//Map to radio transmit status
				return;
			}
			else
			{
				loRa.lbt.elapsedChannels = 0;
				PDS_STORE(PDS_MAC_LBT_PARAMS);
				ResetParametersForUnconfirmedTransmission ();
				MacClearCommands();
				UpdateTransactionCompleteCbParams((StackRetStatus_t)status);//Map to radio transmit status
				return;
			}
		}
	}
	else
	{
		if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == ENABLED)
		{
			loRa.lbt.elapsedChannels = 0;
			PDS_STORE(PDS_MAC_LBT_PARAMS);
			ResetParametersForConfirmedTransmission ();
			MacClearCommands();
			UpdateTransactionCompleteCbParams((StackRetStatus_t)status);//radiotransmitcallback
			return;
		}
		else
		{
			loRa.lbt.elapsedChannels = 0;
			PDS_STORE(PDS_MAC_LBT_PARAMS);
			ResetParametersForUnconfirmedTransmission ();
			MacClearCommands();
			UpdateTransactionCompleteCbParams((StackRetStatus_t)status);//radiotransmitcallback
			return;
		}
	}
}

void LORAWAN_TxDone(void *param)
{
	RadioCallbackParam_t localParam = *(RadioCallbackParam_t *)param;
	LorawanLBTParams_t localLorawanLBTParams;
	LORAWAN_GetAttr(LORAWAN_LBT_PARAMS, NULL, (void *)&localLorawanLBTParams);

	if (loRa.macStatus.macPause == DISABLED)
	{
		if (callbackBackup == RADIO_TX_TIMEOUT_CALLBACK)
		{
			handleTransmissionTimeoutCallback();
		}
		else if (callbackBackup == RADIO_TX_DONE_CALLBACK)
		{
			if(loRa.ecrConfig.override == true)
			{
				loRa.ecrConfig.override = false;
				RADIO_SetAttr(ERROR_CODING_RATE,(void *)&(loRa.ecrConfig.ecr));
			}

			if (ERR_CHANNEL_BUSY == localParam.status)
			{
				if (true == localLorawanLBTParams.lbtTransmitOn)
				{
					if(loRa.lorawanMacStatus.joining == true)
					{
						loRa.lbt.elapsedChannels = 0;
						PDS_STORE(PDS_MAC_LBT_PARAMS);
						SetJoinFailState(LORAWAN_RADIO_CHANNEL_BUSY);
						return;
					}
									
					loRa.lbt.elapsedChannels++;
					PDS_STORE(PDS_MAC_LBT_PARAMS);
					if (((INFINITE_CHANNEL_RETRY == loRa.lbt.maxRetryChannels) || (loRa.lbt.maxRetryChannels > loRa.lbt.elapsedChannels)) && (loRa.retransmission == ENABLED))
					{
						retransmitPacketInBuffer();
						return;
					}
					else
					{
						loRa.lbt.elapsedChannels = 0;
						PDS_STORE(PDS_MAC_LBT_PARAMS);
						loRa.macStatus.macState = IDLE;
						loRa.lorawanMacStatus.syncronization = DISABLED;
						if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == ENABLED)
						{		
							ResetParametersForConfirmedTransmission ();
							MacClearCommands();		
						}
						else
						{
							ResetParametersForUnconfirmedTransmission ();
							MacClearCommands();
						}
						UpdateTransactionCompleteCbParams(LORAWAN_RADIO_CHANNEL_BUSY);
						return;
					}
				}
			}
			else if (ERR_NONE == localParam.status)
			{
				Rx1WindowParamsReq_t rx1WindowParamsReq;
				Rx1WindowParams_t rx1WindowParams;
				int8_t rxWindowOffset1,rxWindowOffset2;
				LorawanSendReq_t *LoRaCurrentSendReq = (LorawanSendReq_t *)loRa.appHandle;

				loRa.lbt.elapsedChannels = 0;
				PDS_STORE(PDS_MAC_LBT_PARAMS);
				if ((0 == loRa.counterRepetitionsUnconfirmedUplink) && (0 == loRa.counterRepetitionsConfirmedUplink))
				{
					if (ENABLED == loRa.macStatus.networkJoined)
					{
						loRa.fCntUp.value ++;  // the uplink frame counter increments for every new transmission (it does not increment for a retransmission)
						/* If maxFcntPdsUpdateValue is '0', means every-time the Frame counter will be updated in PDS */
						if((0 == loRa.maxFcntPdsUpdateValue) || (0 == (loRa.fCntUp.value & ((1 << loRa.maxFcntPdsUpdateValue) - 1))))
						{
							PDS_STORE(PDS_MAC_FCNT_UP);
						}
						if (LORAWAN_CNF == LoRaCurrentSendReq->confirmed)
						{
							loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage = ENABLED;
							loRa.counterRepetitionsConfirmedUplink++;
						}
						else
						{
							loRa.counterRepetitionsUnconfirmedUplink++;
						}
					}
				}
				else
				{
					if (ENABLED == loRa.macStatus.networkJoined)
					{
						if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == DISABLED)
						{
							loRa.counterRepetitionsUnconfirmedUplink ++ ; //for each retransmission, the counter increments
						}
						else // if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == ENABLED)
						{
							loRa.counterRepetitionsConfirmedUplink ++ ; //for each retransmission (if possible or not), the counter increments
						}
					}
				}

				//This flag is used when the reception in RX1 is overlapping the opening of RX2
				loRa.rx2DelayExpired = 0;

				loRa.macStatus.macState = BEFORE_RX1;

				rx1WindowParamsReq.currDr = loRa.currentDataRate;
				rx1WindowParamsReq.drOffset = loRa.offset;
				rx1WindowParamsReq.joining = loRa.lorawanMacStatus.joining;

				LORAREG_GetAttr(RX1_WINDOW_PARAMS,&rx1WindowParamsReq,&rx1WindowParams);
				if (loRa.lorawanMacStatus.joining == 1)
				{
					uint8_t dataRate = 0;
					LORAREG_GetAttr(DEFAULT_RX2_DATA_RATE,NULL,&(dataRate));
					LORAREG_GetAttr(RX_WINDOW_OFFSET,&(dataRate),&(rxWindowOffset2));
				}
				else
				{
					LORAREG_GetAttr(RX_WINDOW_OFFSET,&(loRa.receiveWindow2Parameters.dataRate),&(rxWindowOffset2));
				}
				loRa.receiveWindow1Parameters.dataRate = rx1WindowParams.rx1Dr;
				loRa.receiveWindow1Parameters.frequency = rx1WindowParams.rx1Freq;

				LORAREG_GetAttr(RX_WINDOW_OFFSET,&(loRa.receiveWindow1Parameters.dataRate),&(rxWindowOffset1));
		

				// the join request should never exceed 0.1%
				if (loRa.lorawanMacStatus.joining == 1)
				{
					uint32_t timeout1 = (uint32_t) (loRa.protocolParameters.joinAcceptDelay1 + rxWindowOffset1);
					uint32_t timeout2 = (uint32_t) (loRa.protocolParameters.joinAcceptDelay2 + rxWindowOffset2);
					SwTimerStart(loRa.joinAccept1TimerId, MS_TO_US(timeout1 - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)LorawanReceiveWindow1Callback, NULL);
					SwTimerStart(loRa.joinAccept2TimerId, MS_TO_US(timeout2 - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)LorawanReceiveWindow2Callback, NULL);
					if(loRa.featuresSupported & JOIN_BACKOFF_SUPPORT)
					{
					loRa.joinreqinfo.joinReqTimeOnAir= localParam.TX.timeOnAir;		
					if(loRa.joinreqinfo.isFirstJoinReq && loRa.joinreqinfo.firstJoinReqTimestamp ==0)
					{		
						
						bool startJoinBackOffTimer=true;
						LORAREG_SetAttr(JOIN_BACK_OFF_TIMER,&startJoinBackOffTimer);
						loRa.joinreqinfo.firstJoinReqTimestamp = (SwTimerGetTime() - localParam.TX.timeOnAir);
						
											
					}
					UpdateJoinDutyCycleTimer_t UpdateJoinDutyCycleTimer;
					UpdateJoinDutyCycleTimer.joinreqTimeonAir = loRa.joinreqinfo.joinReqTimeOnAir;
					UpdateJoinDutyCycleTimer.startJoinDutyCycleTimer = true;
					LORAREG_SetAttr(JOIN_DUTY_CYCLE_TIMER,&UpdateJoinDutyCycleTimer);
					}
				}
				else
				{	
					uint32_t timeout1 = (uint32_t) (loRa.protocolParameters.receiveDelay1 + rxWindowOffset1);
					uint32_t timeout2 = (uint32_t) (loRa.protocolParameters.receiveDelay2 + rxWindowOffset2);
					SwTimerStart(loRa.receiveWindow1TimerId, MS_TO_US(timeout1 - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)LorawanReceiveWindow1Callback, NULL);
					SwTimerStart(loRa.receiveWindow2TimerId, MS_TO_US(timeout2 - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)LorawanReceiveWindow2Callback, NULL);
					if (CLASS_C == loRa.edClass)
					{
						loRa.enableRxcWindow = true;
						LorawanClasscTxDone(rxWindowOffset2);
					}
				}

				if((loRa.featuresSupported & DUTY_CYCLE_SUPPORT) || (loRa.aggregatedDutyCycle != 0))
				{
					UpdateDutyCycleTimer_t  update_dutyCycle_timer;
					update_dutyCycle_timer.timeOnAir = localParam.TX.timeOnAir;
					update_dutyCycle_timer.joining = loRa.lorawanMacStatus.joining;
					update_dutyCycle_timer.aggDutyCycle = 1 << loRa.aggregatedDutyCycle;// Execute 2^maxDCycle here
					LORAREG_SetAttr(DUTY_CYCLE_TIMER,&update_dutyCycle_timer);
				}
				else if(loRa.featuresSupported & LBT_SUPPORT)
				{
					/*Update LBT Timer*/
					LORAREG_SetAttr(LBT_TIMER,NULL);
				}			
				/*
				* Start a timer for GPS_EPOCH_TRACK_DURATION_MS duration to track the GPS time received in Ans
				*/
				if( loRa.devTime.isDevTimeReqSent )
				{
                    SwTimestamp_t curtime = SwTimerGetTime();
                    loRa.devTime.isDevTimeReqSent = false;
					SwTimerWriteTimestamp(loRa.devTime.sysEpochTimeIndex, &curtime);
				}
			}
		}
	}
	else
	{		
		if((AppPayload.AppData != NULL))
		{
			StackRetStatus_t status = LORAWAN_RADIO_SUCCESS;
			
			//Standalone radio transmissions finished OK, using the same callback as in LoRaWAN tx
			if (callbackBackup == RADIO_TX_TIMEOUT_CALLBACK)
			{   
				// Radio transmission ended by watchdog timer
				status = LORAWAN_RADIO_TX_TIMEOUT;
			}
			else
			{
				if (ERR_CHANNEL_BUSY == localParam.status)
				{
					status = LORAWAN_RADIO_CHANNEL_BUSY;
				}
			}		

			loRa.cbPar.evt = LORAWAN_EVT_TRANSACTION_COMPLETE;
			loRa.cbPar.param.transCmpl.status = status;
			AppPayload.AppData (loRa.appHandle, &loRa.cbPar);

		}
			
	}
}

void LorawanCheckAndDoRetryOnTimeout(void)
{
    /*
        class C:
            TODO move the below code to retry timer callback
            if (radio is not in receive)
                add assert print

            if (retry count exceeded)
                if (UNCNF)
                    return transaction complete with success
                else
                    return transaction complete with fail
                continue doing RX2
            else
                increment retry count
                radio_transmit()

                if (!last retry)
                    restart RX2 timer with same timeout
                else
                    restart RX2 timer with (RXDelay2+Preamble time)

        class c
    	TODO rework as this will not get hit because of SW use for timeout
        if (state is RX2_OPEN)
            if (rx2ReceiveDuration is infinite)
                assert, as it should not hit here
            else
                if (retry count exceeded)
                    if (UNCNF)
                        send txn complete to app as success
                        configure rx2ReceiveDuration = infinite
                        radio_receive(RX2)
                    else
                        send txn complete to app as fail
                        configure rx2ReceiveDuration = infinite
                        clear ackRequiredFromNextDownlinkMessage
                        radio_receive(RX2)
                else
                    if (UNCNF)
                        radio_transmit()
                    else
                        start ackTimeoutTimerId
    */

    /*
        Class C:

        In all the failure cases
            configure rx2ReceiveDuration to infinite duration.
            Notify application of failure
            Restart RX with RX2 parameters
    */
	
    if (loRa.lorawanMacStatus.ackRequiredFromNextDownlinkMessage == ENABLED) // if last uplink packet was confirmed, we have to send this packet by the number indicated by NbRepConfFrames
    {
        if ((loRa.counterRepetitionsConfirmedUplink <= loRa.maxRepetitionsConfirmedUplink) && (loRa.retransmission == ENABLED))
        {
            if (CLASS_A == loRa.edClass)
            {
                loRa.macStatus.macState = RETRANSMISSION_DELAY;
                SwTimerStart(loRa.ackTimeoutTimerId, MS_TO_US(loRa.protocolParameters.retransmitTimeout - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)AckRetransmissionCallback, NULL);				
            }
            else if (CLASS_C == loRa.edClass)
            {
                /* initiate transmit as we already accounted for RETRANSMIT_TIMEOUT in Tx Done */
                 uint16_t maximumPacketSize = LorawanGetMaxPayloadSize (loRa.currentDataRate) + HDRS_MIC_PORT_MIN_SIZE;
                 // after changing the dataRate, we must check if the size of the packet is still valid
                 if (loRa.lastPacketLength <= maximumPacketSize)
                 {
	                 LorawanGetChAndInitiateRadioTransmit();
                 }
                 else
                 {
					 UpdateTransactionCompleteCbParams(LORAWAN_INVALID_BUFFER_LENGTH);
	                 ResetParametersForConfirmedTransmission ();
					 MacClearCommands();
	                 
                 }
            }
        }
        else
        {
            /* Transaction complete Event */
            UpdateTransactionCompleteCbParams(LORAWAN_NO_ACK);//retry no ack received
			ResetParametersForConfirmedTransmission ();
			MacClearCommands();

            if ( (CLASS_C == loRa.edClass) &&
                 (RADIO_STATE_RX != RADIO_GetState()))
            {
                SYS_ASSERT_ERROR(ASSERT_MAC_UNCNFTXRETRY_RXSTATEFAIL);
            }
        }
    }
    else
    {
        if ((loRa.counterRepetitionsUnconfirmedUplink <= loRa.maxRepetitionsUnconfirmedUplink)  && (loRa.retransmission == ENABLED))
        {
            /* initiate transmit immediately */
            LorawanGetChAndInitiateRadioTransmit();

        }
        else
        {
            ResetParametersForUnconfirmedTransmission ();
			MacClearCommands();
		
            /* Transaction complete Event */
            // inform the application layer that no message was received back from the server
            UpdateTransactionCompleteCbParams(LORAWAN_SUCCESS);
        }
    }
}

// this function is called by the radio when the first or the second receive window expired without receiving any message (either for join accept or for message)
void LORAWAN_RxTimeout(void)
{
    if (loRa.macStatus.macPause == DISABLED)
    {
        if ((CLASS_C == loRa.edClass) && (true == loRa.macStatus.networkJoined))
        {  
			loRa.enableRxcWindow = true;
            LorawanClasscRxTimeout();
        }
        else
        {
            // if the timeout is after the first receive window, we have to wait for the second receive window....
            if ( loRa.macStatus.macState == RX1_OPEN )
            {
                loRa.macStatus.macState = BETWEEN_RX1_RX2;
            }
            else
            {
                // if last message sent was a join request, the join was not accepted after the second window expired
                if (loRa.lorawanMacStatus.joining == 1)
                {
                    SetJoinFailState(LORAWAN_RADIO_BUSY);
                }
                // if last message sent was a data message, and there was no reply...
                else if (loRa.macStatus.networkJoined == 1)
                {
                    LorawanCheckAndDoRetryOnTimeout();
                }
            }
        }
	}
	else
	{
		//Standalone radio reception NOK, using the same callback as in LoRaWAN rx
		/* Transaction complete Event */
		//UpdateTransactionCompleteCbParams(RADIO_NOT_OK);
         if (AppPayload.AppData != NULL)
         {
	         loRa.isTransactionDone = true;
	         loRa.cbPar.evt = LORAWAN_EVT_TRANSACTION_COMPLETE;
	         loRa.cbPar.param.transCmpl.status = LORAWAN_RADIO_NO_DATA;
	         AppPayload.AppData (loRa.appHandle, &loRa.cbPar);
         }
	}
}

static void TransmissionErrorCallback (void)
{
    RadioTransmitParam_t RadioTransmitParam;
    radioConfig_t radioConfig;
    NewTxChannelReq_t newTxChannelReq;
    bool isTxDone = false;

    newTxChannelReq.transmissionType = true;
    newTxChannelReq.txPwr = loRa.txPower;
    newTxChannelReq.currDr = loRa.currentDataRate;

    if (LORAREG_GetAttr (NEW_TX_CHANNEL_CONFIG,&newTxChannelReq,&radioConfig) == LORAWAN_SUCCESS)
    {
		RadioReceiveParam_t RadioReceiveParam;
		/* Stop the receive before transmit */
		RadioReceiveParam.action = RECEIVE_STOP;
		RADIO_Receive(&RadioReceiveParam);
	
        ConfigureRadioTx(radioConfig);
        RadioTransmitParam.bufferLen = loRa.lastPacketLength;
        RadioTransmitParam.bufferPtr = &macBuffer[16];
        if (RADIO_Transmit (&RadioTransmitParam) != ERR_NONE)
        {
            if(CLASS_A == loRa.edClass)
            {
                loRa.macStatus.macState = RETRANSMISSION_DELAY;
            }
        }
        else
        {
            isTxDone = true;
        }
    }

    if (false == isTxDone)
    {
        /* keep retrying forever */
        SwTimerStart(loRa.transmissionErrorTimerId, MS_TO_US(TRANSMISSION_ERROR_TIMEOUT - loRa.radioClkStableDelay), SW_TIMEOUT_RELATIVE, (void *)TransmissionErrorCallback, NULL);
    }
}

static StackRetStatus_t CreateAllSoftwareTimers (void)
{
	StackRetStatus_t retVal = LORAWAN_SUCCESS;
	
    if (LORAWAN_SUCCESS == retVal)
	{
		retVal = SwTimerCreate(&loRa.joinAccept1TimerId);
	}
	
	if (LORAWAN_SUCCESS == retVal)
	{
		retVal = SwTimerCreate(&loRa.joinAccept2TimerId);
	}
	
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.receiveWindow1TimerId);
	}
	
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.receiveWindow2TimerId);
	}
	
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.linkCheckTimerId);
	}

    if (LORAWAN_SUCCESS == retVal)
    {	
		retVal = SwTimerCreate(&loRa.ackTimeoutTimerId);
	}

    if (LORAWAN_SUCCESS == retVal)
    {	
		retVal = SwTimerCreate(&loRa.automaticReplyTimerId);
	}
	
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.unconfirmedRetransmisionTimerId);
	}

    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.abpJoinTimerId);
	}
    
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.transmissionErrorTimerId);
	}
	
    if (LORAWAN_SUCCESS == retVal)
    {
		retVal = SwTimerCreate(&loRa.classCParams.ulAckTimerId);
	}

    if (LORAWAN_SUCCESS == retVal)
    {
        retVal = SwTimerTimestampCreate(&loRa.devTime.sysEpochTimeIndex);
    }
	
	if (LORAWAN_SUCCESS != retVal)
	{
		/* free the allocated timers, if any */
		SwTimerReset();
	}
	
	return retVal;
}

static void StopAllSoftwareTimers (void)
{
    SwTimerStop(loRa.joinAccept1TimerId);
    SwTimerStop(loRa.joinAccept2TimerId);
    SwTimerStop(loRa.linkCheckTimerId);
    SwTimerStop(loRa.receiveWindow1TimerId);
    SwTimerStop(loRa.receiveWindow2TimerId);
    SwTimerStop(loRa.ackTimeoutTimerId);
    SwTimerStop(loRa.automaticReplyTimerId);
    SwTimerStop(loRa.unconfirmedRetransmisionTimerId);
    SwTimerStop(loRa.abpJoinTimerId);
    SwTimerStop(loRa.transmissionErrorTimerId);
    SwTimerStop(loRa.classCParams.ulAckTimerId);
}

void LorawanConfigureRadioForRX2(bool doCallback)
{
    RadioReceiveParam_t RadioReceiveParam;
	uint8_t dataRate = 0;
	uint32_t frequency = 0;
	
	if(loRa.enableRxcWindow == true)
	{
		dataRate = loRa.receiveWindowCParameters.dataRate;
		frequency = loRa.receiveWindowCParameters.frequency;
	}
	else
	{
	    if (loRa.lorawanMacStatus.joining == 1)
	    {
		    LORAREG_GetAttr(DEFAULT_RX2_DATA_RATE,NULL,&(dataRate));
	    }
        else 
	    {
            dataRate = loRa.receiveWindow2Parameters.dataRate;
        }
		frequency = loRa.receiveWindow2Parameters.frequency;
	}
    ConfigureRadioRx( dataRate, frequency);
    RadioReceiveParam.action = RECEIVE_START;
    if ((CLASS_C == loRa.edClass) && (true == loRa.macStatus.networkJoined) && (true == loRa.enableRxcWindow))
    {
        RadioReceiveParam.rxWindowSize = CLASS_C_RX2_WINDOW_SIZE;
    }
    else
	{
		LORAREG_GetAttr(RX_WINDOW_SIZE,&(dataRate),&(RadioReceiveParam.rxWindowSize));
	}

    RadioError_t status;
	status = RADIO_Receive(&RadioReceiveParam);
    if (status != ERR_NONE)
    {
        ResetParametersForConfirmedTransmission ();
        ResetParametersForUnconfirmedTransmission ();
		MacClearCommands();

        if (true == doCallback)
        {
            /* Send Transaction complets callback */
           UpdateTransactionCompleteCbParams((StackRetStatus_t)status);//radio receive error callback
        }
    }

}

// This function checks which value can be assigned to the current data rate.
void FindSmallestDataRate (void)
{
    uint8_t  dataRate;
    bool found = 0;

    if (loRa.currentDataRate > loRa.minDataRate)
    {
        dataRate = loRa.currentDataRate - 1;

        while ( (found == 0) && (dataRate >= loRa.minDataRate) )
        {
			if (LORAREG_ValidateAttr(TX_DATARATE,&dataRate) == LORAWAN_SUCCESS)
			{			
				if(LORAREG_ValidateAttr(SUPPORTED_DR,&dataRate) == LORAWAN_SUCCESS)
				{
					found = 1;
					break;
				}
				/*
            				for ( i = 0; i < loRa.maxChannels; i++ )
            				{
            					if ( (dataRate >= Channels[i].dataRange.min) && (dataRate <= Channels[i].dataRange.max ) && ( Channels[i].status == ENABLED ) )
            					{
            						found = 1;
            						break;
            					}
            				}*/
				if ( (found == 0) &&  (dataRate > loRa.minDataRate) ) // if no channels were found after one search, then the device will switch to the next lower data rate possible
				{
					dataRate = dataRate - 1;
				}
			}
			else
			{
				break;
			}
        }

        if ( found == 1)
        {
            loRa.currentDataRate = dataRate;
        }
    }
}


void UpdateCurrentDataRateAfterDataRangeChanges (void)
{
    if (loRa.currentDataRate > loRa.maxDataRate)
    {
        loRa.currentDataRate = loRa.maxDataRate;
    }

    if (loRa.currentDataRate < loRa.minDataRate)
    {
        loRa.currentDataRate = loRa.minDataRate;
    }
}

/*********************************************************************//**
\brief	This function calls the respective callback function of the
		lorawan layer from radio layer.

\param	callback	- The callback that is propagated from the radio layer.
\param	param		- The callback parameters.
\return		- none.
*************************************************************************/
static void radioCallback(RadioCallbackID_t callback, void *param)

{
    callbackBackup = callback;

    switch (callback)
    {
    case RADIO_TX_DONE_CALLBACK:
	case RADIO_TX_TIMEOUT_CALLBACK:
        LORAWAN_TxDone(param);
        break;

    case RADIO_RX_DONE_CALLBACK:
    case RADIO_RX_ERROR_CALLBACK:
    case RADIO_RX_TIMEOUT_CALLBACK:
        /* callbackBackup is assumed to stay the same after MAC context switch */
        LORAWAN_PostTask(LORAWAN_RX_TASK_ID);
        break;
    case RADIO_FHSS_NEXT_FREQ_CALLBACK:
        if(fhssCallback)
        {
            *(uint32_t *)param = fhssCallback();
        }
        break;

    default:
        break;
    }
}

/*********************************************************************//**
\brief	This function sets the Class of the device

\param	edclass	    - Class of the device
\return		        - Function returns the status of the operation (StackRetStatus_t).
*************************************************************************/
StackRetStatus_t LorawanSetEdClass(EdClass_t edclass)
{
    StackRetStatus_t result;

    result = LORAWAN_SUCCESS;

    if (false == loRa.isTransactionDone)
    {
        result = LORAWAN_BUSY;
    }
    else if (!(loRa.edClassesSupported & edclass))
    {
        result = LORAWAN_INVALID_PARAMETER;
    }
    else if (edclass != loRa.edClass)
    {
        /*
            Switching from Class A to Class C
                Move to RX2_OPEN
                start receive

            Switching from Class C to Class A
                Move to Idle
                Stop receive
                Stop appropriate timers
        */

        if ((CLASS_A == loRa.edClass) &&
                (CLASS_C == edclass))
        {
            loRa.edClass = edclass;
			PDS_STORE(PDS_MAC_ED_CLASS);
        }
        else if ((CLASS_C == loRa.edClass)&&
                 (CLASS_A == edclass))
        {
            RadioReceiveParam_t RadioReceiveParam;

            loRa.edClass = edclass;
			PDS_STORE(PDS_MAC_ED_CLASS);
            loRa.macStatus.macState = IDLE;
            RadioReceiveParam.action = RECEIVE_STOP;
            if (ERR_NONE != RADIO_Receive(&RadioReceiveParam))
            {
                SYS_ASSERT_ERROR(0);
            }
            /* Only timer that may possibly running after Transaction done is ULACKTimer, Stop it */
            SwTimerStop(loRa.classCParams.ulAckTimerId);
        }
        else
        {
            result = LORAWAN_INVALID_PARAMETER;
        }

    }

	return result;
}

/**
 * @Summary
    This function returns the readiness of the stack for sleep
 * @Description
    This function is used for querying the readiness of the stack for sleep.
	This function has dependency on RADIO for corresponding readiness check function in TAL.
 * @Preconditions
    None
 * @Param
    \deviceResetAfterSleep -- 'true' means device will reset during wakeup
                              'false' means device does not reset during wakeup
 * @Returns
    'true' if stack is in ready state for sleep, otherwise 'false'
 * @Example
 *  bool status = LORAWAN_ReadyToSleep(false);
*/
bool LORAWAN_ReadyToSleep(bool deviceResetAfterSleep)
{
  bool ready = false;

	switch (loRa.edClass)
	{
		case CLASS_A:
		case CLASS_C:
		{
			
			if ((IDLE == loRa.macStatus.macState))//||(BEFORE_RX1 == loRa.macStatus.macState)||(BETWEEN_RX1_RX2==loRa.macStatus.macState))
			{
				ready = true;
			}
			break;
		}

		default:
		break;
	}
 
  return ready;
}

/**
 * @Summary
    This function stops the ReceiveWindow2 timer 
	if Device received packet already in RX1 state
*/

static void StopReceiveWindow2Timer(void)
{
	if (CLASS_A  == loRa.edClass)
	{
		loRa.macStatus.macState = IDLE;
		if (SwTimerIsRunning(loRa.receiveWindow2TimerId))
		{
			/* Stop the receive window 2 timer if it is running
				Since Class A device will not receive packet in both receive slots */ 
			SwTimerStop (loRa.receiveWindow2TimerId);
		}
				
	}
	
}

static void handleTransmissionTimeoutCallback(void)
{
	loRa.macStatus.macState = IDLE;
	if (true == loRa.macStatus.networkJoined)
	{
		UpdateTransactionCompleteCbParams(LORAWAN_TX_TIMEOUT);
	}
	else
	{
		SetJoinFailState(LORAWAN_TX_TIMEOUT);
	}
}

/*********************************************************************//**
\brief  This function returns the packet time on air for a given length
\param
    \datarate    - Data rate to be used for packet transmission
    \preambleLen - Length of preamble (unit: symbols -- Lora, bytes -- FSK)
    \impHdrMode  - If packet to be sent in implicit header mode
    \crcOn       - If PHY CRC is to be added in transmitted packet
    \cr          - Coding rate of the transmitted packet
    \length      - Length of payload in bytes to be transmitted in packet
\return
    'uint32_t' Time-on-Air of packet for the given payload length
*************************************************************************/
static uint32_t calcPacketTimeOnAir(uint8_t datarate, uint8_t preambleLen,
    uint8_t impHdrMode, uint8_t crcOn, uint8_t cr, uint8_t length)
{
    bool lowDataRateOptimize = false;
    double bw, ts, tp, tmpDouble, time = 0.0;
    uint32_t np;
    RadioModulation_t modulation;
    RadioDataRate_t sf;

    LORAREG_GetAttr(SPREADING_FACTOR_ATTR, &datarate, &sf);
    LORAREG_GetAttr(MODULATION_ATTR, &datarate, &modulation);

    if (MODULATION_LORA == modulation)
    {
        RadioLoRaBandWidth_t loraBw;
		RADIO_GetAttr(BANDWIDTH, &loraBw);
		
		if (( SF_12 == sf && ( BW_125KHZ == loraBw || BW_250KHZ == loraBw ) ) || \
			( SF_11 == sf && BW_125KHZ == loraBw ))
		{
			lowDataRateOptimize = true;
		}

        LORAREG_GetAttr(BANDWIDTH_ATTR, &datarate, &loraBw);

        switch (loraBw)
        {
            case BW_125KHZ:
            {
                bw = 125000.0;
                break;
            }

            case BW_250KHZ:
            {
                bw = 250000.0;
                break;
            }

            case BW_500KHZ:
            {
                bw = 500000.0;
                break;
            }

            default:
            {
                bw = 0.0;
                break;
            }
        }

        /* Refer: SX1272 data sheet section 4.1.1.7. Time on air */

        /* Compute Ts, time per symbol */
        ts = 1 / (bw / (1 << sf));

        /* Compute Tpreamble, preamble time-on-air */
        tp = (preambleLen + 4.25) * ts;

        /* Compute in steps, Npayload, number of panPayloadload snPayloadmbols */
        tmpDouble  = ((8 * length) - (4 * sf) + 28 + (16 * crcOn) - (impHdrMode ? 20 : 0));
        tmpDouble /= (4 * (sf - (lowDataRateOptimize ? 2 : 0)));

        /* Ceil this tmpDouble */
        np = (uint32_t) tmpDouble;
        if ((tmpDouble - (double)np) > 0.0)
        {
            np += 1;
        }

        np *= (cr + 4);
        np  = (np > 0) ? np : 0;
        np += 8;

        /*
        * Compute Tpacket...
        * The time on air, or packet duration, in simply then the sum of the preamble and payload duration.
        * Payload duration is then the symbol period multiplied by the number of Payload symbols
        */
	    time = (tp /* Tpreamble */ + (np /* Npayload */ * ts /* Symbol time */));
	    time = MS_TO_US(1000 * time);
    }
    else
    {
        time = (RADIO_PHY_FSK_PREAMBLE_BYTES_LENGTH + length) * 8 * 20 /* us: time-per-bit in FSK */;
    }

    return (uint32_t) time;
}

static void lorawanADR(FCtrl_t *fCtrl)
{
    /*
     * ADR procedure implementation as per the following table in V1.0.4 specification 
     *
     * +------------+-----------+---------------+-----------+-----------+----------------------------------------------------------+
     * | ADRACKCnt  | ADRACKReq |   Data Rate   | TX Power  |  NbTrans  |  Channel Mask                                            |
     * +------------+-----------+---------------+-----------+-----------+----------------------------------------------------------+
     * | 0-63       |         0 | DR1           | Max-9dBm  | 3         | Normal operation chmask                                  |
     * | 64 to 95   |         1 | No change     | No change | No change | No change                                                |
     * | 96 to 127  |         1 | No change     | Default   | No change | No change                                                |
     * | 128 to 159 |         1 | DR0 (Default) | Default   | No change | No change                                                |
     * | ? 160      |         1 | DR0 (Default) | Default   | 1         | For dynamic channel plans: re-enable default channels.   |
     * |            |           |               |           |           | For fixed channel plans: All channels enabled            |
     * +------------+-----------+---------------+-----------+-----------+----------------------------------------------------------+
     */

    uint8_t defaultTxPower;
    uint8_t nbTrans;

    /*
     * Step 1:
     * Each time the uplink frame counter is incremented (for each new uplink
     * frame, because repeated transmissions do not increment the frame counter),
     * the end-device SHALL increment an ADRACKCnt counter.
     */
    loRa.adrAckCnt = 1 + loRa.adrAckCnt;
    if (loRa.adrAckCnt < loRa.protocolParameters.adrAckLimit)
    {
        return;
    }

    /*
     * Step 2:
     * After ADR_ACK_LIMIT uplinks (ADRACKCnt ? ADR_ACK_LIMIT) without receiving
     * a Class A downlink response, the end-device SHALL set the ADR acknowledgment
     * request bit (ADRACKReq) on uplink transmissions.
     */
    fCtrl->adrAckReq = ENABLED;
    loRa.lorawanMacStatus.adrAckRequest = ENABLED;

    if (loRa.adrAckCnt == loRa.protocolParameters.adrAckLimit)
    {
        loRa.counterAdrAckDelay = 0;
    }
    else if (loRa.adrAckCnt > loRa.protocolParameters.adrAckLimit)
    {
        loRa.counterAdrAckDelay++;

        /*
         * Step 3:
         * If no Class A downlink frame is received within the next ADR_ACK_DELAY
         * uplinks (i.e., after a total of ADR_ACK_LIMIT + ADR_ACK_DELAY transmitted frames),
         * the end-device SHALL try to regain connectivity by first setting the
         * TX power to the default power, then switching to the next-lower data rate
         * that provides a longer radio range. The end-device SHALL further lower its
         * data rate step by step every time ADR_ACK_DELAY uplink frames are transmitted.
         */
        if (loRa.counterAdrAckDelay >= loRa.protocolParameters.adrAckDelay)
        {
            loRa.counterAdrAckDelay = 0;

            LORAREG_GetAttr(DEF_TX_PWR, NULL, &defaultTxPower);
            
            if ((loRa.adrAckCnt > 95) && (loRa.adrAckCnt < 128))
            {
                /* Between 95 to 127 only TXPower can change */
                if (loRa.txPower != defaultTxPower)
                {
                    setDefaultTxPower(loRa.ismBand);
                    LORAREG_GetAttr(REG_DEF_TX_POWER, NULL, &(loRa.txPower));
                    PDS_STORE(PDS_MAC_TX_POWER);
                }
            }
            else if (loRa.adrAckCnt > 127)
            {
                /* Between 127 to 160  DR MUST be lowered first if possible */
                if (loRa.currentDataRate != loRa.minDataRate)
                {
                    FindSmallestDataRate();
                }
                /* Between 127 to 159, no other change is allowed even if DR has
                 * already reached the minimum
                 */
                else if (loRa.adrAckCnt >= 160)
                {
                    /*
                     * Step 4:
                     * Once the end-device has reached the default data rate,
                     * and transmitted for ADR_ACK_DELAY uplinks with ADRACKReq=1
                     * without receiving a downlink, it SHALL re-enable all default
                     * uplink frequency channels and reset Nbtrans to its default value of 1.
                     */
                    nbTrans = 0;
                    LORAREG_EnableallChannels(loRa.ismBand);

                    /* After Enabling all the Default channels, Check for min and max DataRate device can support */
                    MinMaxDr_t minmaxDr;

                    LORAREG_GetAttr(MIN_MAX_DR, NULL, &(minmaxDr));
                    loRa.minDataRate = minmaxDr.minDr;
                    loRa.maxDataRate = minmaxDr.maxDr;

                    /* Set the Lowest datarate possible if Current DaraRate is not a Lowest DataRate, the band can support */
                    FindSmallestDataRate();

                    loRa.maxRepetitionsConfirmedUplink = nbTrans;
                    loRa.maxRepetitionsUnconfirmedUplink = nbTrans;

                    //LORAWAN_SetAttr(CNF_RETRANSMISSION_NUM, &nbTrans);
                    //LORAWAN_SetAttr(UNCNF_REPETITION_NUM, &nbTrans);
                    PDS_STORE(PDS_MAC_MAX_REP_UNCNF_UPLINK);
                    loRa.macStatus.nbRepModified = 1;
                    PDS_STORE(PDS_MAC_LORAWAN_STATUS);
                }
            }
        }
    }
}

static StackRetStatus_t checkRxPacketPayloadLen(uint8_t bufferLength, Hdr_t *hdr)
{
	uint8_t maximumPacketSize = 0;
    uint8_t frmPayloadLength = 0;
   
	if ((loRa.macStatus.macState == RX1_OPEN))
	{	    
		maximumPacketSize = LorawanGetMaxPayloadSize(loRa.receiveWindow1Parameters.dataRate);	
	}
	else
	{
		maximumPacketSize = LorawanGetMaxPayloadSize(loRa.receiveWindow2Parameters.dataRate);
	}
	if (bufferLength > (HDRS_MIC_PORT_MIN_SIZE + hdr->members.fCtrl.fOptsLen))
	{
		frmPayloadLength = bufferLength - hdr->members.fCtrl.fOptsLen - HDRS_MIC_PORT_MIN_SIZE;
	}
	else
	{
		frmPayloadLength = 0; //No payload received
	}

	if (frmPayloadLength > maximumPacketSize)
	{
		return LORAWAN_INVALID_PACKET;
	}
	else
	{
		return LORAWAN_SUCCESS;
	}
}

/* eof lorawan.c */
