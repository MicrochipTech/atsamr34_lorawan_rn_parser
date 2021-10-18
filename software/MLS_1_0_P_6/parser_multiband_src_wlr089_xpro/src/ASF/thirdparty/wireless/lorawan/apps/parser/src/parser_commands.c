/**
* \file  parser_commands.c
*
* \brief Command definition source file
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
 
#include <stdlib.h>
#include "parser_commands.h"
#include "parser_lorawan.h"
#include "parser_system.h"


static const parserCmdEntry_t maParserLoraSetChCmd[] =
{
    {"freq",         NULL,   Parser_LoraSetChannelFreq,      0,  2},
    {"status",       NULL,   Parser_LoraSetChannelStatus,    0,  2},
    {"drrange",       NULL,  Parser_LoraSetDatarateRange,    0,  3},

};
#define mParserLoraSetChCmdSize (sizeof(maParserLoraSetChCmd) / sizeof(maParserLoraSetChCmd[0]))

static const parserCmdEntry_t maParserLoraGetChCmd[] =
{
    {"freq",         NULL,   Parser_LoraGetChannelFreq,      0,  1},
    {"status",       NULL,   Parser_LoraGetChannelStatus,    0,  1},
    {"drrange",       NULL,  Parser_LoraGetDatarateRange,    0,  1},
};
#define mParserLoraGetChCmdSize (sizeof(maParserLoraGetChCmd) / sizeof(maParserLoraGetChCmd[0]))

static const parserCmdEntry_t maParserLoraSetSubBandCmd[] =
{
    {"status",   NULL, Parser_LoraSetSubBandStatus, 0, 2},
};
#define mParserLoraSetSubBandCmdSize (sizeof(maParserLoraSetSubBandCmd) / sizeof(maParserLoraSetSubBandCmd[0]))

static const parserCmdEntry_t maParserLoraGetSubBandCmd[] =
{
    {"status",   NULL, Parser_LoraGetSubBandStatus, 0, 1},
};
#define mParserLoraGetSubBandCmdSize (sizeof(maParserLoraGetSubBandCmd) / sizeof(maParserLoraGetSubBandCmd[0]))

static const parserCmdEntry_t maParserLoraSetCmd[] =
{
    {"dr",         NULL,   Parser_LoraSetCrtDataRate,      0,  1},
    {"adr",        NULL,   Parser_LoraSetAdr,              0,  1},
    {"devaddr",         NULL,   Parser_LoraSetDevAddr,      0,  1},
    {"deveui",          NULL,   Parser_LoraSetDevEui,       0,  1},
    {"joineui",          NULL,   Parser_LoraSetJoinEui,       0,  1},
    {"nwkskey",         NULL,   Parser_LoraSetNwkSKey,      0,  1},
    {"appskey",         NULL,   Parser_LoraSetAppSKey,      0,  1},
    {"appkey",          NULL,   Parser_LoraSetAppKey,       0,  1},
    {"pwridx",         NULL,   Parser_LoraSetTxPower,      0,  1},
    {"ch",     maParserLoraSetChCmd,  NULL,              mParserLoraSetChCmdSize,  0},
    {"rx2",         NULL,   Parser_LoraSetRx2WindowParams,      0,  2},
    {"bat",         NULL,   Parser_LoraSetBatLevel,      0,  1},
    {"retx",         NULL,   Parser_LoraSetReTxNb,      0,  1},
	{"reps",         NULL,   Parser_LoraSetRepsNb,      0,  1},
    {"ar",         NULL,   Parser_LoraSetAutoReply,      0,  1},
    {"rxdelay1",         NULL,   Parser_LoraSetRxDelay1,      0,  1},
    {"linkchk",   NULL,              Parser_LoraLinkCheck,   0,                 1},
    {"sync",   NULL,              Parser_LoraSetSyncWord,   0,                 1},
    {"upctr",         NULL,   Parser_LoraSetUplinkCounter,      0,  1},
    {"dnctr",         NULL,   Parser_LoraSetDownlinkCounter,      0,  1},
	{"lbt",         NULL,   Parser_LoraSetLbt,      0,  5},
	{"edclass",       NULL, Parser_LoraSetClass,                  0,  1},
	{"mcastenable",   NULL, Parser_LoraSetMcast,                  0,  2},
	{"mcastdevaddr",  NULL, Parser_LoraSetMcastDevAddr,           0,  2},
	{"mcastnwkskey",  NULL, Parser_LoraSetMcastNwksKey,           0,  2},
	{"mcastappskey",  NULL, Parser_LoraSetMcastAppsKey,           0,  2},
	{"mcastfreq",     NULL, Parser_LoraSetMcastFreq,              0,  2},
	{"mcastdr",     NULL, Parser_LoraSetMcastDr,                 0,  2},
	{"aggdcycle",         NULL,   Parser_LoraSetAggregatedDutyCycle,      0,  1},
	{"cryptodevenabled",         NULL,   Parser_LoraSetCryptoDevEnabled,      0,  1},

    //#
    {"subband", maParserLoraSetSubBandCmd, NULL, mParserLoraSetSubBandCmdSize, 0},
	{"joinbackoffenable",  NULL, Parser_LoraSetJoinBackoff,            0,  1},
    {"jntype",  NULL, Parser_LoraSetJoinNonceType, 0, 1},
	{"maxFcntPdsUpdtVal",  NULL, Parser_LoraSetMaxFcntPdsUpdtVal,            0,  1}

};
#define mParserLoraSetCmdSize (sizeof(maParserLoraSetCmd) / sizeof(maParserLoraSetCmd[0]))

static const parserCmdEntry_t maParserLoraGetCmd[] =
{
    {"dr",         NULL,   Parser_LoraGetCrtDataRate,      0,  0},
    {"adr",        NULL,   Parser_LoraGetAdr,              0,  0},
    {"devaddr",         NULL,   Parser_LoraGetDevAddr,      0,  0},
    {"deveui",          NULL,   Parser_LoraGetDevEui,       0,  0},
    {"joineui",          NULL,   Parser_LoraGetJoinEui,       0,  0},
    {"ch",     maParserLoraGetChCmd,  NULL,              mParserLoraGetChCmdSize,  0},
    {"pwridx",         NULL,   Parser_LoraGetTxPower,      0,  0},
    {"rx2",         NULL,   Parser_LoraGetRx2WindowParams,      0,  0},
    {"aggdcycle",         NULL,   Parser_LoraGetAggregatedDutyCycle,      0,  0},
    {"status",         NULL,   Parser_LoraGetMacStatus,      0,  0},
    {"retx",         NULL,   Parser_LoraGetReTxNb,      0,  0},
	{"reps",         NULL,   Parser_LoraGetRepsNb,      0,  0},
    {"mrgn",         NULL,   Parser_LoraGetLinkCheckMargin,      0,  0},
    {"gwnb",         NULL,   Parser_LoraGetLinkCheckGwCnt,      0,  0},
    {"ar",         NULL,   Parser_LoraGetAutoReply,      0,  0},
    {"rxdelay1",         NULL,   Parser_LoraGetRxDelay1,      0,  0},
    {"rxdelay2",         NULL,   Parser_LoraGetRxDelay2,      0,  0},
    {"band",         NULL,   Parser_LoraGetIsm,      0,  0},
    {"sync",         NULL,   Parser_LoraGetSyncWord,      0,  0},
    {"upctr",         NULL,   Parser_LoraGetUplinkCounter,      0,  0},
    {"dnctr",         NULL,   Parser_LoraGetDownlinkCounter,      0,  0},
	{"lbt",         NULL,   Parser_LoraGetLbt,      0,  0},
	{"edclass",       NULL,   Parser_LoraGetClass,                0,  0},
	{"edclasssupported", NULL, Parser_LoraGetSupportedEdClass,    0,  0},
	{"mcastenable",   NULL,   Parser_LoraGetMcast,                0,  1},
	{"mcastdnctr",    NULL,   Parser_LoraGetMcastDownCounter,     0,  1},
	{"mcastdevaddr",  NULL,   Parser_LoraGetMcastDevAddr,         0,  1},
	{"mcastfreq",     NULL, Parser_LoraGetMcastFreq,              0,  1},
	{"mcastdr",     NULL, Parser_LoraGetMcastDr,                 0,  1},
    //#
    {"subband", maParserLoraGetSubBandCmd, NULL, mParserLoraGetSubBandCmdSize, 0},
	{"pktrssi", NULL, Parser_LoraGetMacLastPacketRssi, 0,0},
	{"isfpending", NULL, Parser_LoraGetIsFpending, 0,0},
	{"isdlack", NULL, Parser_LoraGetMacDlAckReqd, 0,0},
	{"lastchid", NULL, Parser_LoraGetMacLastChId, 0,0},
	{"dutycycletime", NULL, Parser_LoraGetMacPendingDutyCycle, 0,0},
	{"cnfretrycnt", NULL, Parser_LoraGetMacCnfRetryCnt, 0,0},
	{"uncnfretrycnt", NULL, Parser_LoraGetMacUncnfRetryCnt, 0,0},
	{"nxtPayloadSize", NULL, Parser_LoraGetMacNextPayloadSize, 0,0},
	{"joindutycycletime", NULL, Parser_LoraGetJoindutycycleremaining,0,0},
    {"jntype",  NULL, Parser_LoraGetJoinNonceType, 0, 0},
	{"joinbackoffenable",  NULL, Parser_LoraGetJoinBackoff,          0,0}
    
};
#define mParserLoraGetCmdSize (sizeof(maParserLoraGetCmd) / sizeof(maParserLoraGetCmd[0]))

const parserCmdEntry_t maParserLoraCmd[] =
{
    {"reset",   NULL,                Parser_LoraReset,   0,                      1},
    {"join",    NULL,                Parser_LoraJoin,   0,                      1},
    {"tx",    NULL,                Parser_LoraSend,   0,                      3},
    {"set",     maParserLoraSetCmd,  NULL,              mParserLoraSetCmdSize,  0},
    {"get",     maParserLoraGetCmd,  NULL,              mParserLoraGetCmdSize,  0},
    {"pause",    NULL,               Parser_LoraPause,  0,                      0},
    {"resume",    NULL,              Parser_LoraResume, 0,                      0},
    {"save",   NULL,                 Parser_LoraSave,   0,                      0},
    {"forceENABLE",   NULL,          Parser_LoraForceEnable,   0,               0},
};

#define mParserLoraCmdSize  (sizeof(maParserLoraCmd) / sizeof(maParserLoraCmd[0]))

#ifdef PARSER_SYS_TEST_SUPPORTED
static const parserCmdEntry_t maParserSysSetCmd[] =
{
    {"nvm",         NULL,   Parser_SystemSetNvm,      0,  2},
    {"pinmode",     NULL,   Parser_SystemSetPinMode,  0,  2},
    {"pindig",      NULL,   Parser_SystemSetPinDig,   0,  2},
};
#define mParserSysSetCmdSize (sizeof(maParserSysSetCmd) / sizeof(maParserSysSetCmd[0]))
#endif
static const parserCmdEntry_t maParserSysGetCmd[] =
{
	
    {"ver",         NULL,   Parser_SystemGetVer,      0,  0},
#ifdef PARSER_SYS_TEST_SUPPORTED		

	{"nvm",         NULL,   Parser_SystemGetNvm,      0,  1},
    {"vdd",         NULL,   Parser_SystemGetBattery,      0,  0},
    {"hweui",       NULL,   Parser_SystemGetHwEui,      0,  0},
    {"pindig",      NULL,   Parser_SystemGetPinDig,   0,  1},
    {"pinana",      NULL,   Parser_SystemGetPinAnalog,   0,  1},
#endif		
};
#define mParserSysGetCmdSize (sizeof(maParserSysGetCmd) / sizeof(maParserSysGetCmd[0]))

const parserCmdEntry_t maParserSysCmd[] =
{
#ifdef PARSER_SYS_TEST_SUPPORTED	
    {"eraseFW",    NULL,               Parser_SystemBootload, 0,                      0},
    {"set",     maParserSysSetCmd,  NULL,              mParserSysSetCmdSize,  0},
#endif		
    {"factoryRESET", NULL,               Parser_SystemFactReset,  0,    0},
	{"reset",    NULL,               Parser_SystemReboot, 0,                      0},
    {"get",     maParserSysGetCmd,  NULL,              mParserSysGetCmdSize,  0},
#ifdef CONF_PMM_ENABLE
	{"sleep",    NULL,                Parser_SystemSleep,  0,                      2},
#endif /* CONF_PMM_ENABLE */
};

#define mParserSysCmdSize  (sizeof(maParserSysCmd) / sizeof(maParserSysCmd[0]))


const parserCmdEntry_t maParserBaseCmd[] =
{
    {"mac",       maParserLoraCmd,       NULL,   mParserLoraCmdSize,        0},

    {"sys",        maParserSysCmd,        NULL,   mParserSysCmdSize,         0},
};

#define mParserBaseCmdSize  (sizeof(maParserBaseCmd) / sizeof(maParserBaseCmd[0]))

const parserCmdEntry_t* gpParserStartCmd = &maParserBaseCmd[0];
const uint8_t gParserStartCmdSize = mParserBaseCmdSize;
