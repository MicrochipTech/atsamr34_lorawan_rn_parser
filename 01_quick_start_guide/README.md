# Quick Start Guide
> "Wireless Made Easy!" - Control SAM R34 IC or WLR089U0 Module with ASCII commands over UART

[Back to Main page](../README.md)

## A la carte

1. [Hardware Setup](#step1)
1. [Software Setup](#step2)
1. [Plug In](#step3)
1. [Link Up](#step4)
1. [Send Data](#step5)
1. [Standby mode](#step6)
1. [Secure Authentication](#step7)

## Hardware Setup<a name="step1"></a>

| Device       | RF Transceiver | Supported Evaluation Kit | Supported IDEs   |
| ------------ | -------------- | ------------------------ | ---------------- |
| SAMR34J18B   | SX1276 (in SIP)| SAMR34 XPRO              | Microchip Studio v7.0|
| WLR089U0     | SX1276 (in SIP)| WLR089U0 Module XPRO     | Microchip Studio v7.0|

- Development Board:
  - [SAM R34 Xplained Pro](https://www.microchip.com/DevelopmentTools/ProductDetails/dm320111)\
or
  - [WLR089U0 Module Xplained Pro](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/EV23M25A1)
- Micro-USB cable
- LoRaWAN Gateway (e.g. [The Things Industries](https://www.thethingsindustries.com/technology/hardware#gateway))

## Software Setup<a name="step2"></a>

- [Microchip Studio 7.0](https://www.microchip.com/mplab/microchip-studio)
- [TeraTerm - Terminal Emulator](https://osdn.net/projects/ttssh2/releases/)
- Network Server with Gateway and end device registered
- LoRaWAN Gateway connected to Network Server

> Notice that Network Servers supporting LoRaWAN 1.04 will only be compatible with this sample code which use Microchip LoRaWAN Stack MLS_SDK_1_0_P_5
(The Things Industries server supports LoRaWAN 1.0.4).

> In [LoRaWAN 1.0.4 Specification](https://lora-alliance.org/resource-hub/lorawan-104-specification-package), `DevNonce` is an incrementing 16-bit value. Make sure DevNonce values are aligned between end device and network/join server to pass successfully the join procedure. `DevNonce` is incremented on each join request tentative from the end device and the value is tracked by the server. `DevNonce` is stored in Persistent Data Storage in the Microchip LoRaWAN Stack.

<p align="center">
<img src="resources/media/devnonce.png" width=320>
</p>

## Plug In<a name="step3"></a>

- Connect your board (EDBG USB) to your computer with a micro USB cable. Your personal computer will recognize the board as a Virtual COM port.
- Open the virtual COM port in a terminal emulator

## Link Up<a name="step4"></a>

Connect to Network Server using pre-programmed provisioning keys

1. Open TeraTerm. Choose the EDBG Virtual COM port and enable local echo
2. The default settings for the UART interface are 115200 bps, 8-bit, no parity, one stop bit and no flow control (115200 8 N 1)
3. Press **RESET** button to reboot the board
```sh
Last reset cause: External Reset
LoRaWAN Stack UP
USER BOARD MLS_SDK_1_0_P_5 Oct 29 2020 16:10:55
```
4. Initialize the LoRaWAN stack using `mac reset <band>` command. Check out the [MAC Command User's Guide](../02_command_guide/README.md#step4) for more details.
```sh
mac reset 868
ok
```
5. Provision the device for OTAA with your own keys
```sh
mac set deveui 000425191801d748
mac set joineui 0004251918010000
mac set appkey 000425191801d748000425191801d748
 ```
6. Join the network
```sh
mac join otaa
ok
accepted
```
<p align="center">
<img src="resources/media/tti_join.png" width=720>
</p>

## Send Data<a name="step5"></a>

1. Transmit unconfirmed message on port 1
```sh
mac tx uncnf 1 AABBCCDDEEFF
ok
mac_tx_ok
```
<p align="center">
<img src="resources/media/tti_send_uncnf.png" width=720>
</p>

2. Transmit confirmed message on port 1
```sh
mac tx cnf 1 0011223344556688
ok
mac_tx_ok
```
<p align="center">
<img src="resources/media/tti_send_cnf.png" width=720>
</p>

## Standby mode<a name="step6"></a>

1. Place jumper J102 in **MEAS** position to measure MCU current
1. Place jumper J101 in **BY-PS** position to bypass I/Os current
1. Initialize the stack with `mac reset <band>` command
1. Put the device in standby mode for 60 seconds

Example:
```sh
mac reset 868
sys sleep standby 60000
..
sleep_ok 59991 ms

```

<p align="center">
<img src="resources/media/sys_sleep_standby_wlr089u0.png" width=720>
</p>

## Secure Authentication<a name="step7"></a>

To enable secure LoRaWAN Authentication, check out the links below for guidance on Hardware Setup and instructions to claim and activate end device to the Join Server:
- [Adding ATECC608-TNGLORA (TTI) Secure Element to SAM R34 IC and WLR089U0 Module](https://github.com/MicrochipTech/atsamr34_ecc608a_tti)
- [Adding ATECC608-TNGACT (Actility) Secure Element to SAM R34 IC and WLR089U0 Module](https://github.com/MicrochipTech/atsamr34_ecc608a_actility)


1. Initialize the LoRaWAN stack
```sh
mac reset 868
ok
```
2. Enable crypto device
```sh
mac set cryptodevenabled on
ok
```
3. Print crypto device info
```sh
// print crypto serial number
sys get cryptosn
01236e0a6ded6e6827

// print crypto dev eui
sys get cryptodeveui
0004A310001FFA2B

// print crypto join eui
sys get cryptojoineui
70b3d57ed0000000

// print crypto tkm info (used for Actility)
sys get cryptotkminfo
aa2301236e0a6ded6e68
```
4. Join the network
```sh
mac join otaa
ok
accepted
```
5. Transmit unconfirmed message on port 1
```
mac tx uncnf 1 AABBCCDDEEFF
ok
mac_tx_ok
```

> Secure element is loaded with unique EUI-64 which is stored in `DEV_EUI_SLOT` memory.
By default in the code, `DEV_EUI_SLOT` is used as deveui key. If required this settings can be changed in `conf_sal.h`.

`#define SERIAL_NUM_AS_DEV_EUI    0 // Value is 1 if ECC608 Serial number is used as DEV_EUI otherwise DEV_EUI will be read from DEV_EUI_SLOT`

<a href="#top">Back to top</a>

