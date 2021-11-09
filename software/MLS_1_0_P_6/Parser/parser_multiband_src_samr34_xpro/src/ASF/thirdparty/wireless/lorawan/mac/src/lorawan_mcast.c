/**
* \file  lorawan_mcast.c
*
* \brief LoRaWAN file for generic multicast handling
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
#include "conf_stack.h"
#include "lorawan.h"
#include "lorawan_defs.h"
#include "lorawan_private.h"
#include "lorawan_mcast.h"
#include "lorawan_reg_params.h"
#include "system_assert.h"
#include "pds_interface.h"

/******************* EXTERN DEFINITIONS *************************************/
extern LoRa_t loRa;

#include "lorawan_pds.h"

extern uint8_t radioBuffer[];

/******************* CONSTANT DEFINITIONS *************************************/


/****************************** VARIABLES *************************************/


/*********************** FUNCTION DEFINITIONS *********************************/

/*********************************************************************//**
\brief	Multicast - initialization of variables and states
*************************************************************************/
void LorawanMcastInit(void)
{
#if (FEATURE_DL_MCAST == 1)
    loRa.mcastParams.numSupportedMcastGroups = 0;
	loRa.mcastParams.mcastGroupMask = 0;
	for(uint8_t i = 0; i < LORAWAN_MCAST_GROUP_COUNT_SUPPORTED; i++)
	{
		loRa.mcastParams.activationParams[i].mcastDevAddr.value = LORAWAN_MCAST_DEVADDR_DEFAULT;
		memset(&loRa.mcastParams.activationParams[i].mcastAppSKey, 0, LORAWAN_SESSIONKEY_LENGTH);
		memset(&loRa.mcastParams.activationParams[i].mcastNwkSKey, 0, LORAWAN_SESSIONKEY_LENGTH);
		loRa.mcastParams.activationParams[i].datarate = loRa.receiveWindow2Parameters.dataRate;
		loRa.mcastParams.activationParams[i].dlFrequency = loRa.receiveWindow2Parameters.frequency;
		loRa.mcastParams.activationParams[i].mcastFCntDownMin.value = 0;
        loRa.mcastParams.activationParams[i].mcastFCntDownMax.value = 0;
        loRa.mcastParams.activationParams[i].mcastFCntDown.value = 0;
	}
	   loRa.receiveWindowCParameters.dataRate = loRa.receiveWindow2Parameters.dataRate;
	   loRa.receiveWindowCParameters.frequency = loRa.receiveWindow2Parameters.frequency;
#endif /* #if (FEATURE_DL_MCAST == 1) */
}

/*********************************************************************//**
\brief	Multicast enable/disable configuration function
\param[in]  enable - notifies whether to enable or disable multicast
\return	LORAWAN_SUCCESS, if successfully enabled
        LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastEnable(bool enable, uint8_t groupId)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

#if (FEATURE_DL_MCAST == 1)

	if(LORAWAN_MCAST_GROUP_COUNT_SUPPORTED > groupId)
	{
		if (true == enable)
		{
			if ((true == loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastApplicationSessionKey) &&
			(true == loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastNetworkSessionKey) &&
			(true == loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastDeviceAddress) &&
			((CLASS_A | CLASS_B | CLASS_C) & loRa.edClass))
			{
				loRa.mcastParams.mcastGroupMask |= 0x01 << groupId;
				PDS_STORE(PDS_MAC_MCAST_GROUP_MASK);

				status = LORAWAN_SUCCESS;
			}
			loRa.mcastParams.numSupportedMcastGroups++;
			PDS_STORE(PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR);
		}
		else
		{
			loRa.mcastParams.mcastGroupMask &= ~(0x1 << groupId);
			PDS_STORE(PDS_MAC_MCAST_GROUP_MASK);

			status = LORAWAN_SUCCESS;
			//if the value is not zero
			loRa.mcastParams.numSupportedMcastGroups--;
			PDS_STORE(PDS_MAC_MCAST_SUPPORTED_GROUP_CNTR);

		}
	}
#endif /* #if (FEATURE_DL_MCAST == 1) */
    return status;
}

/*********************************************************************//**
\brief	Check if the incoming packet is a multicast group the device
        supports
\param[in]  hdr - mac header of the received frame
\param[in]  mType - message type of the received frame
\param[in]  fPort - frame port of the received frame
\return	    LORAWAN_SUCCESS, if successfully validated
            LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastValidateHdr(Hdr_t *hdr, uint8_t mType, uint8_t fPort,uint8_t *groupId)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;

#if (FEATURE_DL_MCAST == 1)
    /*
        if (hdr addr == registered mcast group addr)
            if ((fport == 0) OR
                (foptslen != 0) OR
                (ACK != 0) OR
                (ADRAckReq != 0) OR
                (MType != UNCNF Down))
                fail
            else
                goto next step
        else
            fail
    */
	for(uint8_t i = 0; i < LORAWAN_MCAST_GROUP_COUNT_SUPPORTED; i++)
	{
		/* Check whether the MCAST group is enabled.
		 * if not continue
		 */
		if(0 == (loRa.mcastParams.mcastGroupMask & (0x01 << i)))
		{
			continue;
		}
		/* If MCAST is enable, compare the Dev address*/
		if ((hdr->members.devAddr.value == loRa.mcastParams.activationParams[i].mcastDevAddr.value) &&
			((CLASS_B | CLASS_C) & loRa.edClass)) //check for ED is either Class C or B
		{
			/*Fport Should not be Zero
			 Fopts length should be Zero
			 The ACK and ADRACKReq bits must be zero
			 The MType field must carry the value for Unconfirmed Data Down.*/
			  
			if (!
				((fPort == 0) ||
				(hdr->members.fCtrl.fOptsLen != 0) ||
				(hdr->members.fCtrl.ack != 0) ||
				(hdr->members.fCtrl.adrAckReq != 0) ||
				(mType != FRAME_TYPE_DATA_UNCONFIRMED_DOWN)))
			{
				status = LORAWAN_SUCCESS;
				*groupId = i;
			}
		}
	}
#else /* #if (FEATURE_DL_MCAST == 1) */
    status = LORAWAN_INVALID_PARAMETER;
#endif /* #if (FEATURE_DL_MCAST == 1) */

    return status;
}

/*********************************************************************//**
\brief	Decrypt the packet and provide callback to application
\param[in,out]  buffer  - placeholder for input and output data
\param[in]      bufferLength - length of the received packet
\param[in]      hdr - mac header of the received packet
\return	LORAWAN_SUCCESS, if successfully processed
        LORAWAN_INVALID_PARAMETER, otherwise
*************************************************************************/
StackRetStatus_t LorawanMcastProcessPkt(uint8_t* buffer, uint8_t bufferLength, Hdr_t *hdr,uint8_t groupId)
{
    StackRetStatus_t status = LORAWAN_INVALID_PARAMETER;
	SalStatus_t sal_status = SAL_SUCCESS;
#if (FEATURE_DL_MCAST == 1)
    uint8_t frmPayloadLength;
    uint8_t bufferIndex;
    uint8_t *packet;
    uint32_t extractedMic;
    uint8_t fPort;
    LorawanMcastActivationParams_t *group = & loRa.mcastParams.activationParams[groupId];
    bool canProcessMcastPacket = false;

    /* 8 for the header and 1 for fport*/
    buffer += (LORAWAN_FHDR_SIZE_WITHOUT_FOPTS + sizeof(fPort));
    frmPayloadLength = bufferLength - LORAWAN_FHDR_SIZE_WITHOUT_FOPTS - sizeof (extractedMic); //frmPayloadLength includes port

    bufferIndex = 16 + LORAWAN_FHDR_SIZE_WITHOUT_FOPTS + sizeof (fPort);

    if (group->mcastFCntDownMin.value < group->mcastFCntDownMax.value)
    {
        /* there is no wraparound of counter i.e., min <= cur < max */
        canProcessMcastPacket = (group->mcastFCntDownMin.value <= group->mcastFCntDown.value);
        canProcessMcastPacket = canProcessMcastPacket && (group->mcastFCntDown.value < group->mcastFCntDownMax.value);
    }
    else /* counter will wraparound eventually */
    {
        /* if following is true then counter has not wrapped around yet */
        canProcessMcastPacket = (group->mcastFCntDownMin.value <= group->mcastFCntDown.value);

        if (false == canProcessMcastPacket) /* counter has wrapped around */
        {
            /* counter is still within the max value */
            canProcessMcastPacket = (group->mcastFCntDown.value < group->mcastFCntDownMax.value);
        }
    }
    
    if (canProcessMcastPacket)
    {
        group->mcastFCntDown.members.valueLow = hdr->members.fCnt;
		PDS_STORE(PDS_MAC_MCAST_FCNT_DWN);
        sal_status = EncryptFRMPayload (buffer, frmPayloadLength-1, 1, loRa.mcastParams.activationParams[groupId].mcastFCntDown.value, loRa.mcastParams.activationParams[groupId].mcastAppSKey, SAL_MCAST_APPS_KEY, bufferIndex, radioBuffer, loRa.mcastParams.activationParams[groupId].mcastDevAddr.value);
        if (SAL_SUCCESS != sal_status)
        {
	        /* Transaction complete Event */
	        UpdateTransactionCompleteCbParams(LORAWAN_RXPKT_ENCRYPTION_FAILED);
        }
		packet = buffer - 1;

        if (AppPayload.AppData != NULL)
        {
            loRa.lorawanMacStatus.syncronization = 0;
            /* Send RX Available Event */
            UpdateRxDataAvailableCbParams(hdr->members.devAddr.value, packet, frmPayloadLength, LORAWAN_SUCCESS);
        }

        status = LORAWAN_SUCCESS;
    }
    else
    {
		
    }

	if ((loRa.macStatus.macState == RX1_OPEN) && (loRa.edClass == CLASS_C))
	{   
		loRa.macStatus.macState = RX2_OPEN;
	}
	//Move to Receive state after packet reception
	loRa.enableRxcWindow = true;
	LorawanConfigureRadioForRX2(false);

#else /* #if (FEATURE_DL_MCAST == 1) */
    status = LORAWAN_INVALID_PARAMETER;
#endif /* #if (FEATURE_DL_MCAST == 1) */
    return status;
}

StackRetStatus_t LorawanAddMcastAddr(uint32_t mcast_devaddr, uint8_t groupId)
{
	StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * Passing uni-cast device address for multi-cast group address
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
		(mcast_devaddr == loRa.activationParameters.deviceAddress.value)
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
	else
	{
		loRa.mcastParams.activationParams[groupId].mcastDevAddr.value = mcast_devaddr;
		PDS_STORE(PDS_MAC_MCAST_DEV_ADDR);
		loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastDeviceAddress = 1;
		PDS_STORE(PDS_MAC_MCAST_KEYS);
		result = LORAWAN_SUCCESS;			
	}
	return result;
}

StackRetStatus_t LorawanAddMcastAppskey(uint8_t * appSkey, uint8_t groupId)
{
	StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * Passing (void *)0 i.e., NULL for appSkey pointer
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
		(NULL == appSkey)
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
	else
	{
		memcpy(&loRa.mcastParams.activationParams[groupId].mcastAppSKey, appSkey, LORAWAN_SESSIONKEY_LENGTH);
		PDS_STORE(PDS_MAC_MCAST_APP_SKEY);
		loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastApplicationSessionKey = 1;
		PDS_STORE(PDS_MAC_MCAST_KEYS);
		result = LORAWAN_SUCCESS;
	}
	return result;
}

StackRetStatus_t LorawanAddMcastNwkskey(uint8_t * nwkSkey, uint8_t groupId)
{
	StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * Passing (void *)0 i.e., NULL for nwkSkey pointer
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
		(NULL == nwkSkey)
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
	else
	{
		memcpy(&loRa.mcastParams.activationParams[groupId].mcastNwkSKey, nwkSkey, LORAWAN_SESSIONKEY_LENGTH);
		PDS_STORE(PDS_MAC_MCAST_NWK_SKEY);
		loRa.mcastParams.activationParams[groupId].mcastKeysMask.mcastNetworkSessionKey = 1;
		PDS_STORE(PDS_MAC_MCAST_KEYS);
		result = LORAWAN_SUCCESS;
	}
	return result;
}

StackRetStatus_t LorawanAddMcastFcntMin(uint32_t cnt, uint8_t groupId)
{
    StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    */
	if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
    else
    {        
        loRa.mcastParams.activationParams[groupId].mcastFCntDownMin.value = cnt;
        loRa.mcastParams.activationParams[groupId].mcastFCntDown.value = cnt;
        result = LORAWAN_SUCCESS;
    }
    return result;
}

StackRetStatus_t LorawanAddMcastFcntMax(uint32_t cnt, uint8_t groupId)
{
    StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    */
	if( LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
    else
    {
        loRa.mcastParams.activationParams[groupId].mcastFCntDownMax.value = cnt;
        result = LORAWAN_SUCCESS;
    }
    return result;
}

StackRetStatus_t LorawanAddMcastDlFrequency(uint32_t dlFreq, uint8_t groupId)
{
    StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * \dlFreq IS NOT a valid frequency for the current regional setting
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
        (LORAWAN_SUCCESS != LORAREG_ValidateAttr(RX_FREQUENCY, &dlFreq))
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
    else
    {
        loRa.mcastParams.activationParams[groupId].dlFrequency = dlFreq;
		loRa.receiveWindowCParameters.frequency = dlFreq;
		PDS_STORE(PDS_MAC_RXC_PARAMS); 
        result = LORAWAN_SUCCESS;
    }
    return result;
}

StackRetStatus_t LorawanAddMcastDatarate(uint8_t dr, uint8_t groupId)
{
    StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * \dr IS NOT a valid data rate for the current regional setting
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
        (LORAWAN_SUCCESS != LORAREG_ValidateAttr(RX_DATARATE, &dr))
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
    else
    {
        loRa.mcastParams.activationParams[groupId].datarate = dr;
		loRa.receiveWindowCParameters.dataRate = dr;
		PDS_STORE(PDS_MAC_RXC_PARAMS); 
        result = LORAWAN_SUCCESS;
    }
    return result;
}

StackRetStatus_t LorawanAddMcastPeriodicity(uint8_t periodicity, uint8_t groupId)
{
    StackRetStatus_t result;
    /*
    * Invalid case:
    * -------------
    * Passing groupId greater than \LORAWAN_MCAST_GROUP_COUNT_SUPPORTED
    * \periodicity IS NOT a valid periodicity value
    */
	if( (LORAWAN_MCAST_GROUP_COUNT_SUPPORTED <= groupId) || \
        (periodicity > 7)
	  )
	{
		result = LORAWAN_INVALID_PARAMETER;
	}
    else
    {
        loRa.mcastParams.activationParams[groupId].periodicity = periodicity;
        result = LORAWAN_SUCCESS;
    }
    return result;
}
