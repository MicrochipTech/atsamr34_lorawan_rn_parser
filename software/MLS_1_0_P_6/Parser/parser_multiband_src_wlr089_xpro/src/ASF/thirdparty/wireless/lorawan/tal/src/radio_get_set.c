/**
* \file  radio_get_set.c
*
* \brief This is the radio_get_set source file which contains implimentations for
*        Getting and Setting the Radio and Transciver parameters.
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

/************************************************************************/
/*  Includes                                                            */
/************************************************************************/
#include "radio_interface.h"
#include "radio_driver_SX1276.h"
#include "radio_registers_SX1276.h"
#include "radio_transaction.h"
#include "radio_driver_hal.h"
#include "radio_get_set.h"
#include "sw_timer.h"
#include "stdint.h"
#include "string.h"

/************************************************************************/
/*  Global variables                                                    */
/************************************************************************/

/************************************************************************/
/*  Prototypes															*/
/************************************************************************/

/*********************************************************************//**
\brief	This function sets FSK frequency deviation in FSK mode.

\param frequencyDeviation	- Sets the transmit radio frequency deviation.
\return						- none.
*************************************************************************/
static void Radio_WriteFSKFrequencyDeviation(uint32_t frequencyDeviation);

/*********************************************************************//**
\brief	This function sets FSK bitrate in FSK mode.

\param bitRate	- Sets the bitrate to be set.
\return			- none.
*************************************************************************/
static void Radio_WriteFSKBitRate(uint32_t bitRate);

/*********************************************************************//**
\brief	This function sets transmit power.

\param power	- Sets the power to be set.
\return			- none.
*************************************************************************/
static void Radio_WritePower(int8_t power);

/************************************************************************/
/* Implementations                                                      */
/************************************************************************/

/*********************************************************************//**
\brief This function gets the radio attribute.

\param attribute - The attribute to get.
\param value     - The pointer that is used to get the value.
\return          - The error condition for getting a given attribute.
*************************************************************************/
RadioError_t RADIO_GetAttr(RadioAttribute_t attribute, void *value)
{
	switch (attribute)
	{
		case RADIO_CALLBACK:
		{
			value = (void *)radioConfiguration.radioCallback;
		}
			break;
		case LORA_SYNC_WORD:
		{
			*(uint8_t *)value = radioConfiguration.syncWordLoRa;
		}
		break;
		case CHANNEL_FREQUENCY:
		{
			*(uint32_t *)value = radioConfiguration.frequency;
		}
		break;
		case CHANNEL_FREQUENCY_DEVIATION:
		{
			*(uint32_t *)value = radioConfiguration.frequencyDeviation;
		}
		break;
		case PREAMBLE_LEN:
		{
			*(uint16_t *)value = radioConfiguration.preambleLen;
		}
		break;
		case OUTPUT_POWER:
		{
			*(uint8_t *)value = radioConfiguration.outputPower;
			
		}
		break;
		case CRC_ON:
		{
			*(uint8_t *)value = radioConfiguration.crcOn;
		}
		break;
		case IQINVERTED:
		{
			*(uint8_t *)value = radioConfiguration.iqInverted;
		}
		break;
		case BANDWIDTH:
		{
			*(RadioLoRaBandWidth_t *)value = radioConfiguration.bandWidth;
		}
		break;
		case PABOOST:
		{
			*(uint8_t *)value = radioConfiguration.paBoost;
		}
		break;
		case MODULATION:
		{
			*(RadioModulation_t *)value = radioConfiguration.modulation;
		}
		break;
		case FREQUENCY_HOP_PERIOD:
		{
			*(uint16_t *)value = radioConfiguration.frequencyHopPeriod;
		}
		break;
		case ERROR_CODING_RATE:
		{
			*(RadioErrorCodingRate_t *)value = radioConfiguration.errorCodingRate;
		}
		break;
		case WATCHDOG_TIMEOUT:
		{
			*(uint32_t *)value = radioConfiguration.watchdogTimerTimeout;
		}
		break;
		case FSK_BIT_RATE:
		{
			*(uint32_t *)value = radioConfiguration.bitRate;
		}
		break;
		case FSK_DATA_SHAPING:
		{
			*(RadioFSKShaping_t *)value = radioConfiguration.fskDataShaping;
		}
		break;
		case FSK_RX_BW:
		{
			*(RadioFSKBandWidth_t *)value = radioConfiguration.rxBw;
		}
		break;
		case FSK_AFC_BW:
		{
			*(RadioFSKBandWidth_t *)value = radioConfiguration.afcBw;
		}
		break;
		case FSK_SYNC_WORD:
		{
			memcpy((uint8_t *)value, radioConfiguration.syncWord, radioConfiguration.syncWordLen);
		}
		break;
		case FSK_SYNC_WORD_LEN:
		{
			*(uint8_t *)value = radioConfiguration.syncWordLen;
		}
		break;
		case PACKET_SNR:
		{
			*(int8_t *)value = radioConfiguration.packetSNR;
		}
		break;
		case SPREADING_FACTOR:
		{
			*(RadioDataRate_t *)value = radioConfiguration.dataRate;
		}
		break;
/*#ifdef LBT*/
		case RADIO_LBT_PARAMS:
		{
			*(RadioLBTParams_t *)value = radioConfiguration.lbt.params;
		}
		break;
/*#endif*/ // LBT
		case RADIO_CLOCK_STABLE_DELAY:
		{
			*(uint8_t *)value = radioConfiguration.clockStabilizationDelay;
		}
		break;
		
 	    case PACKET_RSSI_VALUE:
 		{
	 		*(int16_t *)value = radioConfiguration.packetRSSI;
 	    }
		break;
		default:
		{
		//UNKOWN ATTRIBUTE
		return ERR_OUT_OF_RANGE;
		}
	}

	return ERR_NONE;
}

/*********************************************************************//**
\brief This function sets the radio attribute.

\param attribute - The attribute to set.
\param value     - The pointer that is used to set the value.
\return          - The error condition for setting a given attribute.
*************************************************************************/
RadioError_t RADIO_SetAttr(RadioAttribute_t attribute, void *value)
{
	if (!(RADIO_STATE_IDLE == RADIO_GetState()))
	{
		return ERR_RADIO_BUSY;
	}

	switch (attribute)
	{
		case RADIO_CALLBACK:
		{
			if (value)
			{
				radioConfiguration.radioCallback = (RadioCallback_t)value;
			}
			else
			{
				return ERR_INVALID_REQ;
			}
		}
		break;
/*#ifdef LBT*/
		case RADIO_LBT_PARAMS:
		{
			RadioLBTParams_t params = *(RadioLBTParams_t *)value;
			RadioLBTParams_t local;
			if (false == params.lbtTransmitOn)
			{
				local.lbtNumOfSamples = 0;
				local.lbtScanPeriod = 0;
				local.lbtThreshold = 0;
				local.lbtTransmitOn = params.lbtTransmitOn;
				radioConfiguration.lbt.lbtScanTimeout = 0;
				radioConfiguration.lbt.params = local;
			}
			else // (true == params.lbtTransmitOn)
			{
				if (0 == params.lbtScanPeriod)
				{
					return ERR_INVALID_REQ;
				}
				else if (0 == params.lbtNumOfSamples)
				{
					return ERR_INVALID_REQ;
				}
				else
				{
					float localScanTimeout = (MS_TO_US((params.lbtScanPeriod))/params.lbtNumOfSamples);
					// calculation for 500us, Since the localScanTimeout is in uS, we're comparing directly with value 500
					/*if ((500) > localScanTimeout)
					{
						return ERR_INVALID_REQ;
					}*/
					radioConfiguration.lbt.lbtScanTimeout = (uint32_t)localScanTimeout;
					radioConfiguration.lbt.params = params;
				}
			}
		}
		break;
/*#endif*/ // LBT
		case LORA_SYNC_WORD:
		{
			radioConfiguration.syncWordLoRa = *(uint8_t *)value;
		}
		break;
		case CHANNEL_FREQUENCY:
		{
			uint32_t freq;

			freq = *(uint32_t *)value;

			if ( ((freq >= FREQ_137000KHZ) && (freq <= FREQ_175000KHZ)) ||
			((freq >= FREQ_410000KHZ) && (freq <= FREQ_525000KHZ)) ||
			((freq >= FREQ_862000KHZ) && (freq <= FREQ_1020000KHZ)) )
			{
				radioConfiguration.frequency = freq;
			}
			else
			{
				return ERR_OUT_OF_RANGE;
			}

		}
		break;
		case CHANNEL_FREQUENCY_DEVIATION:
		{
			radioConfiguration.frequencyDeviation = *(uint32_t *)value;
		}
		break;
		case PREAMBLE_LEN:
		{
			radioConfiguration.preambleLen = *(uint16_t *)value;
		}
		break;
		case OUTPUT_POWER:
		{
			radioConfiguration.outputPower = *(uint8_t *)value;
		}
		break;
		case CRC_ON:
		{
			uint8_t crcOn;
			crcOn = *(uint8_t *)value;
			if((crcOn != ENABLED) && (crcOn != DISABLED))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.crcOn = crcOn;
		}
		break;
		case IQINVERTED:
		{
			uint8_t iqInv;
			iqInv = *(uint8_t *)value;
			if((iqInv != ENABLED) && (iqInv != DISABLED))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.iqInverted =  iqInv;
		}
		break;
		case BANDWIDTH:
		{
			RadioLoRaBandWidth_t bw;
			bw = *(RadioLoRaBandWidth_t *)value;
			if((bw < BW_125KHZ) || (bw > BW_500KHZ))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.bandWidth =  bw;
		}
		break;
		case PABOOST:
		{
			radioConfiguration.paBoost = *(uint8_t *)value;
		}
		break;
		case MODULATION:
		{
			RadioModulation_t mod;
			mod =  *(RadioModulation_t *)value;
			if((mod > MODULATION_LORA))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.modulation = mod;
		}
		break;
		case FREQUENCY_HOP_PERIOD:
		{
			radioConfiguration.frequencyHopPeriod = *(uint16_t *)value;
		}
		break;
		case ERROR_CODING_RATE:
		{
			RadioErrorCodingRate_t ecr;
			ecr =  *(RadioErrorCodingRate_t *)value;
			if((ecr < CR_4_5) || (ecr > CR_4_8))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.errorCodingRate = ecr;
		}
		break;
		case WATCHDOG_TIMEOUT:
		{
			radioConfiguration.watchdogTimerTimeout = *(uint32_t *)value;
		}
		case FSK_BIT_RATE:
		{
			radioConfiguration.bitRate = *(uint32_t *)value;
		}
		break;
		case FSK_DATA_SHAPING:
		{
			RadioFSKShaping_t dataShaping;
			dataShaping =  *(RadioFSKShaping_t *)value;
			if((dataShaping > FSK_SHAPING_GAUSS_BT_0_3))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.fskDataShaping = dataShaping;
		}
		break;
		case FSK_RX_BW:
		{
			RadioFSKBandWidth_t bw;
			bw = *(RadioFSKBandWidth_t *)value;
			if((bw < FSKBW_250_0KHZ)||(bw > FSKBW_2_6KHZ))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.rxBw = bw;
		}
		break;
		case FSK_AFC_BW:
		{
			RadioFSKBandWidth_t bw;
			bw = *(RadioFSKBandWidth_t *)value;
			if((bw < FSKBW_250_0KHZ)||(bw > FSKBW_2_6KHZ))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.afcBw = bw;
		}
		break;
		case FSK_SYNC_WORD:
		{
			memcpy(radioConfiguration.syncWord,(uint8_t *)value, radioConfiguration.syncWordLen);
		}
		break;
		case FSK_SYNC_WORD_LEN:
		{
			uint8_t syncWordLen;
			syncWordLen = *(uint8_t *)value;

			if (syncWordLen > 8)
			{
				syncWordLen = 8;
			}

			radioConfiguration.syncWordLen = syncWordLen;
		}
		break;
		case SPREADING_FACTOR:
		{
			RadioDataRate_t sf;
			sf = *(RadioDataRate_t *)value;
			if((sf < SF_7)||(sf > SF_12))
			{
				return ERR_OUT_OF_RANGE;
			}
			radioConfiguration.dataRate = sf;
		}
		break;
		default:
		{
			//Unknown Attribute
			return ERR_OUT_OF_RANGE;
		}
	}

	return ERR_NONE;
}

/*********************************************************************//**
\brief The Radio Init initializes the transceiver
*************************************************************************/
void RADIO_Init(void)
{
    RADIO_InitDefaultAttributes();
    RADIO_SetCallbackBitmask(RADIO_DEFAULT_CALLBACK_MASK);

    HAL_RegisterDioInterruptHandler(DIO0, RADIO_DIO0);
    HAL_RegisterDioInterruptHandler(DIO1, RADIO_DIO1);
    HAL_RegisterDioInterruptHandler(DIO2, RADIO_DIO2);
    HAL_RegisterDioInterruptHandler(DIO3, RADIO_DIO3);
    HAL_RegisterDioInterruptHandler(DIO4, RADIO_DIO4);
	HAL_RegisterDioInterruptHandler(DIO5, RADIO_DIO5);
	
}

/*********************************************************************//**
\brief	This function sets the transmit frequency.

\param frequency	- Sets the transmit radio frequency.
\return				- none.
*************************************************************************/
void Radio_WriteFrequency(uint32_t frequency)
{
    uint32_t num, num_mod;
    // Frf = (Fxosc * num) / 2^19
    // We take advantage of the fact that 32MHz = 15625Hz * 2^11
    // This simplifies our formula to Frf = (15625Hz * num) / 2^8
    // Thus, num = (Frf * 2^8) / 15625Hz

    // First, do the division, since Frf * 2^8 does not fit in 32 bits
    num = frequency / 15625;
    num_mod = frequency % 15625;

    // Now do multiplication as well, both for the quotient as well as for
    // the remainder
    num <<= SHIFT8;
    num_mod <<= SHIFT8;

    // Try to correct for the remainder. After the multiplication we can still
    // recover some accuracy
    num_mod = num_mod / 15625;
    num += num_mod;

    // Now variable num holds the representation of the frequency that needs to
    // be loaded into the radio chip
    RADIO_RegisterWrite(REG_FRFMSB, (num >> SHIFT16) & 0xFF);
    RADIO_RegisterWrite(REG_FRFMID, (num >> SHIFT8) & 0xFF);
    RADIO_RegisterWrite(REG_FRFLSB, num & 0xFF);
}

/*********************************************************************//**
\brief	This function sets FSK frequency deviation in FSK mode.

\param frequencyDeviation	- Sets the transmit radio frequency deviation.
\return						- none.
*************************************************************************/
static void Radio_WriteFSKFrequencyDeviation(uint32_t frequencyDeviation)
{
    uint32_t num;

    // Fdev = (Fxosc * num) / 2^19
    // We take advantage of the fact that 32MHz = 15625Hz * 2^11
    // This simplifies our formula to Fdev = (15625Hz * num) / 2^8
    // Thus, num = (Fdev * 2^8) / 15625Hz

    num = frequencyDeviation;
    num <<= SHIFT8;     // Multiply by 2^8
    num /= 15625;       // divide by 15625

    // Now variable num holds the representation of the frequency deviation that
    // needs to be loaded into the radio chip
    RADIO_RegisterWrite(REG_FSK_FDEVMSB, (num >> SHIFT8) & 0xFF);
    RADIO_RegisterWrite(REG_FSK_FDEVLSB, num & 0xFF);
}

/*********************************************************************//**
\brief	This function sets FSK bitrate in FSK mode.

\param bitRate	- Sets the bitrate to be set.
\return			- none.
*************************************************************************/
static void Radio_WriteFSKBitRate(uint32_t bitRate)
{
    uint32_t num;

    num = 32000000;
    num /= bitRate;

    // Now variable num holds the representation of the bitrate that
    // needs to be loaded into the radio chip
    RADIO_RegisterWrite(REG_FSK_BITRATEMSB, (num >> SHIFT8));
    RADIO_RegisterWrite(REG_FSK_BITRATELSB, num & 0xFF);
    RADIO_RegisterWrite(REG_FSK_BITRATEFRAC, 0x00);
}

/*********************************************************************//**
\brief	This function sets transmit power.

\param power	- Sets the power to be set.
\return			- none.
*************************************************************************/
static void Radio_WritePower(int8_t power)
{
    uint8_t paDac;
    uint8_t ocp;

    if (radioConfiguration.paBoost == 0)
    {
        // RFO pin used for RF output
        if (power < -3)
        {
            power = -3;
        }
        if (power > 15)
        {
            power = 15;
        }

        paDac = RADIO_RegisterRead(REG_PADAC);
        paDac &= ~(0x07);
        paDac |= 0x04;
        RADIO_RegisterWrite(REG_PADAC, paDac);

        if (power < 0)
        {
            // MaxPower = 2
            // Pout = 10.8 + MaxPower*0.6 - 15 + OutPower
            // Pout = -3 + OutPower
            power += 3;
            RADIO_RegisterWrite(REG_PACONFIG, 0x20 | power);
        }
        else
        {
            // MaxPower = 7
            // Pout = 10.8 + MaxPower*0.6 - 15 + OutPower
            // Pout = OutPower
            RADIO_RegisterWrite(REG_PACONFIG, 0x70 | power);
        }
    }
    else
    {
        // PA_BOOST pin used for RF output

        // Lower limit
        if (power < 2)
        {
            power = 2;
        }

        // Upper limit
        if (power >= 20)
        {
            power = 20;
        }
        else if (power > 17)
        {
            power = 17;
        }

        ocp = RADIO_RegisterRead(REG_OCP);
        paDac = RADIO_RegisterRead(REG_PADAC);
        paDac &= ~(0x07);
        if (power == 20)
        {
            paDac |= 0x07;
            power = 15;
            ocp &= ~(0x20);
        }
        else
        {
            paDac |= 0x04;
            power -= 2;
            ocp |= 0x20;
        }

        RADIO_RegisterWrite(REG_PADAC, paDac);
        RADIO_RegisterWrite(REG_PACONFIG, 0x80 | power);
        RADIO_RegisterWrite(REG_OCP, ocp);
    }
}

/*********************************************************************//**
\brief	This function prepares the transceiver for transmit and receive
		according to modulation set.

\param symbolTimeout	- Sets the symbolTimeout parameter.
\return					- none.
*************************************************************************/
void Radio_WriteConfiguration(uint16_t symbolTimeout)
{
    uint32_t tempValue;
    uint8_t regValue;
    uint8_t i;

    // Load configuration from RadioConfiguration_t structure into radio
    Radio_WriteMode(MODE_SLEEP, radioConfiguration.modulation, 0);
    Radio_WriteFrequency(radioConfiguration.frequency);
    Radio_WritePower(radioConfiguration.outputPower);

    if (MODULATION_LORA == radioConfiguration.modulation)
    {
        RADIO_RegisterWrite(0x39, radioConfiguration.syncWordLoRa);

        RADIO_RegisterWrite(REG_LORA_MODEMCONFIG1,
                            (radioConfiguration.bandWidth << SHIFT4) |
                            (radioConfiguration.errorCodingRate << SHIFT1) |
                            (radioConfiguration.implicitHeaderMode & 0x01));

        RADIO_RegisterWrite(REG_LORA_MODEMCONFIG2,
                            (radioConfiguration.dataRate << SHIFT4) |
                            ((radioConfiguration.crcOn & 0x01) << SHIFT2) |
                            ((symbolTimeout & 0x0300) >> SHIFT8));


        // Handle frequency hopping, if necessary
        if (0 != radioConfiguration.frequencyHopPeriod)
        {
            tempValue = radioConfiguration.frequencyHopPeriod;
            // Multiply by BW/1000 (since period is in ms)
            switch (radioConfiguration.bandWidth)
            {
                case BW_125KHZ:
                    tempValue *= 125;
                    break;
                case BW_250KHZ:
                    tempValue *= 250;
                    break;
                case BW_500KHZ:
                    tempValue *= 500;
                    break;
                default:
                    // Disable frequency hopping
                    tempValue = 0;
                    break;
            }
            // Divide by 2^SF
            tempValue >>= radioConfiguration.dataRate;
        }
        else
        {
            tempValue = 0;
        }
        RADIO_RegisterWrite(REG_LORA_HOPPERIOD, (uint8_t) tempValue);

        RADIO_RegisterWrite(REG_LORA_SYMBTIMEOUTLSB, (symbolTimeout & 0xFF));

        // If the symbol time is > 16ms, LowDataRateOptimize needs to be set
        // This long symbol time only happens for SF12&BW125, SF12&BW250
        // and SF11&BW125 and the following if statement checks for these
        // conditions
		regValue = RADIO_RegisterRead(REG_LORA_MODEMCONFIG3);
        
        if (((SF_12 == radioConfiguration.dataRate) &&
			((BW_125KHZ == radioConfiguration.bandWidth) || (BW_250KHZ == radioConfiguration.bandWidth))
			) ||
			((SF_11 == radioConfiguration.dataRate) && (BW_125KHZ == radioConfiguration.bandWidth)))
        {
	        regValue |= 1 << SHIFT3;     // Set LowDataRateOptimize
        }
        else
        {
	        regValue &= ~(1 << SHIFT3);    // Clear LowDataRateOptimize
        }
		
        regValue |= 1 << SHIFT2;         // LNA gain set by internal AGC loop
        RADIO_RegisterWrite(REG_LORA_MODEMCONFIG3, regValue);

        regValue = RADIO_RegisterRead(REG_LORA_DETECTOPTIMIZE);
        regValue &= ~(0x07);        // Clear DetectOptimize bits
        regValue |= 0x03;           // Set value for SF7 - SF12
        RADIO_RegisterWrite(REG_LORA_DETECTOPTIMIZE, regValue);

        // Also set DetectionThreshold value for SF7 - SF12
        RADIO_RegisterWrite(REG_LORA_DETECTIONTHRESHOLD, 0x0A);

        // Errata settings to mitigate spurious reception of a LoRa Signal
        if (0x12 == radioConfiguration.regVersion)
        {
            // Chip already is in sleep mode. For these BWs we don't need to
            // offset Frf
            if ( (BW_125KHZ == radioConfiguration.bandWidth) ||
                (BW_250KHZ == radioConfiguration.bandWidth) )
            {
                regValue = RADIO_RegisterRead(0x31);
                regValue &= ~0x80;                                  // Clear bit 7
                RADIO_RegisterWrite(0x31, regValue);
                RADIO_RegisterWrite(0x2F, 0x40);
                RADIO_RegisterWrite(0x30, 0x00);
            }

            if (BW_500KHZ == radioConfiguration.bandWidth)
            {
                regValue = RADIO_RegisterRead(0x31);
                regValue |= 0x80;                                   // Set bit 7
                RADIO_RegisterWrite(0x31, regValue);
            }
        }

        regValue = RADIO_RegisterRead(REG_LORA_INVERTIQ);
        regValue &= ~(1 << 6);                                        // Clear InvertIQ bit
        regValue |= (radioConfiguration.iqInverted & 0x01) << SHIFT6;    // Set InvertIQ bit if needed
        RADIO_RegisterWrite(REG_LORA_INVERTIQ, regValue);

        RADIO_RegisterWrite(REG_LORA_PREAMBLEMSB, radioConfiguration.preambleLen >> SHIFT8);
        RADIO_RegisterWrite(REG_LORA_PREAMBLELSB, radioConfiguration.preambleLen & 0xFF);

        RADIO_RegisterWrite(REG_LORA_FIFOADDRPTR, 0x00);
        RADIO_RegisterWrite(REG_LORA_FIFOTXBASEADDR, 0x00);
        RADIO_RegisterWrite(REG_LORA_FIFORXBASEADDR, 0x00);

        // Errata sensitivity increase for 500kHz BW
        if (0x12 == radioConfiguration.regVersion)
        {
            if ( (BW_500KHZ == radioConfiguration.bandWidth) &&
                (radioConfiguration.frequency >= FREQ_862000KHZ) &&
                (radioConfiguration.frequency <= FREQ_1020000KHZ)
                )
            {
                RADIO_RegisterWrite(0x36, 0x02);
                RADIO_RegisterWrite(0x3a, 0x64);
            }
            else if ( (BW_500KHZ == radioConfiguration.bandWidth) &&
                       (radioConfiguration.frequency >= FREQ_410000KHZ) &&
                       (radioConfiguration.frequency <= FREQ_525000KHZ)
                       )
            {
                RADIO_RegisterWrite(0x36, 0x02);
                RADIO_RegisterWrite(0x3a, 0x7F);
            }
            else
            {
                RADIO_RegisterWrite(0x36, 0x03);
            }

            // LoRa Inverted Polarity 500kHz fix (May 26, 2015 document)
            if ((BW_500KHZ == radioConfiguration.bandWidth) && (1 == radioConfiguration.iqInverted))
            {
                RADIO_RegisterWrite(0x3A, 0x65);     // Freq to time drift
                RADIO_RegisterWrite(0x3B, 25);       // Freq to time invert = 0d25
            }
            else
            {
                RADIO_RegisterWrite(0x3A, 0x65);     // Freq to time drift
                RADIO_RegisterWrite(0x3B, 29);       // Freq to time invert = 0d29 (default)
            }
        }

        // Clear all interrupts (just in case)
        RADIO_RegisterWrite(REG_LORA_IRQFLAGS, 0xFF);
    }
    else
    {
        // FSK modulation
        Radio_WriteFSKFrequencyDeviation(radioConfiguration.frequencyDeviation);
        Radio_WriteFSKBitRate(radioConfiguration.bitRate);

        RADIO_RegisterWrite(REG_FSK_PREAMBLEMSB, (radioConfiguration.preambleLen >> SHIFT8) & 0x00FF);
        RADIO_RegisterWrite(REG_FSK_PREAMBLELSB, radioConfiguration.preambleLen & 0xFF);
		
		// Triggering event: PreambleDetect does AfcAutoOn, AgcAutoOn
		// Also sets RestartRxOnCollision bit
		RADIO_RegisterWrite(REG_FSK_RXCONFIG, 0x9E);

        // Configure PaRamp
        regValue = RADIO_RegisterRead(REG_PARAMP);
        regValue &= ~0x60;    // Clear shaping bits
        regValue |= radioConfiguration.fskDataShaping << SHIFT5;
        RADIO_RegisterWrite(REG_PARAMP, regValue);

        // Variable length packets, whitening, Clear FIFO when CRC fails
        // no address filtering, CCITT CRC and whitening
        regValue = 0xC0;
        if (radioConfiguration.crcOn)
        {
            regValue |= 0x10;   // Enable CRC
        }
        RADIO_RegisterWrite(REG_FSK_PACKETCONFIG1, regValue);
        RADIO_RegisterWrite(REG_FSK_PACKETCONFIG2, 1 << SHIFT6);

        // Syncword value
        for (i = 0; i < radioConfiguration.syncWordLen; i++)
        {
            // Take advantage of the fact that the SYNCVALUE registers are
            // placed at sequential addresses
            RADIO_RegisterWrite(REG_FSK_SYNCVALUE1 + i, radioConfiguration.syncWord[i]);
        }

        // Enable sync word generation/detection if needed, Syncword size = syncWordLen + 1 bytes
        if (radioConfiguration.syncWordLen != 0)
        {
            RADIO_RegisterWrite(REG_FSK_SYNCCONFIG, 0x10 | (radioConfiguration.syncWordLen - 1));
        } else
        {
            RADIO_RegisterWrite(REG_FSK_SYNCCONFIG, 0x00);
        }

        // Clear all FSK interrupts (just in case)
        RADIO_RegisterWrite(REG_FSK_IRQFLAGS1, 0xFF);
        RADIO_RegisterWrite(REG_FSK_IRQFLAGS2, 0xFF);
    }
}

/**
 End of File
*/
