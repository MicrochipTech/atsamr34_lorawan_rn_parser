# Command User's Guide
> "Wireless Made Easy!" - Control SAM R34 IC or WLR089U0 Module with ASCII commands over UART

[Back to Main page](../README.md#top)

## A la carte

1. [Command Syntax](#step1)
1. [Command Organization](#step2)
1. [System Commands](#step3)
1. [MAC Commands](#step4)
1. [Limitations](#step5)


## Command Syntax<a name="step1"></a>

To issue commands to the device, the user sends keywords followed by optional parameters. Commands (keywords) are case-sensitive, and spaces must not be used in parameters. Hex input data can be uppercase or lowercase. String text data, such as OTAA used for the join procedure, can be uppercase or lowercase.

The use of shorthand for parameters is NOT supported.

Depending on the command, the parameter may expect values in either decimal or hexadecimal form; refer to the command description for the expected form. For example, when configuring the frequency, the command expects a decimal value in Hertz such as `868100000` (868.1 MHz). Alternatively, when configuring the LoRaWAN device address, the hex value is entered into the parameter as `aabbccdd`. To enter a number in hex form, use the value directly. For example, the hex value 0xFF would be entered as `FF`.

## Command Organization<a name="step2"></a>

There are three general command categories.

| Command Type | Keyword | Description |
| ------------ | ------- | ----------- |
| System | <`sys`> | Issues system level behavior actions, gathers status information on the firmware and hardware version. |
| LoRaWAN Class A and Class C Protocols | <`mac`> | Issues LoRaWAN Class A and Class C protocols network communication behaviors, actions and configuration commands. |
| Transceiver commands | <`radio`> | Issues radio specific configuration, directly accessing and updating the transceiver setup |

## System Commands<a name="step3"></a>

System commands begin with the system keyword <`sys`> and include the categories shown below.

| Parameter | Description |
| --------- | ----------- |
| `sleep` | Puts the system in sleep for a finit number of milliseconds |
| `reset` | Resets and restarts the device `
| `set` | Sets specified system parameter values. |
| `get` | Gets specified system parameter values. |


### `sys sleep <mode> <length>`

`<mode>`: `standby` or `backup`\
`<length>`: decimal number representing the number of milliseconds the system is put to sleep, from 1000 (1s) to 130990000 (36h23m10s).\
Response: `sleep_ok` after the system gets back from Sleep mode\
Response: `invalid_param` if the parameters are not valid

Pressing the User button (SW0) on ATSAMR34 XPRO/WLR089 XPRO will wake up the system placed in standby mode.

Example: `sys sleep standby 60000`  // Puts the system to sleep for 60 sec

### `sys reset`

This command resets and restarts the device; stored internal configurations will be loaded automatically upon reboot.

Response: no response

Example: `sys reset`  // Resets and restarts the device

### System Set Commands

#### `sys set customparam <value>`

Used as an example command to set an application parameter.

`<value>`: decimal number representing the custom value to set, from 0 to 4294967295 (2<sup>32</sup>-1)

Response: `ok` if the value is valid\
Response: `invalid_param` if not a valid entry

Example: `sys set customparam 3235`

### System Get Commands

| Parameter | Description |
| --------- | ----------- |
| customparam | Returns the custom parameter value |
| ver | Returns the information on hardware platform, firmware version, release date |
| hweui | Returns the preprogrammed EUI node address |

#### `sys get customparam`

Returns the custom parameter value.

Response: decimal value

Example: `sys get customparam`

#### `sys get ver`

Returns the information on hardware platform, firmware version, release date.

Response: USER BOARD MLS_SDK_1_0_P_5 Sep  3 2020 10:45:46

Example: `sys get ver` // Returns version-related information

#### `sys get hweui`

Returns the preprogrammed EUI node address.

Response: hexadecimal number representing the preprogrammed EUI node address.

* SAM R34 Xplained Pro: unique EUI is stored in EDBG component
* WLR089U0 Xplained Pro: unique EUI is stored in data flash memory of the module

Example: `sys get hweui` // Reads the preprogrammed EUI node address

> The preprogrammed EUI node address is a read-only value and cannot be changed or erased. This value can be used to configure the device EUI using the `mac set deveui` command.

```
sys get hweui
<hweui>
mac set deveui <hweui>

```

## MAC Commands<a name="step4"></a>

LoRaWAN protocol commands beging with the keyword <`mac`> and include the categories shown below.

| Parameter | Description |
| --------- | ----------- |
| reset | Resets the device to a specific frequency band |
| tx | Sends the data string on a specified port number and sets default values for most of the LoRaWAN parameters |
| join | Informs the device to join the configured network |
| forceENABLE | Enables the device after the LoRaWAN network server commanded the end device to become silent immediately |
| pause | Pauses Microchip LoRaWAN Stack functionality to allow radio transceiver configuration. |
| resume | Restores the Microchip LoRaWAN Stack functionality |
| set | Accesses and modifies specific MAC related parameters |
| get | Reads back current MAC related parameters from the module |

### `mac reset <band>`

`<band>`: string representing the band selected.
```
"868"      // EU868
"433"      // EU433
"na915"    // NA915
"au915"    // AU915
"kr920"    // KR290
"jpn923"   // JPN923
"brn923"   // AS923
"cmb923"   // AS923
"ins923"   // AS923
"laos923"  // AS923
"nz923"    // AS923
"sp923"    // AS923
"twn923"   // AS923
"thai923"  // AS923
"vtm923"   // AS923
"ind865"   // IND865
```
Response: `ok` if band is valid.
Response: `invalid_param` if band is not valid.

Example: `mac reset 868` // Sets the default values and selects the 868 default band

> This command will set default values for most of the LoRaWAN parameters. Everything set prior to this command will lose its set value.

> This command MUST be the FIRST command issued in order to initialize the Microchip LoRaWAN Stack accordingly with the selected region.

### `mac tx <type> <portno> <data>`

`<type>`: string representing the uplink payload type, either `cnf` or `uncnf` (`cnf` - confirmed message, `uncnf` - unconfirmed message)\
`<portno>`: decimal number representing the port number, from 1 to 223\
`<data>`: hexadecimal value. The length of `<data>` bytes capable of being transmitted are dependent upon the set data rate (for further details, refer to the _LoRaWAN Specification v1.0.4_).

Response: this command may reply with two responses. The first response will be received immediately after entering the command. In case the command is valid (`ok` reply received), a second reply will be received after the end of the uplink transmission. For further details, refer to the _LoRaWAN Specification v1.0.4_.

Response after entering the command:
* `ok` - if parameters and configurations are valid and the packet was forwarded to the radio transceiver for transmission
* `invalid_param` - if parameters are not valid
* `not_joinded` - if the network is not joined
* `no_free_ch` - if all channels are busy
* `silent` - if the device is in Silent Immediately state
* `frame_counter_err_rejoin_needed` – if the frame counter rolled over
* `busy` – if MAC state is not in an Idle state
* `mac_paused` – if MAC was paused and not resumed back
* `invalid_data_len` if application payload length is greater than the maximum
application payload length corresponding to the current data rate 

Response after the first uplink transmission attempt:
* `mac_tx_ok` if uplink transmission was successful and no downlink data was received back from the server
* `mac_rx <portno> <data>` if transmission was successful, `<portno>`: port number, from 1 to 223; `<data>`: hexadecimal value that was received from the server
* `mac_err` if transmission was unsuccessful, ACK not received back from the server
* `invalid_data_len` if application payload length is greater than the maximum application payload length corresponding to the current data rate. This can occur after an earlier uplink attempt if retransmission back-off has reduced the data rate.

A confirmed message will expect an acknowledgment from the server; otherwise, the message will be retransmitted by the number indicated by the command `mac set retx <value>`, whereas an unconfirmed message will not expect any acknowledgment back from the server. For further details, refer to the LoRaWAN™ Specification V1.0.4.

The port number allows multiplexing multiple data streams on the same link. For example, the end device can send measurements on one port number and configuration data on another. The server application can then distinguish the two types of data based on the port number.

Example: `mac tx cnf 4 5A5B5B` // Sends a confirmed frame on port 4 with application payload 5A5B5B

If the automatic reply feature is enabled and the server sets the Frame Pending bit or
initiates downlink confirmed transmissions, multiple responses will be displayed after
each downlink packet is received by the device. A typical scenario for this case would
be (prerequisites: free LoRaWAN channels available and automatic reply enabled):
* The module sends a packet on port 4 with application payload 0xAB
* Radio transmission is successful and the device will display the first response: `ok`
* The server needs to send two separate downlink confirmed packets back on port 1 with the following data: `0xAC`, then `0xAF`. First it will transmit the first one (`0xAC`)
and will set the Frame Pending bit. The device will display the second response `mac_rx 1 AC`\
* The device will initiate an automatic uplink unconfirmed transmission with no application payload on the first free channel because the Frame Pending bit was set in the downlink transmission
* The server will send back the second confirmed packet (0xAF). The device will display a third response `mac_rx 1 AF`
* The device will initiate an automatic unconfirmed transmission with no application payload on the first free channel because the last downlink transmission was confirmed, so the server needs an ACK
* If no reply is received back from the server, the device will display the fourth response after the end of the second Receive window: `mac_tx_ok`
* After this scenario, the user is allowed to send packets when at least one enabled channel is free

Based on this scenario, the following responses will be displayed by the device:
* `mac tx cnf 4 AB`
* `ok`
* `mac_rx 1 AC`
* `mac_rx 1 AF`
* `mac_tx_ok`

### `mac join <mode>`

`<mode>`: string representing the join procedure type (case-insensitive), either `otaa`
or `abp` (`otaa` – over-the-air activation, `abp` – activation by personalization).

Response: this command may reply with two responses. The first response will be received immediately after entering the command. In case the command is valid (`ok` reply received) a second reply will be received after the end of the join procedure. For further details, refer to the LoRaWAN™ Specification V1.0.4.

Response after entering the command:
* `ok` – if parameters and configurations are valid and the join request packet was
forwarded to the radio transceiver for transmission
* `invalid_param` – if `<mode>` is not valid
* `keys_not_init` – if the keys corresponding to the Join mode (`otaa` or `abp`) were not configured
* `no_free_ch` – if all channels are busy
* `silent` – if the device is in a Silent Immediately state
* `busy` – if MAC state is not in an Idle state
* `mac_paused` – if MAC was paused and not resumed back

Response after the join procedure:
* `denied` if the join procedure was unsuccessful (the device attempted to join the
network, but was rejected)
* `accepted` if the join procedure was successful

This command informs the device it should attempt to join the configured network. Device activation type is selected with `<mode>`. Parameter values can be `otaa` (over-the-air activation) or `abp` (activation by personalization). The `<mode>` parameter is not case sensitive. Before joining the network, the specific parameters for each activation type should be configured (for over the air activation: device EUI, application EUI, application key; for activation by personalization: device address, network session key, application session key).

Example: `mac join otaa` // Attempts to join the network using over-the-air activation

### `mac forceENABLE`

Response: `ok`

The network can issue a certain command (Duty Cycle Request frame with parameter 255) that would require the device to go silent immediately. This mechanism disables any further communication of the device, effectively isolating it from the network. Using `mac forceENABLE` after this network command has been received restores the module’s connectivity by allowing it to send data.

Example: `mac forceENABLE` // Disables the Silent Immediately state

> The `silent immediately status bit` of the MAC status register indicates the device has been silenced by the network.

### `mac pause`

Response: 0 – 4294967295 (decimal number representing the number of milliseconds
the mac can be paused).

This command pauses the Microchip LoRaWAN stack functionality to allow radio transceiver configuration. Through the use of `mac pause`, radio commands can be generated between a LoRaWAN Class A protocol uplink application (`mac tx` command), and the LoRaWAN Class A protocol Receive windows (second response for the `mac tx`
command). This command will reply with the time interval in milliseconds that the
transceiver can be used without affecting the LoRaWAN functionality. The maximum
value (4294967295) is returned whenever the Microchip LoRaWAN stack functionality is in Idle state and the transceiver can be used without restrictions. ‘0’ is returned when the LoRaWAN stack functionality cannot be paused.

For example, when operating in LoRaWAN Class C mode, the receiver is continuously in receive. The `mac pause` command will return ‘0’ indicating that the LoRaWAN stack
cannot be paused.

After the radio configuration is complete, the `mac resume` command must be used to return to LoRaWAN protocol commands.

Example: `mac pause` // Pauses the LoRaWAN stack functionality if the response is different from 0

> If already joined to a network, this command MUST be called BEFORE configuring the radio parameters, initiating radio reception, or transmission.

### `mac resume`

Response: `ok`

This command resumes LoRaWAN stack functionality, in order to continue normal functionality after being paused.

Example: `mac resume` // Resumes the Microchip LoRaWAN stack functionality

> This command MUST be called AFTER all radio commands have been issued and all the corresponding asynchronous messages have been replied.

### MAC Set Commands

| Parameter | Description |
| --------- | ----------- |
| adr | Sets the adaptive data rate |
| appkey | Sets the application key |
| ar | Sets the state of the automatic reply |
| appskey | Sets the application session key |
| bat | Sets the battery level needed for Device Status Answer frame command response |
| ch | Allows modification of channel related parameters |
| devaddr | Sets the unique network device address |
| deveui | Sets the globally unique identifier |
| dnctr | Sets the value of the downlink frame counter that will be used for the next downlink reception |
| dr | Sets the data rate to be used for the next transmissions |
| edclass | Sets the LoRaWAN operating class |
| joinbackoffenable | Sets Join backoff support |
| joineui | Sets the join/application EUI key |
| lbt | Sets the Listen-Before-Talk (LBT) parameters |
| linkchk | Sets the time interval for the link check process to be triggered |
| mcastenable | Sets the Multicast state to on, or off |
| mcastappskey | Sets the multicast application session key |
| mcastdevaddr | Sets the multicast network device address |
| mcastdnctr | Sets the value of the multicast downlink frame counter that will be used for the next multicast downlink reception |
| mcastnwkskey | Sets the multicast network session key |
| nwkskey | Sets the network session key |
| pwridx | Sets the output power to be used on the next transmissions |
| reps | Sets the number of repetition for the unconfirmed uplink message |
| retx | Sets the number of retransmissions to be used for an uplink confirmed packet |
| rx2 | Sets the data rate and frequency used for the second Receive window |
| rxdelay1 | Sets the value used for the first Receive window delay |
| subband | Sets the status of the frequency subbands |
| sync | Sets the synchronization word for the LoRaWAN communication |
| upctr | Sets the value of the uplink frame counter that will be used for the next uplink transmission |

#### `mac set adr <status>`

`<state>`: string value representing the state, either `on` or `off`

Response: `ok` if state is valid
Response: `invalid_param` if state is not valid

This command configures the Adaptive Data Rate (ADR). LoRa network allows the end-devices to individually use any of the possible data rates, this is referred to as Adaptive Data Rate (ADR). If the ADR is set, the network will control the data rate of the end-device through the appropriate MAC commands. If the ADR is not set, the network will not attempt to control the data rate of the end-device regardless of the received signal quality.

Example: `mac set adr on` // Enable ADR

#### `mac set appkey <appKey>`

`<appKey>`: 16-byte hexadecimal number representing the application key 

Response: `ok` if key is valid\
Response: `invalid_param` if key is not valid

This command sets the application key for the device. The application key is used to derive the security credentials for communication during over-the-air activation.

Example: `mac set appkey 00112233445566778899AABBCCDDEEFF`

#### `mac set appskey <appSessKey>`

`<appSessKey>`: 16-byte hexadecimal number representing the application session key

Response: `ok` if key is valid\
Response: `invalid_param` if key is not valid

This key provides security for communication between module and application server.

Example: `mac set appskey AFBECD56473829100192837465FAEBDC`

#### `mac set ar <state>`

`<state>`: string value representing the state, either `on` or `off`

Response: `ok` if state is valid
Response: `invalid_param` if state is not valid

By enabling the automatic reply, the module will transmit a packet without a payload immediately after a confirmed downlink is received, or when the Frame Pending bit has been set by the server. If set to `off`, no automatic reply will be transmitted.

Example: `mac set ar on` // Enables the automatic reply process inside the device

> The device implementation will initiate automatic transmissions with no application payload if the automatic reply feature is enabled and the server sets the Frame Pending bit or initiates a confirmed downlink transmission. In this case, if all enabled channels are busy due to duty cycle limitations, the stack will wait for the first channel that will become free to transmit. The user will not be able to initiate uplink transmissions until the automatic transmissions are done.

#### `mac set bat <level>`

`<level>`: decimal number representing the level of the battery, from 0 to 255. `‘0’`
means external power, `‘1’` means low level, `254` means high level, `255` means the end device was not able to measure the battery level.

Response: `ok` if the battery level is valid\
Response: `invalid_param` if the battery level is not valid

This command sets the battery level required for Device Status Answer frame in use with the LoRaWAN Class A protocol.

Example: `mac set bat 127` // Battery is set to ~50%

#### `mac set ch freq <channelID> <frequency>`

`<channelID>`: decimal number representing the channel number. (e.g. range from 3 to 15 for EU)\
`<frequency>`: decimal number representing the frequency in Hz.

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the operational frequency on the given channel ID. For EU region, the default channels (0-2) cannot be modified in terms of frequency.

Example: `mac set ch freq 13 864000000` // Define frequency for channel 13 to be 864 MHz

> Check out the file `lorawan_multiband.h` for regional parameters details related to the frequency band to operate.

#### `mac set ch drrange <channelID> <minRange> <maxRange>`

`<channelID>`: decimal number representing the channel\
`<minRange>`: decimal number representing the minimum data rate\
`<maxRange>`: decimal number representing the maximum data rate

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

Example: `mac set ch drrange 13 0 2` // Using EU863-870 band: on channel 13 the data rate can range from 0 (SF12/125 kHz) to 2 (SF10/125 kHz) as required

> Check out the files `lorawan_multiband.h` and `conf_regparams.h` for regional parameters details related to the frequency band to operate.

#### `mac set ch status <channelID> <status>`

`<channelID>`: decimal number representing the channel\
`<status>`: string value representing the state, either `on` or `off`

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

Example: `mac set ch status 4 off` // channel ID 4 is disabled from use

> `<channelID>` parameters (frequency, data range, duty cycle) must be issued prior to enabling the status of that channel

#### `mac set cryptodevenabled <status>`

`<status>`: string value representing the state, either `on` or `off`

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command informs the stack that crypto device is used for key storage.

Example: `mac set cryptodevenabled on`

#### `mac set devaddr <address>`

`<address>`: 4-byte hexadecimal number representing the device address, from 00000000 to FFFFFFFF

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command configures the device with a 4-byte unique network device address `<address>`. The `<address>` MUST be UNIQUE to the current network. This must be
directly set solely for activation by personalization devices. This parameter must not be set before attempting to join using over-the-air activation because it will be overwritten once the join process is over.

Example: `mac set devaddr ABCDEF01`

#### `mac set deveui <devEUI>`

`<devEUI>`: 8-byte hexadecimal number representing the device EUI

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the globally unique device identifier for the device. The identifier must be set by the host MCU. The module contains a pre-programmed unique EUI and
can be retrieved using the sys get hweui command or user
provided EUI can be configured using the `mac set deveui` command.

Example: `mac set deveui 0004A30B001A55ED`

#### `mac set dnctr <fCntDown>`

`<fCntDown>`: decimal number representing the value of the downlink frame counter that will be used for the next downlink reception, from 0 to 4294967295.

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command sets the value of the downlink frame counter that will be used for the next downlink reception.

Example: `mac set dnctr 30`

#### `mac set dr <dataRate>`

`<dataRate>`: decimal number representing the data rate, but within the limits of the data rate range for the defined channels.

Response: `ok` if data rate is valid\
Response: `invalid_param` if data rate is not valid

This command sets the data rate to be used for the next transmission.

Example: `mac set dr 5` // on EU863-870; SF7/125 kHz

> Check out the files `lorawan_multiband.h` and `conf_regparams.h` for regional parameters details related to the frequency band to operate.

#### `mac set edclass <class>`

`<class>`: a letter representing the LoRaWAN device class, either `a` or `c`

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameters is not valid

This command sets the end device LoRaWAN operating class. The default end device class is Class A. When the class is configured as Class C, the end device will enter Class C continuous receive mode after the next uplink message is sent. The LoRaWAN network server must also configure this node as a Class C node. The network server configuration is performed out of band from LoRaWAN communications.

Example: `mac set edclass c`

#### `mac set joinbackoffenable <status>

`<status>`: string value representing the state, either `on` or `off`

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command configures Join backoff support as per in Specification.

Example: `mac set joinbackoffenable on`

#### `mac set joineui <joinEUI>`

`<joinEUI>`: 8-byte hexadecimal number representing the join/application EUI

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command sets the join/application EUI.

Example: `mac set joineui 0011223344556677`

#### `mac set lbt <scanPeriod> <threshold> <maxRetryChannels> <numOfSamples> <transmitOn>`

`<scanPeriod>`: scan duration in ms of a single channel, from 0 to 0xFFFF\
`<threshold>`: threshold which channel is assumed to be occupied, from 0 to 0xFFFF\
`<maxRetryChannels>`: number of MAX channels to do LBT. If Set to 0xFFFF, LBT will be done till a free channel is found. If set to 0, retry will not be done, so LBT will be done once. If set to 1, retry will be done once, so LBT will be done twice. From 0 to 0xFFFF\
`<numOfSamples>`: number of RSSI read samples for a single channel, from to 0 to 255\
`<transmitOn>`: switch for radio to decide if the transmit request is LBT based, 1 or 0

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the Listen-Before-Talk (LBT) parameters.

Example: `mac set lbt 5 -80 5 5 1`

> Only applicable for regions which support LBT like Korea or Japan

#### `mac set linkchk <linkChk>`

`<linkChk>`: decimal number from 0 to 65535 that sets the time interval in seconds for the link check process

Response: `ok` if the time interval is valid\
Response: `invalid_param` if the time interval is not valid

This command sets the time interval for the link check process to be triggered periodically. A `<value>` of '0' will disable the link check process. When the time interval expires, the next application packet that will be sent to the server will include also a link check MAC command.

Example: `mac set linkchk 600` // the device will attempt a link check process at 600-second intervals

#### `mac set mcastenable <state> <groupID>`

`<state>`: string value representing the state, either on or off\
`<groupID>`: decimal number representing the group ID from 0 to 3

Response: `ok` if arguments are valid\
Response: `invalid_param` if the arguments are not valid

This command sets the end device Multicast the group ID and the state to either be enabled or disabled. When multicast is enabled, and the device is operating in Class C continuous receive mode, the end device can receive multicast messages from the server.

Example: `mac set mcastenable on 0`

> Multicast keys must be set prior to execute this command

#### `mac set mcastappskey <mcastApplicationSessionkey> <groupID>`

`<mcastApplicationSessionkey>`: 16-byte hexadecimal number representing the
application session key\
`<groupID>`: decimal number representing the group ID from 0 to 3

Response: `ok` if arguments are valid\
Response: `invalid_param` if the arguments are not valid

This command sets the multicast application session key for the module. This key identifies the multicast application session key used when the network sends a multicast message from an application.

Example: `mac set mcastappskey 29100192AFBECD564738837465FAEBDC 0`

#### `mac set mcastdevaddr <mcastAddress> <groupID>`

`<mcastAddress>`: 4-byte hexadecimal number representing the device multicast
 address, from 00000000 - FFFFFFFF\
`<groupID>`: decimal number representing the group ID from 0 to 3

Response: `ok` if arguments are valid\
Response: `invalid_param` if the arguments are not valid

This command configures the module with a 4-byte multicast network device address. The address MUST match the multicast address on the current network. This must be directly set for multicast devices.

Example: `mac set mcastdevaddr 54ABCDEF 0`

#### `mac set mcastdr <mcastDatarate> <groupID>`

`<mcastDatarate>`: decimal number representing the multicast data rate (0-5)
`<groupID>`: decimal number representing the group id from 0 to 3

Response: `ok` if arguments are valid\
Response: `invalid_param` if the arguments are not valid

Example: `mac set mcastdr 5 0` // set dr 5 to group id 0

#### `mac set mcastnwkskey <mcastNetworkSessionkey> <groupID>`

`<mcastNetworkSessionkey>`: 16-byte hexadecimal number representing the
network session key\
`<groupID>`: decimal number representing the group id from 0 to 3

Response: `ok` if arguments are valid
Response: `invalid_param` if the arguments are not valid

This command sets the multicast network session key for the module. This key is 16
bytes in length, and provides security for communication between the module and multicast network server.

Example: `mac set mcastnwkskey 6AFBECD1029384755647382910DACFEB 0`

#### `mac set nwkskey <nwkSessKey>`

`<nwkSessKey>`: 16-byte hexadecimal number representing the network session key

Response: `ok` if key is valid\
Response: `invalid_param` if key is not valid

This command sets the network session key for the module. This key is 16 bytes in
length, and provides security for communication between the module and network
server.

Example: `mac set nwkskey 1029384756AFBECD5647382910DACFEB`

#### `mac set pwridx <pwrIndex>`

`<pwrIndex>`: decimal number representing the index value for the output power, from 0 to 7 for EU 868 MHz, from 5 to 10 for NA 915 MHz

Response: `ok` if power index is valid\
Response: `invalid_param` if power index is not valid

This command sets the output power to be used on the next transmissions.

Example: `mac set pwridx 1` // Sets the TX output power to Max EIRP - 2 dB on the next transmission for a 868 MHz EU band. By default, the Max EIRP is considered to be +16dBm.\
Example: `mac set pwridx 5` // Sets the TX output power to 20 dBm on the next transmission for a 915 MHz NA band

Check out the LoRaWAN regional parameters and the End-device Output Power encoding section for more details.


#### `mac set reps <repsNb>`

`<repsNb>`: decimal number representing the number of repetition for the unconfirmed uplink messages, from 0 to 255

Response: `ok` if value is valid\
Response: `invalid_param` if value is not valid

Example: `mac set reps 5`

#### `mac set retx <reTxNb>`

`<reTxNb>`: decimal number representing the number of retransmissions for an uplink
confirmed packet, from 0 to 255

Response: `ok` if value is valid\
Response: `invalid_param` if value is not valid

This command sets the number of retransmissions to be used for an uplink confirmed packet, if no downlink acknowledgment is received from the server.

Example: `mac set retx 5` // The number of retransmissions made for an uplink confirmed packet is set to 5

#### `mac set rx2 <dataRate> <frequency>`

`<dataRate>`: decimal number representing the data rate, from 0 to 7 for EU, from 8 to 13 for NA915\
`<frequency>`: decimal number representing the frequency in Hz, from 863000000 to
870000000 Hz for EU, from 923300000 to 927500000 Hz in 600kHz steps for NA915

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the data rate and frequency used for the second Receive window. The configuration of the Receive window parameters should be in concordance with
the server configuration.

Example: `mac set rx2 3 865000000` // Receive window 2 is configured with
SF9/125 kHz data rate with a center frequency of 865 MHz\
Example: `mac set rx2 8 927500000` // Receive window 2 is configured with
SF12/500 kHz data rate with a center frequency of 927.5 MHz

#### `mac set rxdelay1 <rxDelay>`

`<rxDelay>`: decimal number representing the delay between the transmission and the first Reception window in milliseconds, from 0 to 65535

Response: `ok` if value is valid\
Response: `invalid_param` if value is not valid

This command will set the delay between the transmission and the first Reception window to the `<rxDelay>` in milliseconds. The delay between the transmission and
the second Reception window is calculated in software as the delay between the
transmission and the first Reception window + 1000 (ms).

Example: `mac set rxdelay1 1000` // Set the delay between the transmission and the first Receive window to 1000 ms

#### `mac set subband status <subbandID> <status>`

`<subbandID>`: decimal number representing the subband, from 1 to 8\
`<status>`: string value representing the state, either `on` or `off`

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

Example: `mac set subband status 2 on` // subband 2 is enabled

#### `mac set sync <synchWord>`

`<synchWord>`: one byte long hexadecimal number representing the synchronization
word for the LoRaWAN communication

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command sets the synchronization word for the LoRaWAN communication. The configuration of the synchronization word should be in accordance with the Gateway
configuration.

Example: `mac set sync 34` // Synchronization word is configured to use the 0x34 value

#### `mac set upctr <fCntUp>`

`<fCntUp>`: decimal number representing the value of the uplink frame counter that will be used for the next uplink transmission, from 0 to 4294967295 

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command sets the value of the uplink frame counter that will be used for the next
uplink transmission.

Example: `mac set upctr 10`

### MAC Get commands

| Parameter | Description |
| --------- | ----------- |
| adr | Gets the state of adaptive data rate for the device |
| ar | Gets the state of the automatic reply |
| ch | Gets parameters related information which pertains to channel operation and behaviors |
| devaddr | Gets the current stored unique network device address for that specific end device |
| deveui | Gets the current stored globally unique identifier for that specific end device |
| dnctr | Gets the value of the downlink frame counter that will be used for the next downlink reception |
| dr | Gets the data rate to be used for the next transmission |
| dutycycletime | Gets the pending duty cycle value |
| edclass | Gets the LoRaWAN operating class for the end device |
| edclassupported | Gets the supported LoRaWAN class |
| joinbackoffenable | Gets the state of Join backoff support |
| joindutycycletime | Gets the join duty cycler emaining |
| joineui | Gets the join/application identifier for the end device |
| gwnb | Gets the number of gateways that successfully received the last Link Check Request frame |
| lbt | Gets the Listen-Before-Talk (LBT) parameters |
| mcastenable | Gets the state of multicast reception for the end device |
| mcastdevaddr | Gets the current stored multicast network device address for the end device |
| mcastdnctr | Gets the value of the multicast downlink frame counter that will be used for the next multilink downlink reception |
| mrgn | Gets the demodulation margin as received in the last Link Check Answer frame |
| pktrssi | Gets the last packet RSSI |
| pwridx | Gets the output power |
| reps | Gets the number of repetition for the unconfirmed uplink message |
| retx | Gets the number of retransmissions to be used for an uplink confirmed packet |
| rx2 | Gets the data rate and frequency used for the second Receive window |
| rxdelay1 | Gets the value used for the first Receive window delay |
| rxdelay2 | Gets the value used for the second Receive window delay |
| status | Gets the current status of the stack |
| subband | Gets the status of the frequency subbands |
| sync | Gets the synchronization word for the LoRaWAN communication |
| upctr | Gets the value of the uplink frame counter that will be used for the next uplink transmission |

#### `mac get adr`

Response: string representing the state of the adaptive data rate mechanism, either `on` or `off`

This command will return the state of the adaptive data rate mechanism. It will reflect if the ADR is `on` or `off` on the requested device.

Default: `off`\
Example: `mac get adr`

#### `mac get ar`

Response: string representing the state of the automatic reply, either `on` or `off`

This command will return the current state for the automatic reply (AR) parameter. The
response will indicate if the AR is `on` or `off`.

Default: `off`\
Example: `mac get ar`

#### `mac get ch freq <channelID>`

`<channelID>`: decimal number representing the channel number. (e.g. range from 0 to 15 for EU)

Response: decimal number representing the frequency of the channel, from
863000000 to 870000000 Hz for EU, from 902300000 to 914900000 Hz for NA, depending on the frequency band selected

This command returns the frequency on the requested `<channelID>`, entered in decimal form.

Example: `mac get ch freq 0`

#### `mac get ch drrange <channelID>`

`<channelID>`: decimal number representing the channel, from 0 to 15 for EU, from 0 to 71 for NA

Response: decimal number representing the minimum data rate of the channel, from 0 to 7 and a decimal number representing the maximum data rate of the channel, from 0 to 7

This command returns the allowed data rate index range on the requested `<channelID>`, entered in decimal form. The <minRate> and <maxRate> index values are returned in decimal form and reflect index values. 

Example: `mac get ch drrange 0`

#### `mac get ch status <channelID>`

`<channelID>`: decimal number representing the channel, from 0 to 15 for EU, from 0 to 71 for NA

Response: string representing the state of the channel, either `on` or `off`. 

This command returns if `<channelID>` is currently enabled for use. `<channelID>` is entered in decimal form and the response will be on or off reflecting the channel is enabled or disabled appropriately.

Example: `mac get ch status 2`

#### `mac get devaddr`

Response: 4-byte hexadecimal number representing the device address, from
00000000 to FFFFFFFF. 

This command will return the current end-device address of the device.

Default: `00000000`\
Example: `mac get devaddr`

#### `mac get deveui`

Response: 8-byte hexadecimal number representing the device EUI. 

This command returns the globally unique end-device identifier, as set in the module.

Default: `0000000000000000`\
Example: `mac get deveui`

#### `mac get dnctr`

Response: decimal number representing the value of the downlink frame counter that
will be used for the next downlink reception, from 0 to 4294967295. 

This command will return the value of the downlink frame counter that will be used for the next downlink reception.

Default: `0`\
Example: `mac get dnctr`

#### `mac get dr`

Response: decimal number representing the current data rate.

This command will return the current data rate.

Default: `5`\
Example: `mac get dr`

#### `mac get dutycycletime`

Response: decimal number representing the time to wait in millisecond prior to issue a new transmission

This command returns the pending duty cycle counter value to wait before issuing a new transmission.

Example: `mac get dutycycletime`

#### `mac get edclass`

Response: string representing the current LoRaWAN operation class

This command will return the LoRaWAN operation class as set in the device.

Default: `CLASS A`\
Example: `mac get edclass`

#### `mac get edclasssupported`

Response: string representing the supported LoRaWAN operation class

This command will return the supported LoRaWAN operation class.

Default: `A&C`\
Example: `mac get edclasssupported`

#### `mac get gwnb`

Response: decimal number representing the number of gateways, from 0 to 255

This command will return the number of gateways that successfully received the last
Link Check Request frame command, as received in the last Link Check Answer.

Default: `0`\
Example: `mac get gwnb`

#### `mac get joinbackoffenable`

Response: string representing the state of the channel, either `on` or `off`. 

This command returns the Join backoff support.

Default: `on`\
Example: `mac get joinbackoffenable`

#### `mac get joindutycycletime`

Response: decimal number representing the time to wait in millisecond prior to issue a new join request

This command returns the pending duty cycle counter value to wait before issuing a new join request

Example: `mac get joindutycycletime`

#### `mac get joineui`

Response: 8-byte hexadecimal number representing the join/application EUI

This command will return the join/application identifier for the device. The application identifier is a value given to the device by the network.

Default: `0000000000000000`\
Example: `mac get joineui`

#### `mac get mcastenable <groupID>`

`<groupID>`: decimal number representing the group ID from 0 to 3

Response: string representing the Multicast state of the module, either `on` or `off`

This command will return the Multicast state as set in the device.

Default: `off`\
Example: `mac get mcast 0`

#### `mac get mcastdevaddr <groupID>`

`<groupID>`: decimal number representing the group ID from 0 to 3

Response: 4-byte hexadecimal number representing the device multicast address, from 00000000 to FFFFFFFF

This command will return the current multicast end-device address of the device.

Default: `ffffffff`\
Example: `mac get mcastdevaddr 0`

#### `mac get mcastdnctr <groupID>`

`<groupID>`: decimal number representing the group ID from 0 to 3

Response: decimal number representing the value of the downlink frame counter that will be used for the next multilink downlink reception, from 0 to 4294967295

This command will return the value of the downlink frame counter that will be used for
the next downlink reception.

Default: `0`\
Example: `mac get mcastdnctr 0`

#### `mac get mrgn`

Response: decimal number representing the demodulation margin, from 0 to 255

This command will return the demodulation margin as received in the last Link Check Answer frame.

Default: `255`\
Example: `mac get mrgn`

#### `mac get pktrssi`

Response: decimal number representing the RSSI of the lastest packet received

This command returns the RSSI of the latest packet received.

Example: `mac get pktrssi`

#### `mac get pwridx`

Response: decimal number representing the current output power index

This command returns the current output power index according to the region selected.

Default: `1` for EU868, `7` for NA915\
Example: `mac get pwridx`

#### `mac get reps`

Response: decimal number representing the number of repetition for the unconfirmed uplink messages, from 0 to 255

Default: `0`\
Example: `mac get reps`

#### `mac get retx`

Response: decimal number representing the number of retransmissions, from 0 to 255

This command will return the currently configured number of retransmissions which are
attempted for a confirmed uplink communication when no downlink response has been received.

Default: `7`\
Example: `mac get retx`

#### `mac get rx2`

Response: decimal number representing the data rate configured for the second
Receive window, from 0 to 7 and a decimal number for the frequency configured for the second Receive window

This command will return the current data rate and frequency configured to be used
during the second Receive window.

Example: `mac get rx2`

#### `mac get rxdelay1`

Response: decimal number representing the interval, in milliseconds, for `rxdelay1`, from 0 to 65535

This command will return the interval, in milliseconds, for `rxdelay1`.

Default: `1000`\
Example: `mac get rxdelay1`

#### `mac get rxdelay2`

Response: decimal number representing the interval, in milliseconds, for `rxdelay2`, from 0 to 65535

This command will return the interval, in milliseconds, for `rxdelay2`.

Default: `2000`\
Example: `mac get rxdelay2`

#### `mac get status`

Response: 4-byte hexadecimal number representing the current status of the stack

This command will return the current status of the stack. The value returned is a bit mask represented in hexadecimal form. 

Default: `00000000`\
Example: `mac get status`

MAC STATUS BIT-MAPPED REGISTER:

| Bit position | Description |
| ------------ | ----------- |
| 31-18 | RFU |
| 17 | Multicast status ('0' - multicast disabled, '1' - multicast enabled) |
| 16 | Rejoin needed ('0' - end device functional, '1' - end device not functional and rejoin is needed) |
| 15 | RX timing setup updated ('0' - not updated, '1' - updated via RX TimingSetupReq MAC command) |
| 14 | Second Receive window parameters updated ('0' - not updated, '1' - updated via RX ParamSetupReq MAC command) |
| 13 | Prescaler updated ('0' - not updated, '1' - updated via DutyCycleReq MAC command) |
| 12 | NbRep updated ('0' - not updated, '1' - updated via LinkADRReq MAC command) |
| 11 | Output power updated ('0' - not updated, '1' - updated via LinkADRReq MAC command) |
| 10 | Channels updated ('0' - not updated, '1' - updated via CFList or NewChannelReq MAC command) |
| 9 | Link check status ('0' - link check is disabled, '1' - link check is enabled) |
| 8 | Rx Done status ('0' - Rx data is not ready, '1' - Rx data is ready) |
| 7 | Mac pause status ('0' - mac is not paused, '1' - mac is paused) |
| 6 | Silent immediately status ('0' - disabled, '1' - enabled) |
| 5 | ADR status ('0' - disabled, '1' - enabled) |
| 4 | Automatic reply status ('0' - disabled, '1' - enabled) |
| 3 - 1 | Mac state - determine the state of transmission (rx window open, between tx and rx, etc.) |
| 0 | Join status ('0' - network not joined, '1' - network joined) |
<br> 

| Mac state value | Description |
| --------------- | ----------- |
| 0 | Idle (transmission are possible) |
| 1 | Transmission occuring |
| 2 | Before the opening of Receive window 1 |
| 3 | Receive window 1 is open |
| 4 | Between Receive window 1 and Receive window 2 |
| 5 | Receive window 2 is open |
| 6 | Retransmission delay - used for ADR_ACK delay, FSK can occur |
| 7 | APB_delay |

Example: 
```
mac get status
00000421 -> [0000 0000 0000 0000 0000 0100 0010 1001]b // CF list updated, ADR enabled, network joined
```

#### `mac get sync`

Response: one byte long hexadecimal number representing the synchronization word for the LoRaWAN communication.

This command will return the synchronization word for the LoRaWAN communication.

Default: `34`\
Example: `mac get sync`

#### `mac get upctr`

Response: decimal number representing the value of the uplink frame counter that will be used for the next uplink transmission, from 0 to 4294967295

This command will return the value of the uplink frame counter that will be used for the next uplink transmission.

Default: `0`\
Example: `mac get upctr`





## Limitations<a name="step5"></a>

The list of known limitations are described below:

1. `sys eraseFW` command is not implemented
1. `sys factoryRESET` has the same effect as `sys reset` command
1. `sys set nvm <address> <data>` command is not implemented
1. `sys set pindig <pinname> <pinstate>` is not implemented
1. `sys set pinmode <pinname> <pinmode>` is not implemented
1. `sys get pindig <pinname>` is not implemented
1. `sys get pinana <pinname>` is not implemented
1. `mac save` command is not implemented and redundant with PDS. In SAMR34 Microchip LoRaWAN Stack, Persistent Data Server (PDS) is implemented with task posting hooks and whenever it sees a change in persistence-enabled RAM paramters, then it will automatically saves them to Non-volatile memory.
When the device reboots or power is rebooted, after initialized the stack thru `mac reset <region>` command, it restores the persistent data from the non-volatile memory
1. `radio` commands are not supported here, check out the Radio Utility tool part of [SAM R34 Reference Design Package](https://www.microchip.com/wwwproducts/en/ATSAMR34J18) or [WLR089U0 Reference Design Package](https://www.microchip.com/wwwproducts/en/WLR089U0) to use radio commands

<a href="#top">Back to top</a>

