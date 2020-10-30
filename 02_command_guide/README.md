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

> This command must be issued at the very beginning in order to initialize internally the Microchip LoRaWAN Stack.

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
| joineui | Sets the join/application EUI key |
| linkchk | Sets the time interval for the link check process to be triggered |
| mcastenable | Sets the Multicast state to on, or off |
| mcastappskey | Sets the multicast application session key |
| mcastdevaddr | Sets the multicast network device address |
| mcastdnctr | Sets the value of the multicast downlink frame counter that will be used for the next multicast downlink reception |
| mcastnwkskey | Sets the multicast network session key |
| nwkskey | Sets the network session key |
| pwridx | Sets the output power to be used on the next transmissions |
| retx | Sets the number of retransmissions to be used for an uplink confirmed
packet |
| rx2 | Sets the data rate and frequency used for the second Receive window |
| rxdelay1 | Sets the value used for the first Receive window delay |
| sync | Sets the synchronization word for the LoRaWAN communication |
| upctr | Sets the value of the uplink frame counter that will be used for the next uplink transmission |

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

`<state>`: string value representing the state, either `on` or `off`.

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

### `mac set ch freq <channelID> <frequency>`

`<channelID>`: decimal number representing the channel number. (e.g. range from 3 to 15 for EU)\
`<frequency>`: decimal number representing the frequency in Hz.

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the operational frequency on the given channel ID. For EU region, the default channels (0-2) cannot be modified in terms of frequency.

Example: `mac set ch freq 13 864000000` // Define frequency for channel 13 to be 864 MHz

> Check out the file `lorawan_multiband.h` for regional parameters details related to the frequency band to operate.

### `mac set ch dcycle <channelID> <dutyCycle>`

`<channelID>`: decimal number representing the channel number (e.g. range from 0 to 15 for EU)\
`<dutyCycle>`: decimal number representing the duty cycle, from 0 to 65535

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the duty cycle used on the given channel ID on the device. The `<dutyCycle>` value that needs to be configured can be obtained from the actual duty cycle X (in percentage) using the following formula: `<dutyCycle>` = (100/X) – 1. For EU region, the default settings consider only the three default channels (0-2), and their default duty cycle is 0.33%. If a new channel is created either by the server or by the user, all the channels (including the default ones) must be updated by the user in terms of duty cycle to comply with the ETSI regulations.

Example: `mac set ch dcycle 13 9` // Defines duty cycle for channel 13 to be 10%. Since (100/10) – 1 = 9, the parameter that gets configured is 9.

> Check out the file `lorawan_multiband.h` for regional parameters details related to the frequency band to operate.

### `mac set ch drrange <channelID> <minRange> <maxRange>`

`<channelID>`: decimal number representing the channel\
`<minRange>`: decimal number representing the minimum data rate\
`<maxRange>`: decimal number representing the maximum data rate

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

Example: `mac set ch drrange 13 0 2` // Using EU863-870 band: on channel 13 the data rate can range from 0 (SF12/125 kHz) to 2 (SF10/125 kHz) as required

> Check out the files `lorawan_multiband.h` and `conf_regparams.h` for regional parameters details related to the frequency band to operate.

### `mac set ch status <channelID> <status>`

`<channelID>`: decimal number representing the channel\
`<status>`: string value representing the state, either `on` or `off`

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

Example: `mac set ch status 4 off` // channel ID 4 is disabled from use

> `<channelID>` parameters (frequency, data range, duty cycle) must be issued prior to enabling the status of that channel

### `mac set devaddr <address>`

`<address>`: 4-byte hexadecimal number representing the device address, from 00000000 to FFFFFFFF

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command configures the device with a 4-byte unique network device address `<address>`. The `<address>` MUST be UNIQUE to the current network. This must be
directly set solely for activation by personalization devices. This parameter must not be set before attempting to join using over-the-air activation because it will be overwritten once the join process is over.

Example: `mac set devaddr ABCDEF01`

### `mac set deveui <devEUI>`

`<devEUI>`: 8-byte hexadecimal number representing the device EUI

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the globally unique device identifier for the device. The identifier must be set by the host MCU. The module contains a pre-programmed unique EUI and
can be retrieved using the sys get hweui command or user
provided EUI can be configured using the `mac set deveui` command.

Example: `mac set deveui 0004A30B001A55ED`

### `mac set dnctr <fCntDown>`

`<fCntDown>`: decimal number representing the value of the downlink frame counter that will be used for the next downlink reception, from 0 to 4294967295.

Response: `ok` if parameter is valid\
Response: `invalid_param` if parameter is not valid

This command sets the value of the downlink frame counter that will be used for the next downlink reception.

Example: `mac set dnctr 30`

### `mac set dr <dataRate>`

`<dataRate>`: decimal number representing the data rate, but within the limits of the data rate range for the defined channels.

Response: `ok` if data rate is valid
Response: `invalid_param` if data rate is not valid

This command sets the data rate to be used for the next transmission.

Example: `mac set dr 5` // on EU863-870; SF7/125 kHz

> Check out the files `lorawan_multiband.h` and `conf_regparams.h` for regional parameters details related to the frequency band to operate.

### `mac set edclass <class>`

`<class>`: a letter representing the LoRaWAN device class, either `a` or `c`

Response: `ok` if parameters are valid
Response: `invalid_param` if parameters are not valid

This command sets the end device LoRaWAN operating class. The default end device class is Class A. When the class is configured as Class C, the end device will enter Class C continuous receive mode after the next uplink message is sent. The LoRaWAN network server must also configure this node as a Class C node. The network server configuration is performed out of band from LoRaWAN communications.

Example: `mac set edclass c`

### `mac set joineui <joinEUI>`

`<joinEUI>`: 8-byte hexadecimal number representing the join/application EUI

Response: `ok` if parameters are valid\
Response: `invalid_param` if parameters are not valid

This command sets the join/application EUI.

Example: `mac set joineui 0011223344556677`

### `mac set linkchk <linkcheck>`




## Limitations<a name="step5"></a>

The list of known limitations are described below:

1. `sys eraseFW` command is not implemented.
1. `sys factoryRESET` has the same effect as `sys reset` command.
1. `sys set nvm <address> <data>` command is not implemented.
1. `sys set pindig <pinname> <pinstate>` is not implemented.
1. `sys set pinmode <pinname> <pinmode>` is not implemented.
1. `sys get pindig <pinname>` is not implemented.
1. `sys get pinana <pinname>` is not implemented.
1. `mac save` command is not implemented and redundant with PDS. In SAMR34 Microchip LoRaWAN Stack, Persistent Data Server (PDS) is implemented with task posting hooks and whenever it sees a change in persistence-enabled RAM paramters, then it will automatically saves them to Non-volatile memory.
When the device reboots or power is rebooted, after initialized the stack thru `mac reset <region>` command, it restores the persistent data from the non-volatile memory
1. `radio` commands are not supported, check out the Radio Utility tool part of [SAM R34 Reference Design Package](https://www.microchip.com/wwwproducts/en/ATSAMR34J18) or [WLR089U0 Reference Design Package](https://www.microchip.com/wwwproducts/en/WLR089U0)

<a href="#top">Back to top</a>

