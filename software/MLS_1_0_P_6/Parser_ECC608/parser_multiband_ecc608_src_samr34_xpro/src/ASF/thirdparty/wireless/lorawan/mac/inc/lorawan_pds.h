/**
* \file  lorawan_pds.h
*
* \brief This file is for the PDS defs for LoraWAN Module
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
 

#ifndef MLS_LORA_PDS_H
#define	MLS_LORA_PDS_H

#ifdef	__cplusplus
extern "C" {
#endif


/* MAC PDS Items */

/* MAC Items Start Index */
#define MAC_PDS_FID1_START_INDEX    PDS_FILE_MAC_01_IDX << 8
#define MAC_PDS_FID2_START_INDEX    PDS_FILE_MAC_02_IDX << 8
   

/* PDS MAC Items - List*/
typedef enum _pds_mac_items_fid1
{
	PDS_MAC_ISM_BAND = MAC_PDS_FID1_START_INDEX,
	PDS_MAC_ED_CLASS,
	PDS_MAC_PERIOD_FOR_LINK_CHK,
	PDS_MAC_SYNC_WORD,
	PDS_MAC_EVENT_MASK,
	PDS_MAC_MCAST_FCNT_DWN,
	PDS_MAC_MCAST_DEV_ADDR,
	PDS_MAC_MCAST_APP_SKEY,
	PDS_MAC_MCAST_NWK_SKEY,
	PDS_MAC_MCAST_KEYS,
	PDS_MAC_MCAST_GROUP_MASK,
	PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR,
	PDS_MAC_LBT_PARAMS,
	PDS_MAC_TX_POWER,
	PDS_MAC_PRESCALR,
	PDS_MAC_RX_DELAY_1,
	PDS_MAC_RX_DELAY_2,
	PDS_MAC_JOIN_ACCEPT_DELAY_1,
	PDS_MAC_JOIN_ACCEPT_DELAY_2,
	PDS_MAC_RETRANSMIT_TIMEOUT,
	PDS_MAC_ADR_ACK_LIMIT,
	PDS_MAC_ADR_ACK_DELAY,
	PDS_MAC_MAX_REP_UNCNF_UPLINK,
	PDS_MAC_MAX_REP_CNF_UPLINK,	
	PDS_MAC_FCNT_UP,
	PDS_MAC_DEV_NONCE,
	PDS_MAC_FID1_MAX_VALUE  /* Always add new items above this value */	
} pds_mac_items_fid1_t;

typedef enum _pds_mac_items_fid2
{
	PDS_MAC_RX2_PARAMS = MAC_PDS_FID2_START_INDEX,
	PDS_MAC_RX1_OFFSET,
	PDS_MAC_ACTIVATION_TYPE,
	PDS_MAC_DEV_ADDR,
	PDS_MAC_NWK_SKEY,
	PDS_MAC_APP_SKEY,
	PDS_MAC_APP_KEY,
	PDS_MAC_JOIN_EUI,
	PDS_MAC_DEV_EUI,
	PDS_MAC_LORAWAN_MAC_KEYS,
	PDS_MAC_FCNT_DOWN,
	PDS_MAC_LORAWAN_STATUS,
	PDS_MAC_CURR_DR,
	PDS_MAC_MAX_FCNT_INC,
	PDS_MAC_CRYPTO_DEV_ENABLED,
    PDS_MAC_JOIN_NONCE,
	PDS_MAC_RXC_PARAMS,
	PDS_MAC_FID2_MAX_VALUE  /* Always add new items above this value */
} pds_mac_items_fid2_t;

#define PDS_MAC_ISM_BAND_ADDR					((uint8_t *)&(loRa.ismBand))
#define PDS_MAC_ED_CLASS_ADDR					((uint8_t *)&(loRa.edClass))
#define PDS_MAC_PERIOD_FOR_LINK_CHK_ADDR		((uint8_t *)&(loRa.periodForLinkCheck))
#define PDS_MAC_SYNC_WORD_ADDR					((uint8_t *)&(loRa.syncWord))
#define PDS_MAC_EVENT_MASK_ADDR					((uint8_t *)&(loRa.evtmask))
#define PDS_MAC_MCAST_FCNT_DWN_ADDR				((uint8_t *)&(loRa.mcastParams.activationParams[0].mcastFCntDown.value))
#define PDS_MAC_MCAST_DEV_ADDR_ADDR				((uint8_t *)&(loRa.mcastParams.activationParams[0].mcastDevAddr))
#define PDS_MAC_MCAST_APP_SKEY_ADDR				((uint8_t *)&(loRa.mcastParams.activationParams[0].mcastAppSKey))
#define PDS_MAC_MCAST_NWK_SKEY_ADDR				((uint8_t *)&(loRa.mcastParams.activationParams[0].mcastNwkSKey))
#define PDS_MAC_MCAST_KEYS_ADDR					((uint8_t *)&(loRa.mcastParams.activationParams[0].mcastKeysMask.value))
#define PDS_MAC_MCAST_GROUP_MASK_ADDR			((uint8_t *)&(loRa.mcastParams.mcastGroupMask))
#define PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR_ADDR ((uint8_t *)&(loRa.mcastParams.numSupportedMcastGroups))
#define PDS_MAC_LBT_PARAMS_ADDR					((uint8_t *)&(loRa.lbt))
#define PDS_MAC_TX_POWER_ADDR					((uint8_t *)&(loRa.txPower))
#define PDS_MAC_PRESCALR_ADDR					((uint8_t *)&(loRa.aggregatedDutyCycle))
#define PDS_MAC_RX_DELAY_1_ADDR					((uint8_t *)&(loRa.protocolParameters.receiveDelay1))
#define PDS_MAC_RX_DELAY_2_ADDR					((uint8_t *)&(loRa.protocolParameters.receiveDelay2))
#define PDS_MAC_JOIN_ACCEPT_DELAY_1_ADDR		((uint8_t *)&(loRa.protocolParameters.joinAcceptDelay1))
#define PDS_MAC_JOIN_ACCEPT_DELAY_2_ADDR		((uint8_t *)&(loRa.protocolParameters.joinAcceptDelay2))
#define PDS_MAC_RETRANSMIT_TIMEOUT_ADDR			((uint8_t *)&(loRa.protocolParameters.retransmitTimeout))
#define PDS_MAC_ADR_ACK_LIMIT_ADDR				((uint8_t *)&(loRa.protocolParameters.adrAckLimit))
#define PDS_MAC_ADR_ACK_DELAY_ADDR				((uint8_t *)&(loRa.protocolParameters.adrAckDelay))
#define PDS_MAC_MAX_REP_UNCNF_UPLINK_ADDR		((uint8_t *)&(loRa.maxRepetitionsUnconfirmedUplink))
#define PDS_MAC_MAX_REP_CNF_UPLINK_ADDR		    ((uint8_t *)&(loRa.maxRepetitionsConfirmedUplink))
#define PDS_MAC_RX2_PARAMS_ADDR					((uint8_t *)&(loRa.receiveWindow2Parameters))
#define PDS_MAC_RX1_OFFSET_ADDR					((uint8_t *)&(loRa.offset))
#define PDS_MAC_ACTIVATION_TYPE_ADDR			((uint8_t *)&(loRa.activationParameters.activationType))
#define PDS_MAC_DEV_ADDR_ADDR					((uint8_t *)&(loRa.activationParameters.deviceAddress))
#define PDS_MAC_NWK_SKEY_ADDR					((uint8_t *)&(loRa.activationParameters.networkSessionKeyRom))
#define PDS_MAC_APP_SKEY_ADDR					((uint8_t *)&(loRa.activationParameters.applicationSessionKeyRom))
#define PDS_MAC_APP_KEY_ADDR					((uint8_t *)&(loRa.activationParameters.applicationKey))
#define PDS_MAC_JOIN_EUI_ADDR					((uint8_t *)&(loRa.activationParameters.joinEui))
#define PDS_MAC_DEV_EUI_ADDR					((uint8_t *)&(loRa.activationParameters.deviceEui))
#define PDS_MAC_LORAWAN_MAC_KEYS_ADDR			((uint8_t *)&(loRa.macKeys))
#define PDS_MAC_FCNT_UP_ADDR					((uint8_t *)&(loRa.fCntUp.value))
#define PDS_MAC_DEV_NONCE_ADDR					((uint8_t *)&(loRa.devNonce))
#define PDS_MAC_FCNT_DOWN_ADDR					((uint8_t *)&(loRa.fCntDown.value))
#define PDS_MAC_LORAWAN_STATUS_ADDR				((uint8_t *)&(loRa.macStatus.value))
#define PDS_MAC_CURR_DR_ADDR					((uint8_t *)&(loRa.currentDataRate))
#define PDS_MAC_MAX_FCNT_INC_ADDR               ((uint8_t *)&(loRa.maxFcntPdsUpdateValue))
#define PDS_MAC_CRYPTO_DEV_ENABLED_ADDR			((uint8_t *)&(loRa.cryptoDeviceEnabled))
#define PDS_MAC_JOIN_NONCE_ADDR                 ((uint8_t *)&(loRa.joinNonce))
#define PDS_MAC_RXC_PARAMS_ADDR					((uint8_t *)&(loRa.receiveWindowCParameters))

/* PDS MAC Items Size */

#define PDS_MAC_ISM_BAND_SIZE					sizeof(loRa.ismBand)
#define PDS_MAC_ED_CLASS_SIZE					sizeof(loRa.edClass)
#define PDS_MAC_PERIOD_FOR_LINK_CHK_SIZE		sizeof(loRa.periodForLinkCheck)
#define PDS_MAC_SYNC_WORD_SIZE					sizeof(loRa.syncWord)
#define PDS_MAC_EVENT_MASK_SIZE					sizeof(loRa.evtmask)
#define PDS_MAC_MCAST_FCNT_DWN_SIZE				sizeof(loRa.mcastParams.activationParams[0].mcastFCntDown.value)
#define PDS_MAC_MCAST_DEV_ADDR_SIZE				sizeof(loRa.mcastParams.activationParams[0].mcastDevAddr)
#define PDS_MAC_MCAST_APP_SKEY_SIZE				sizeof(loRa.mcastParams.activationParams[0].mcastAppSKey)
#define PDS_MAC_MCAST_NWK_SKEY_SIZE				sizeof(loRa.mcastParams.activationParams[0].mcastNwkSKey)
#define PDS_MAC_MCAST_KEYS_SIZE					sizeof(loRa.mcastParams.activationParams[0].mcastKeysMask.value)
#define PDS_MAC_MCAST_GROUP_MASK_SIZE			sizeof(loRa.mcastParams.mcastGroupMask)
#define PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR_SIZE   sizeof(loRa.mcastParams.numSupportedMcastGroups)
#define PDS_MAC_LBT_PARAMS_SIZE					sizeof(loRa.lbt)
#define PDS_MAC_TX_POWER_SIZE					sizeof(loRa.txPower)
#define PDS_MAC_PRESCALR_SIZE					sizeof(loRa.aggregatedDutyCycle)
#define PDS_MAC_RX_DELAY_1_SIZE					sizeof(loRa.protocolParameters.receiveDelay1)
#define PDS_MAC_RX_DELAY_2_SIZE					sizeof(loRa.protocolParameters.receiveDelay2)
#define PDS_MAC_JOIN_ACCEPT_DELAY_1_SIZE		sizeof(loRa.protocolParameters.joinAcceptDelay1)
#define PDS_MAC_JOIN_ACCEPT_DELAY_2_SIZE		sizeof(loRa.protocolParameters.joinAcceptDelay2)
#define PDS_MAC_RETRANSMIT_TIMEOUT_SIZE			sizeof(loRa.protocolParameters.retransmitTimeout)
#define PDS_MAC_ADR_ACK_LIMIT_SIZE				sizeof(loRa.protocolParameters.adrAckLimit)
#define PDS_MAC_ADR_ACK_DELAY_SIZE				sizeof(loRa.protocolParameters.adrAckDelay)
#define PDS_MAC_MAX_REP_UNCNF_UPLINK_SIZE		sizeof(loRa.maxRepetitionsUnconfirmedUplink)
#define PDS_MAC_MAX_REP_CNF_UPLINK_SIZE 		sizeof(loRa.maxRepetitionsConfirmedUplink)
#define PDS_MAC_RX2_PARAMS_SIZE					sizeof(loRa.receiveWindow2Parameters)
#define PDS_MAC_RX1_OFFSET_SIZE					sizeof(loRa.offset)
#define PDS_MAC_ACTIVATION_TYPE_SIZE			sizeof(loRa.activationParameters.activationType)
#define PDS_MAC_DEV_ADDR_SIZE					sizeof(loRa.activationParameters.deviceAddress)
#define PDS_MAC_NWK_SKEY_SIZE					sizeof(loRa.activationParameters.networkSessionKeyRom)
#define PDS_MAC_APP_SKEY_SIZE					sizeof(loRa.activationParameters.applicationSessionKeyRom)
#define PDS_MAC_APP_KEY_SIZE					sizeof(loRa.activationParameters.applicationKey)
#define PDS_MAC_JOIN_EUI_SIZE					sizeof(loRa.activationParameters.joinEui)
#define PDS_MAC_DEV_EUI_SIZE					sizeof(loRa.activationParameters.deviceEui)
#define PDS_MAC_LORAWAN_MAC_KEYS_SIZE			sizeof(loRa.macKeys)
#define PDS_MAC_FCNT_UP_SIZE					sizeof(loRa.fCntUp.value)
#define PDS_MAC_DEV_NONCE_SIZE					sizeof(loRa.devNonce)
#define PDS_MAC_FCNT_DOWN_SIZE					sizeof(loRa.fCntDown.value)
#define PDS_MAC_LORAWAN_STATUS_SIZE				sizeof(loRa.macStatus.value)
#define PDS_MAC_CURR_DR_SIZE					sizeof(loRa.currentDataRate)
#define PDS_MAC_MAX_FCNT_INC_SIZE               sizeof(loRa.maxFcntPdsUpdateValue)
#define PDS_MAC_CRYPTO_DEV_ENABLED_SIZE			sizeof(loRa.cryptoDeviceEnabled)
#define PDS_MAC_JOIN_NONCE_SIZE                 sizeof(loRa.joinNonce)
#define PDS_MAC_RXC_PARAMS_SIZE					sizeof(loRa.receiveWindowCParameters)

/* PDS MAC Items offset*/

/* Offset in PDS_FILE_MAC_01_IDX */

#define PDS_MAC_ISM_BAND_OFFSET				(PDS_FILE_START_OFFSET)
#define PDS_MAC_ED_CLASS_OFFSET				(PDS_MAC_ISM_BAND_OFFSET             + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_ISM_BAND_SIZE)
#define PDS_MAC_PERIOD_FOR_LINK_CHK_OFFSET  (PDS_MAC_ED_CLASS_OFFSET             + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_ED_CLASS_SIZE)
#define PDS_MAC_SYNC_WORD_OFFSET 			(PDS_MAC_PERIOD_FOR_LINK_CHK_OFFSET  + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_PERIOD_FOR_LINK_CHK_SIZE)
#define PDS_MAC_EVENT_MASK_OFFSET 			(PDS_MAC_SYNC_WORD_OFFSET            + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_SYNC_WORD_SIZE)
#define PDS_MAC_MCAST_FCNT_DWN_OFFSET 		(PDS_MAC_EVENT_MASK_OFFSET           + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_EVENT_MASK_SIZE)
#define PDS_MAC_MCAST_DEV_ADDR_OFFSET 		(PDS_MAC_MCAST_FCNT_DWN_OFFSET       + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_FCNT_DWN_SIZE)
#define PDS_MAC_MCAST_APP_SKEY_OFFSET 		(PDS_MAC_MCAST_DEV_ADDR_OFFSET       + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_DEV_ADDR_SIZE)			
#define PDS_MAC_MCAST_NWK_SKEY_OFFSET 		(PDS_MAC_MCAST_APP_SKEY_OFFSET       + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_APP_SKEY_SIZE)
#define PDS_MAC_MCAST_KEYS_OFFSET					(PDS_MAC_MCAST_NWK_SKEY_OFFSET				+ PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_NWK_SKEY_SIZE)
#define PDS_MAC_MCAST_GROUP_MASK_OFFSET				(PDS_MAC_MCAST_KEYS_OFFSET					+ PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_KEYS_SIZE)
#define PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR_OFFSET	(PDS_MAC_MCAST_GROUP_MASK_OFFSET			+ PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_GROUP_MASK_SIZE)
#define PDS_MAC_LBT_PARAMS_OFFSET 					(PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR_OFFSET  + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR_SIZE)
#define PDS_MAC_TX_POWER_OFFSET 			(PDS_MAC_LBT_PARAMS_OFFSET           + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_LBT_PARAMS_SIZE)
#define PDS_MAC_PRESCALR_OFFSET 			(PDS_MAC_TX_POWER_OFFSET             + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_TX_POWER_SIZE)
#define PDS_MAC_RX_DELAY_1_OFFSET 			(PDS_MAC_PRESCALR_OFFSET             + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_PRESCALR_SIZE)
#define PDS_MAC_RX_DELAY_2_OFFSET 			(PDS_MAC_RX_DELAY_1_OFFSET           + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RX_DELAY_1_SIZE)
#define PDS_MAC_JOIN_ACCEPT_DELAY_1_OFFSET 	(PDS_MAC_RX_DELAY_2_OFFSET           + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RX_DELAY_2_SIZE)
#define PDS_MAC_JOIN_ACCEPT_DELAY_2_OFFSET 	(PDS_MAC_JOIN_ACCEPT_DELAY_1_OFFSET  + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_JOIN_ACCEPT_DELAY_1_SIZE)
#define PDS_MAC_RETRANSMIT_TIMEOUT_OFFSET 			(PDS_MAC_JOIN_ACCEPT_DELAY_2_OFFSET         + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_JOIN_ACCEPT_DELAY_2_SIZE)
#define PDS_MAC_ADR_ACK_LIMIT_OFFSET 		(PDS_MAC_RETRANSMIT_TIMEOUT_OFFSET          + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RETRANSMIT_TIMEOUT_SIZE)
#define PDS_MAC_ADR_ACK_DELAY_OFFSET 		(PDS_MAC_ADR_ACK_LIMIT_OFFSET        + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_ADR_ACK_LIMIT_SIZE)
#define PDS_MAC_MAX_REP_UNCNF_UPLINK_OFFSET (PDS_MAC_ADR_ACK_DELAY_OFFSET        + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_ADR_ACK_DELAY_SIZE)
#define PDS_MAC_MAX_REP_CNF_UPLINK_OFFSET   (PDS_MAC_MAX_REP_UNCNF_UPLINK_OFFSET   + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MAX_REP_UNCNF_UPLINK_SIZE)
#define PDS_MAC_FCNT_UP_OFFSET				(PDS_MAC_MAX_REP_CNF_UPLINK_OFFSET + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MAX_REP_CNF_UPLINK_SIZE)
#define PDS_MAC_DEV_NONCE_OFFSET				(PDS_MAC_FCNT_UP_OFFSET + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_FCNT_UP_SIZE)


/* Offset in PDS_FILE_MAC_02_IDX */
#define PDS_MAC_RX2_PARAMS_OFFSET 			(PDS_FILE_START_OFFSET)
#define PDS_MAC_RX1_OFFSET_OFFSET 			(PDS_MAC_RX2_PARAMS_OFFSET       + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RX2_PARAMS_SIZE)
#define PDS_MAC_ACTIVATION_TYPE_OFFSET 		(PDS_MAC_RX1_OFFSET_OFFSET       + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RX1_OFFSET_SIZE)
#define PDS_MAC_DEV_ADDR_OFFSET 			(PDS_MAC_ACTIVATION_TYPE_OFFSET  + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_ACTIVATION_TYPE_SIZE)
#define PDS_MAC_NWK_SKEY_OFFSET 			(PDS_MAC_DEV_ADDR_OFFSET         + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_DEV_ADDR_SIZE)
#define PDS_MAC_APP_SKEY_OFFSET 			(PDS_MAC_NWK_SKEY_OFFSET         + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_NWK_SKEY_SIZE)
#define PDS_MAC_APP_KEY_OFFSET 				(PDS_MAC_APP_SKEY_OFFSET         + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_APP_SKEY_SIZE)
#define PDS_MAC_APP_EUI_OFFSET 				(PDS_MAC_APP_KEY_OFFSET          + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_APP_KEY_SIZE)
#define PDS_MAC_DEV_EUI_OFFSET 				(PDS_MAC_APP_EUI_OFFSET          + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_JOIN_EUI_SIZE)
#define PDS_MAC_LORAWAN_MAC_KEYS_OFFSET 	(PDS_MAC_DEV_EUI_OFFSET          + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_DEV_EUI_SIZE)
#define PDS_MAC_FCNT_DOWN_OFFSET			(PDS_MAC_LORAWAN_MAC_KEYS_OFFSET + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_LORAWAN_MAC_KEYS_SIZE)
#define PDS_MAC_LORAWAN_STATUS_OFFSET		(PDS_MAC_FCNT_DOWN_OFFSET	     + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_FCNT_DOWN_SIZE)
#define PDS_MAC_CURR_DR_OFFSET				(PDS_MAC_LORAWAN_STATUS_OFFSET   + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_LORAWAN_STATUS_SIZE)
#define PDS_MAC_MAX_FCNT_INC_OFFSET         (PDS_MAC_CURR_DR_OFFSET          + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_CURR_DR_SIZE)
#define PDS_MAC_CRYPTO_DEV_ENABLED_OFFSET   (PDS_MAC_MAX_FCNT_INC_OFFSET	 + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_MAX_FCNT_INC_SIZE)
#define PDS_MAC_JOIN_NONCE_OFFSET           (PDS_MAC_CRYPTO_DEV_ENABLED_OFFSET + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_CRYPTO_DEV_ENABLED_SIZE)
#define PDS_MAC_RXC_PARAMS_OFFSET			(PDS_MAC_JOIN_NONCE_OFFSET		 + PDS_SIZE_OF_ITEM_HDR + PDS_MAC_RXC_PARAMS_SIZE)

void Lorawan_Pds_fid1_CB(void);
void Lorawan_Pds_fid2_CB(void);

#ifdef	__cplusplus
}
#endif

#endif	/* MLS_LORA_PDS_H */

/**
 End of File
*/