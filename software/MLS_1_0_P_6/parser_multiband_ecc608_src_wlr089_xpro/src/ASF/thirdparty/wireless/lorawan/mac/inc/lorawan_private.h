/**
* \file  lorawan_private.h
*
* \brief LoRaWAN private header file
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
 
#ifndef _LORAWAN_PRIVATE_H
#define	_LORAWAN_PRIVATE_H

#ifdef	__cplusplus
extern "C" {
#endif

/****************************** INCLUDES **************************************/
#include "compiler.h"
#include "lorawan_defs.h"
#include "sal.h"

/****************************** DEFINES ***************************************/ 
#define INVALID_VALUE         0xFF

#define RESPONSE_OK                             1
#define RESPONSE_NOT_OK                         0

#define RESERVED_FOR_FUTURE_USE                 0

#define MAXIMUM_BUFFER_LENGTH                   271

//Data rate (DR) encoding
#define DR0                                     0
#define DR1                                     1
#define DR2                                     2
#define DR3                                     3
#define DR4                                     4
#define DR5                                     5
#define DR6                                     6
#define DR7                                     7
#define DR8                                     8
#define DR9                                     9
#define DR10                                    10
#define DR11                                    11
#define DR12                                    12
#define DR13                                    13
#define DR14                                    14
#define DR15                                    15

#define MAJOR_VERSION3                          0

/* bit mask for MAC commands response */
#define CHANNEL_MASK_ACK                            0x01
#define DATA_RATE_ACK                               0x02
#define RX1_DR_OFFSET_ACK                           0x04
#define POWER_ACK                                   0x04
#define UPLINK_FREQ_EXISTS_ACK                      0x02

/* FPORT Range */
#define FPORT_MIN                               1
#define FPORT_MAX                               223

#define MAX_NB_CMD_TO_PROCESS					32

/* Converting FREQUENCY value to hertz */
#define MAC_CMD_FREQ_IN_HZ(frequency)            (frequency * 100)

/* Frequency value size in MAC Commands */
#define FREQ_VALUE_SIZE_IN_BYTES                  (3)

/* Take the last 3 bytes of the frequency */
#define MAC_CMD_FREQ_VALUE(frequency)            (frequency & 0x00FFFFFF)

//13 = sizeof(MIC) + MHDR + FHDR + sizeof (fPort);
#define HDRS_MIC_PORT_MIN_SIZE 13

/* Frame header 7 bytes and FPORT 1 byte */
#define FHDR_FPORT_SIZE         8

#define ABP_TIMEOUT_MS                          50

#define JA_JOIN_NONCE_SIZE                       3
#define JA_NET_ID_SIZE                          3

#define MAX_FOPTS_LEN                           0x0F
#define LORAWAN_MCAST_DEVADDR_DEFAULT           0xFFFFFFFF

#define LORAWAN_FHDR_SIZE_WITHOUT_FOPTS         8

#define LORAWAN_TEST_PORT                       224

/***************************** TYPEDEFS ***************************************/

typedef union
{
    uint8_t value;
	COMPILER_PACK_SET(1)
    struct
    {
       uint8_t nbRep:4;
       uint8_t chMaskCntl:3;
       uint8_t rfu:1;
    };
	COMPILER_PACK_RESET()
} Redundancy_t;

typedef enum
{
    IDLE                      =0,
    TRANSMISSION_OCCURRING      ,
    BEFORE_RX1                  ,         //between TX and RX1, FSK can occur
    RX1_OPEN                    ,
    BETWEEN_RX1_RX2             ,         //FSK can occur
    RX2_OPEN                    ,
    RETRANSMISSION_DELAY        ,         //used for ADR_ACK delay, FSK can occur
    ABP_DELAY                   ,         //used for delaying in calling the join callback for ABP
} LoRaMacState_t;

// types of frames
typedef enum
{
    FRAME_TYPE_JOIN_REQ         =0x00 ,
    FRAME_TYPE_JOIN_ACCEPT            ,
    FRAME_TYPE_DATA_UNCONFIRMED_UP    ,
    FRAME_TYPE_DATA_UNCONFIRMED_DOWN  ,
    FRAME_TYPE_DATA_CONFIRMED_UP      ,
    FRAME_TYPE_DATA_CONFIRMED_DOWN    ,
    FRAME_TYPE_RFU                    ,
    FRAME_TYPE_PROPRIETARY            ,
}LoRaMacFrameType_t;

// MAC commands CID
typedef enum
{
    LINK_CHECK_CID              = 0x02,
    LINK_ADR_CID                = 0x03,
    DUTY_CYCLE_CID              = 0x04,
    RX2_SETUP_CID               = 0x05,
    DEV_STATUS_CID              = 0x06,
    NEW_CHANNEL_CID             = 0x07,
    RX_TIMING_SETUP_CID         = 0x08,
    TX_PARAM_SETUP_CID          = 0x09,
    DL_CHANNEL_CID              = 0x0A,
    DEV_TIME_CID             = 0x0D
}LoRaMacCid_t;

//activation parameters
typedef union
{
    uint32_t value;
    uint8_t buffer[4];
} DeviceAddress_t;

typedef union
{
    uint32_t value;
    struct
    {
        uint16_t valueLow;
        uint16_t valueHigh;
    } members;
} FCnt_t;

//union used for instantiation of DeviceEui and Application Eui
typedef union
{
    uint8_t buffer[8];
    struct
    {
        uint32_t genericEuiL;
        uint32_t genericEuiH;
    }members;
} GenericEui_t;

COMPILER_PACK_SET(1)
typedef struct
{
    ActivationType_t activationType;
    DeviceAddress_t deviceAddress;
    uint8_t networkSessionKeyRom[LORAWAN_SESSIONKEY_LENGTH];
    uint8_t applicationSessionKeyRom[LORAWAN_SESSIONKEY_LENGTH];
	uint8_t networkSessionKeyRam[LORAWAN_SESSIONKEY_LENGTH];
	uint8_t applicationSessionKeyRam[LORAWAN_SESSIONKEY_LENGTH];
    uint8_t applicationKey[LORAWAN_SESSIONKEY_LENGTH];
    GenericEui_t joinEui;
    GenericEui_t deviceEui;
} ActivationParameters_t;
COMPILER_PACK_RESET()

typedef union
{
    uint8_t value;
	COMPILER_PACK_SET(1)
    struct
    {
       uint8_t fOptsLen:4;
       uint8_t fPending:1;
       uint8_t ack:1;
       uint8_t adrAckReq:1;
       uint8_t adr:1;
    };
	COMPILER_PACK_RESET()
} FCtrl_t;

// Mac header structure
typedef union
{
    uint8_t value;
	COMPILER_PACK_SET(1)
    struct
    {
        uint8_t major           : 2;
        uint8_t rfu             : 3;
        uint8_t mType           : 3;
    }bits;
	COMPILER_PACK_RESET()
} Mhdr_t;

typedef union
{
    uint8_t fHdrCounter[23];
	COMPILER_PACK_SET(1)
    struct
    {
        Mhdr_t mhdr             ;
        DeviceAddress_t devAddr ;
        FCtrl_t fCtrl           ;
        uint16_t fCnt           ;
        uint8_t MacCommands[15] ;
    }members;
	COMPILER_PACK_RESET()
} Hdr_t;

typedef union
{
    uint8_t value;
	COMPILER_PACK_SET(1)
    struct
    {
        uint8_t rx2DataRate     : 4;
        uint8_t rx1DROffset     : 3;
        uint8_t rfu             : 1;
    }bits;
	COMPILER_PACK_RESET()
} DlSettings_t;

//Protocol parameters
COMPILER_PACK_SET(1)
typedef struct
{
    uint16_t receiveDelay1     ;
    uint16_t receiveDelay2     ;
    uint16_t joinAcceptDelay1  ;
    uint16_t joinAcceptDelay2  ;
    uint16_t maxFcntGap        ;
    uint16_t retransmitTimeout        ;
    uint8_t adrAckLimit        ;
    uint8_t adrAckDelay        ;
} ProtocolParams_t;
typedef struct _JoinReqinfo_t
{
	
	uint64_t firstJoinReqTimestamp;
	uint32_t joinReqTimeOnAir;
	bool isFirstJoinReq;
	
} JoinReqinfo_t;
typedef struct
{
   uint8_t receivedCid;
   unsigned channelMaskAck :1;           // used for link adr answer
   unsigned dataRateAck :1;              // used for link adr answer
   unsigned powerAck :1;                 // used for link adr answer
   unsigned channelAck :1;               // used for RX param setup request
   unsigned dataRateReceiveWindowAck :1; // used for RX param setup request
   unsigned rx1DROffestAck :1;           // used for RX param setup request
   unsigned dataRateRangeAck :1;        // used for new channel answer
   unsigned channelFrequencyAck :1;     // used for new channel and DL Channel answer
   unsigned uplinkFreqExistsAck :1;     // used for DL channel answer
} LorawanCommands_t;

typedef struct  
{
	uint8_t channelMaskAck :1;
	uint8_t dataRateAck	   :1;
	uint8_t powerAck	   :1;
	uint8_t reserved       :5;
	uint8_t dataRate;
	uint8_t txPower;	
	uint8_t count;
	uint16_t channelMask;
	Redundancy_t redundancy;
} LinkAdrResp_t;

COMPILER_PACK_RESET()
typedef union
{
    uint16_t value;
	COMPILER_PACK_SET(1)
    struct
    {
       uint16_t deviceEui: 1;             //if set, device EUI was defined
       uint16_t joinEui:1;
       uint16_t deviceAddress: 1;
       uint16_t applicationKey:1;
       uint16_t networkSessionKey:1;
       uint16_t applicationSessionKey:1;
    };
	COMPILER_PACK_RESET()
} LorawanMacKeys_t;
COMPILER_PACK_SET(1)
typedef struct
{
    uint32_t frequency;
    uint8_t dataRate;
} ReceiveWindowParameters_t;
COMPILER_PACK_RESET()


/* Holds the Multicast group bit mask for Keys and Device adddress */
typedef union _LorawanMcastKeys {
	uint8_t value;
	COMPILER_PACK_SET(1)
	struct {
	   uint8_t mcastDeviceAddress: 1;
	   uint8_t mcastNetworkSessionKey: 1;
	   uint8_t mcastApplicationSessionKey: 1;		
		};
	COMPILER_PACK_RESET()
}LorawanMcastKeys_t;


/* Holds multicast parameter for one multicast group */
typedef struct _LorawanMcastActivationParams_t
{
	/** multicast group address to match DL frame */
	DeviceAddress_t mcastDevAddr;

	/** multicast session keys */
	uint8_t mcastNwkSKey[LORAWAN_SESSIONKEY_LENGTH];
	uint8_t mcastAppSKey[LORAWAN_SESSIONKEY_LENGTH];

	/** Downlink frame counter just for mcast group */
	FCnt_t mcastFCntDown;

    /* Downlink frame counter boundaries */
    FCnt_t mcastFCntDownMin;    
    FCnt_t mcastFCntDownMax;
	
    /* McGroup key mask */
    LorawanMcastKeys_t mcastKeysMask;

    /** Frequency and DR */
    uint32_t dlFrequency;
    uint8_t datarate;
    uint8_t periodicity;
} LorawanMcastActivationParams_t;

typedef struct _LorawanMcastParams_t
{
	/** number of mcast groups - currently 1 */
	uint8_t numSupportedMcastGroups;
	uint8_t mcastGroupMask;
	/** activation parameters for multicast downlink packet processing */
	/* TODO : change below to array when multiple mcast groups are supported */
	LorawanMcastActivationParams_t activationParams[LORAWAN_MCAST_GROUP_COUNT_SUPPORTED];
} LorawanMcastParams_t;

typedef union _JoinAccept
{
	uint8_t joinAcceptCounter[29];
	COMPILER_PACK_SET(1)
	struct
	{
		Mhdr_t mhdr;
		uint8_t joinNonce[3];
		uint8_t networkId[3];
		DeviceAddress_t deviceAddress;
		DlSettings_t DLSettings;
		uint8_t rxDelay;
		uint8_t cfList[16];
	} members;
	COMPILER_PACK_RESET()
} JoinAccept_t;

/*Structure for storing listen Before Talk parameters*/
typedef struct _LorawanLBT
{
	/* This field is used to count the number of channels elapsed */
	uint16_t elapsedChannels;
	/* This field controls the number of MAX channels to do lbt.
	 * If set to zero, lbt will be done for first packet and if set
	 * to 0xFFFF LBT is done till channel is got.
	 */
	uint16_t maxRetryChannels;
} LorawanLBT_t;
/*#endif*/ // LBT

typedef struct _ClassCParams_t
{
    /** Holds the duration of RX2 receive - will be a finite value even for
        infinite receive */
    uint32_t rx2ReceiveDuration;

    /** uplink ack timer */
    uint8_t ulAckTimerId;

} ClassCParams;

typedef struct _Lora
{
	ActivationParameters_t activationParameters;
	ReceiveWindowParameters_t receiveWindow1Parameters;
	ReceiveWindowParameters_t receiveWindow2Parameters;
	ReceiveWindowParameters_t receiveWindowCParameters;
	JoinReqinfo_t joinreqinfo;
	LorawanStatus_t macStatus;
	FCnt_t fCntUp;
	FCnt_t fCntDown;
	uint32_t periodForLinkCheck;
	LorawanMacStatus_t lorawanMacStatus;
	uint8_t aggregatedDutyCycle;
	LorawanCommands_t macCommands[MAX_NB_CMD_TO_PROCESS];
	uint16_t adrAckCnt;
	uint16_t devNonce;
	uint16_t lastPacketLength;
	ProtocolParams_t protocolParameters;
	LorawanMacKeys_t macKeys;
	uint8_t crtMacCmdIndex;
	uint8_t maxRepetitionsUnconfirmedUplink;
	uint8_t maxRepetitionsConfirmedUplink;
	uint8_t counterRepetitionsUnconfirmedUplink;
	uint8_t counterRepetitionsConfirmedUplink;
	//uint8_t lastUsedChannelIndex;
	uint8_t linkCheckMargin;
	uint8_t linkCheckGwCnt;
	uint8_t currentDataRate;
	uint8_t batteryLevel;
	uint8_t txPower;
	uint8_t deftxPower;
	uint8_t joinAccept1TimerId;
	uint8_t joinAccept2TimerId;
	uint8_t receiveWindow1TimerId;
	uint8_t receiveWindow2TimerId;
	uint8_t automaticReplyTimerId;
	uint8_t linkCheckTimerId;
	uint8_t ackTimeoutTimerId;
	uint8_t unconfirmedRetransmisionTimerId;
	uint8_t minDataRate;
	uint8_t maxDataRate;
	uint8_t maxChannels;
	uint8_t counterAdrAckDelay;
	uint8_t offset;
	bool macInitialized;
	bool rx2DelayExpired;
	bool abpJoinStatus;
	uint8_t abpJoinTimerId;
	uint8_t transmissionErrorTimerId;
	uint8_t edClass;
	uint8_t edClassesSupported;
	IsmBand_t ismBand;
	uint8_t syncWord;
	uint32_t evtmask;
	void *appHandle;
	appCbParams_t cbPar;
	FeaturesSupported_t featuresSupported;
	LorawanLBT_t lbt;
	ClassCParams classCParams;
	LorawanMcastParams_t mcastParams;
	bool isTransactionDone;
	ecrConfig_t ecrConfig;
	LinkAdrResp_t linkAdrResp;
	bool retransmission;
	uint8_t radioClkStableDelay;
	uint8_t maxFcntPdsUpdateValue;
	bool cryptoDeviceEnabled;
    DevTime_t devTime;
    StackVersion_t stackVersion;
	bool enableRxcWindow;
    uint32_t joinNonce;
	bool joinAcceptChMaskReceived;
    JoinNonceType_t joinNonceType;
} LoRa_t;


extern AppData_t AppPayload;

/*************************** FUNCTIONS PROTOTYPE ******************************/

// Callback functions
void LorawanReceiveWindow1Callback (void);

void LorawanReceiveWindow2Callback (void);

void LorawanLinkCheckCallback (void);

void AckRetransmissionCallback (void);

void UnconfirmedTransmissionCallback (void);

void AutomaticReplyCallback (void);

// Update and validation functions

void UpdateCurrentDataRate (uint8_t valueNew);

void UpdateDLSettings(uint8_t dlRx2Dr, uint8_t dlRx1DrOffset);

void UpdateTxPower (uint8_t txPowerNew);

void UpdateRetransmissionAckTimeoutState (void);

void UpdateJoinSuccessState(void);

void UpdateReceiveWindow2Parameters (uint32_t frequency, uint8_t dataRate);

void UpdateReceiveWindowCParameters(uint32_t frequency , uint8_t dataRate);

void UpdateJoinInProgress(uint8_t state);

void UpdateCurrentDataRateAfterDataRangeChanges (void);

//Initialization functions

void ResetParametersForConfirmedTransmission (void);

void ResetParametersForUnconfirmedTransmission (void);

void SetJoinFailState(StackRetStatus_t status);

uint16_t Random (uint16_t max);

// MAC commands transmission functions
StackRetStatus_t EncodeDeviceTimeReq(void);
StackRetStatus_t EncodeLinkCheckReq(void);
//MAC commands functions

uint8_t* ExecuteDutyCycle (uint8_t *ptr);

uint8_t* ExecuteLinkAdr (uint8_t *ptr);

uint8_t* ExecuteDevStatus (uint8_t *ptr);

uint8_t* ExecuteNewChannel (uint8_t *ptr);

uint8_t* ExecuteRxParamSetupReq   (uint8_t *ptr);

uint8_t* ExecuteDlChannel (uint8_t *ptr);

uint8_t* ExecuteTxParamSetup (uint8_t *ptr);

uint8_t* ExecuteDevTimeAns (uint8_t *ptr);

uint32_t GetRx1Freq (void);


void LorawanLinkCheckConfigure (uint16_t period);

void LorawanGetReceiveWindow2Parameters (ReceiveWindow2Params_t *rx2Params);

void LorawanGetReceiveWindowCParameters (ReceiveWindowParameters_t *rxcParams);

StackRetStatus_t LorawanSetReceiveWindow2Parameters (uint32_t frequency, uint8_t dataRate);

StackRetStatus_t LorawanSetReceiveWindowCParameters(uint32_t frequency, uint8_t dataRate);

uint32_t LorawanGetFrequency (uint8_t channelId);

StackRetStatus_t LorawanSetFrequency (uint8_t channelId, uint32_t frequencyNew);

StackRetStatus_t LorawanSetDataRange (uint8_t channelId, uint8_t dataRangeNew);

uint8_t LorawanGetDataRange (uint8_t channelId);

StackRetStatus_t LorawanSetChannelIdStatus (uint8_t channelId, bool statusNew);

bool LorawanGetChannelIdStatus (uint8_t channelId);

StackRetStatus_t LorawanSetDutyCycle (uint8_t channelId, uint16_t dutyCycleValue);

uint16_t LorawanGetDutyCycle (uint8_t channelId);

uint8_t LorawanGetIsmBand(void) ;

StackRetStatus_t LorawanSetEdClass(EdClass_t edclass);

// Helper Functions
void AssemblePacket (bool confirmed, uint8_t port, uint8_t *buffer, uint16_t bufferLength);

uint8_t PrepareJoinRequestFrame (void);

SalStatus_t EncryptFRMPayload (uint8_t* buffer, uint8_t bufferLength, uint8_t dir, uint32_t frameCounter, uint8_t* key, salItems_t key_type, uint16_t macBufferIndex, uint8_t* bufferToBeEncrypted, uint32_t devAddr);

void UpdateTransactionCompleteCbParams(StackRetStatus_t status);

void UpdateRxDataAvailableCbParams(uint32_t devAddr, uint8_t *pData,uint8_t dataLength,StackRetStatus_t status);

void LorawanCheckAndDoRetryOnTimeout(void);

void LorawanGetChAndInitiateRadioTransmit(void);

void LorawanConfigureRadioForRX2(bool doCallback);

void ConfigureRadioRx(uint8_t dataRate, uint32_t freq);

//Class C Functions

void LorawanClasscUlAckTimerCallback(uint8_t param);

StackRetStatus_t LorawanClasscValidateSend(void);

uint32_t LorawanClasscPause(void);

void LorawanClasscReceiveWindowCallback(void);

void LorawanClasscRxTimeout(void);

void LorawanClasscTxDone(int8_t rxWindowOffset2);

void LorawanClasscRxDone(Hdr_t *hdr);

void LorawanClasscNotifyAppOnReceive(uint32_t devAddr, uint8_t *pData,uint8_t dataLength, StackRetStatus_t status);

StackRetStatus_t LorawanProcessFcntDown(Hdr_t *hdr, bool isMulticast);

#ifdef	__cplusplus
}
#endif

#endif	/* _LORAWAN_PRIVATE_H */
