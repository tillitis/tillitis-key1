// SPDX-FileCopyrightText: 2017 WCH <wch-ic.com>
// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

/********************************** (C) COPYRIGHT *******************************
 * File Name          : CDC.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2017/03/01
 * Description        : CH554 as CDC device to serial port, select serial port 1
 *******************************************************************************/
#include <stdint.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>

#include "config.h"
#include "debug.h"
#include "flash.h"
#include "io.h"
#include "lib.h"
#include "mem.h"
#include "print.h"
#include "usb_strings.h"

XDATA AT0000 uint8_t Ep0Buffer[3*MAX_PACKET_SIZE] = { 0 }; // Endpoint 0, Default endpoint, OUT & IN buffer[64], must be an even address +
                                                           // Endpoint 4, DEBUG endpoint, buffer OUT[64]+IN[64], must be an even address
XDATA AT00C0 uint8_t Ep1Buffer[DEFAULT_EP1_SIZE]  = { 0 }; // Endpoint 1, CDC Ctrl endpoint, IN[8] buffer
XDATA AT00C8 uint8_t Ep2Buffer[2*MAX_PACKET_SIZE] = { 0 }; // Endpoint 2, CDC Data endpoint, buffer OUT[64]+IN[64], must be an even address
XDATA AT0148 uint8_t Ep3Buffer[2*MAX_PACKET_SIZE] = { 0 }; // Endpoint 3, FIDO endpoint, buffer OUT[64]+IN[64], must be an even address

uint16_t SetupLen = 0;
uint8_t SetupReq = 0;
uint8_t UsbConfig = 0;
const uint8_t *pDescr = NULL;         // USB configuration flag

#define UsbSetupBuf                   ((PUSB_SETUP_REQ)Ep0Buffer)

#define CDC_CTRL_EPOUT_ADDR            0x01              // CDC Ctrl Endpoint OUT Address
#define CDC_CTRL_EPOUT_SIZE            DEFAULT_EP1_SIZE  // CDC Ctrl Endpoint OUT Size

#define CDC_CTRL_EPIN_ADDR             0x81              // CDC Ctrl Endpoint IN Address
#define CDC_CTRL_EPIN_SIZE             DEFAULT_EP1_SIZE  // CDC Ctrl Endpoint IN Size

#define CDC_DATA_EPOUT_ADDR            0x02              // CDC Data Endpoint OUT Address
#define CDC_DATA_EPOUT_SIZE            MAX_PACKET_SIZE   // CDC Data Endpoint OUT Size

#define CDC_DATA_EPIN_ADDR             0x82              // CDC Data Endpoint IN Address
#define CDC_DATA_EPIN_SIZE             MAX_PACKET_SIZE   // CDC Data Endpoint IN Size

#define FIDO_EPOUT_ADDR                0x03              // FIDO Endpoint OUT Address
#define FIDO_EPOUT_SIZE                MAX_PACKET_SIZE   // FIDO Endpoint OUT Size

#define FIDO_EPIN_ADDR                 0x83              // FIDO Endpoint IN Address
#define FIDO_EPIN_SIZE                 MAX_PACKET_SIZE   // FIDO Endpoint IN Size

#define CCID_BULK_EPOUT_ADDR           0x03              // CCID Bulk Endpoint OUT Address
#define CCID_BULK_EPOUT_SIZE           MAX_PACKET_SIZE   // CCID Bulk Endpoint OUT Size

#define CCID_BULK_EPIN_ADDR            0x83              // CCID Bulk Endpoint IN Address
#define CCID_BULK_EPIN_SIZE            MAX_PACKET_SIZE   // CCID Bulk Endpoint IN Size

#define DEBUG_EPOUT_ADDR               0x04              // DEBUG Endpoint OUT Address
#define DEBUG_EPOUT_SIZE               MAX_PACKET_SIZE   // DEBUG Endpoint OUT Size

#define DEBUG_EPIN_ADDR                0x84              // DEBUG Endpoint IN Address
#define DEBUG_EPIN_SIZE                MAX_PACKET_SIZE   // DEBUG Endpoint IN Size

#define CDC_CTRL_FS_BINTERVAL          32                // Gives 32 ms polling interval at Full Speed for interrupt transfers
#define CDC_DATA_FS_BINTERVAL          0                 // bInterval is ignored for BULK transfers
#define FIDO_FS_BINTERVAL              2                 // Gives 2 ms polling interval at Full Speed for interrupt transfers
#define CCID_BULK_FS_BINTERVAL         0                 // bInterval is ignored for BULK transfers
#define DEBUG_FS_BINTERVAL             2                 // Gives 2 ms polling interval at Full Speed for interrupt transfers

#define MAX_CFG_DESC_SIZE              (9+66+77+32)      // Size of CfgDesc+CdcDesc+MAX(FidoDesc,CcidDesc)+DebugDesc

#define NUM_INTERFACES                 4                 // Number of interfaces

#define CHANGE_ME                      0x00              // Value placeholder

#define FIDO_REPORT_DESC_SIZE          47                // Size of FidoReportDesc
#define DEBUG_REPORT_DESC_SIZE         34                // Size of DebugReportDesc

#define CCID_VALUE_BCDCCID                 0x0110
#define CCID_VALUE_DWPROTOCOLS             0x00000002
#define CCID_VALUE_DWDEFAULTCLOCK          3580
#define CCID_VALUE_DWMAXIUMUMCLOCK         3580
#define CCID_VALUE_DWDATARATE              9600
#define CCID_VALUE_DWMAXDATARATE           9600
#define CCID_VALUE_DWMAXIFSD               254
#define CCID_VALUE_DWSYNCPROTOCOLS         0x0
#define CCID_VALUE_DWMECHANICAL            0x0
#define CCID_VALUE_DWFEATURES              0x000400FE
#define CCID_VALUE_DWMAXCCIDMESSAGELENGTH  3072
#define CCID_VALUE_WLCDLAYOUT              0x0

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define LOBYTE(x)  ((uint8_t)( (x)        & 0x00FFU))
#define HIBYTE(x)  ((uint8_t)(((x) >> 8U) & 0x00FFU))

#define BYTE0(x)   ((uint8_t)( (x)        & 0x000000FFU))  // Least significant byte
#define BYTE1(x)   ((uint8_t)(((x) >>  8) & 0x000000FFU))  // Second byte
#define BYTE2(x)   ((uint8_t)(((x) >> 16) & 0x000000FFU))  // Third byte
#define BYTE3(x)   ((uint8_t)(((x) >> 24) & 0x000000FFU))  // Most significant byte

uint8_t FidoInterfaceNum = 0;
uint8_t CcidInterfaceNum = 0;
uint8_t DebugInterfaceNum = 0;

XDATA uint8_t ActiveCfgDesc[MAX_CFG_DESC_SIZE];
XDATA uint8_t ActiveCfgDescSize = 0;

// Device Descriptor
FLASH uint8_t DevDesc[] = {
        0x12,                             /* bLength */
        USB_DESC_TYPE_DEVICE,             /* bDescriptorType: Device */
        0x00,                             /* bcdUSB (low byte), USB Specification Release Number in Binary-Coded Decimal (2.0 is 200h) */
        0x02,                             /* bcdUSB (high byte) USB Specification Release Number in Binary-Coded Decimal (2.0 is 200h) */
        USB_DEV_CLASS_MISCELLANEOUS,      /* bDeviceClass: Miscellaneous Device Class (Composite) */
        0x02,                             /* bDeviceSubClass: Common Class */
        0x01,                             /* bDeviceProtocol: IAD (Interface Association Descriptor) */
        DEFAULT_EP0_SIZE,                 /* bMaxPacketSize */
        0x09,                             /* idVendor */            // VID LOBYTE
        0x12,                             /* idVendor */            // VID HIBYTE
        0x85,                             /* idProduct */           // PID LOBYTE
        0x88,                             /* idProduct */           // PID HIBYTE
        0x00,                             /* bcdDevice (device release number in binary-coded decimal (BCD) format, low byte, i.e. YY) rel. XX.YY */
        0x01,                             /* bcdDevice (device release number in binary-coded decimal (BCD) format, high byte, i.e. XX) rel. XX.YY */
        USB_IDX_MFC_STR,                  /* Index of manufacturer string */
        USB_IDX_PRODUCT_STR,              /* Index of product string */
        USB_IDX_SERIAL_STR,               /* Index of serial number string */
        0x01,                             /* bNumConfigurations */
        /* 18 */
};

// Configuration Descriptor
FLASH uint8_t CfgDesc[] = {
        /******************** Configuration Descriptor ********************/
        0x09,                             /* bLength: Configuration Descriptor size */
        USB_DESC_TYPE_CONFIGURATION,      /* bDescriptorType: Configuration */
        CHANGE_ME,                        /* wTotalLength (low byte): Bytes returned */
        0x00,                             /* wTotalLength (high byte): Bytes returned */
        CHANGE_ME,                        /* bNumInterfaces: 1 CDC Ctrl + 1 CDC Data, 1 FIDO or 1 CCID, 1 DEBUG ) */
        0x01,                             /* bConfigurationValue: Configuration value */
        0x00,                             /* iConfiguration: Index of string descriptor describing the configuration */
        0xA0,                             /* bmAttributes: Bus powered and Support Remote Wake-up */
        0x32,                             /* MaxPower 100 mA: this current is used for detecting Vbus */
        /* 9 */
};

// CDC Descriptor
FLASH uint8_t CdcDesc[] = {
        /******************** IAD (Interface Association Descriptor), should be positioned just before the CDC interfaces ********************/
        /******************** This is to associate the two CDC interfaces with the CDC class ********************/
        0x08,                                /* bLength: IAD Descriptor size */
        USB_DESC_TYPE_INTERFACE_ASSOCIATION, /* bDescriptorType: Interface Association */
        0x00,                                /* bFirstInterface: 0 */
        0x02,                                /* bInterfaceCount: 2 */
        USB_DEV_CLASS_CDC_CONTROL,           /* bFunctionClass: Communications & CDC Control */
        0x02,                                /* bFunctionSubClass: Abstract Control Model */
        0x01,                                /* bFunctionProtocol: Common AT commands */
        0x00,                                /* iFunction: Index of string descriptor */
        /******************** Interface, CDC Ctrl Descriptor (one endpoint) ********************/
        /* 8 */
        0x09,                             /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,          /* bDescriptorType: Interface */
        CHANGE_ME,                        /* bInterfaceNumber: Number of Interface */
        0x00,                             /* bAlternateSetting: Alternate setting */
        0x01,                             /* bNumEndpoints: Number of endpoints in Interface */
        USB_DEV_CLASS_CDC_CONTROL,        /* bInterfaceClass: Communications and CDC Control */
        0x02,                             /* bInterfaceSubClass : Abstract Control Model */
        0x01,                             /* bInterfaceProtocol : AT Commands: V.250 etc */
        USB_IDX_INTERFACE_CDC_CTRL_STR,   /* iInterface: Index of string descriptor */
        /******************** Header Functional Descriptor ********************/
        /* 17 */
        0x05,                             /* bFunctionLength: Size of this descriptor in bytes */
        USB_DESC_TYPE_CS_INTERFACE,       /* bDescriptorType: Class-Specific Interface */
        0x00,                             /* bDescriptorSubtype: Header Functional Descriptor */
        0x10,                             /* bcdCDC (low byte): CDC version 1.10 */
        0x01,                             /* bcdCDC (high byte): CDC version 1.10 */
        /******************** Call Management Functional Descriptor (no data interface, bmCapabilities=03, bDataInterface=01) ********************/
        /* 22 */
        0x05,                             /* bFunctionLength: Size of this descriptor */
        USB_DESC_TYPE_CS_INTERFACE,       /* bDescriptorType: Class-Specific Interface */
        0x01,                             /* bDescriptorSubtype: Call Management Functional Descriptor */
        0x00,                             /* bmCapabilities:
                                             D7..2: 0x00 (RESERVED,
                                             D1   : 0x00 (0 - Device sends/receives call management information only over the Communications Class interface
                                                          1 - Device can send/receive call management information over a Data Class interface)
                                             D0   : 0x00 (0 - Device does not handle call management itself
                                                          1 - Device handles call management itself) */
        0x00,                             /* bDataInterface: Interface number of Data Class interface optionally used for call management */
        /******************** Abstract Control Management Functional Descriptor ********************/
        /* 27 */
        0x04,                             /* bLength */
        USB_DESC_TYPE_CS_INTERFACE,       /* bDescriptorType: Class-Specific Interface */
        0x02,                             /* bDescriptorSubtype: Abstract Control Management Functional Descriptor */
        0x02,                             /* bmCapabilities:
                                             D7..4: 0x00 (RESERVED, Reset to zero)
                                             D3   : 0x00 (1 - Device supports the notification Network_Connection)
                                             D2   : 0x00 (1 - Device supports the request Send_Break)
                                             D1   : 0x01 (1 - Device supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State)
                                             D0   : 0x00 (1 - Device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature) */
        /******************** Union Functional Descriptor. CDC Ctrl interface numbered 0; CDC Data interface numbered 1 ********************/
        /* 31 */
        0x05,                             /* bLength */
        USB_DESC_TYPE_CS_INTERFACE,       /* bDescriptorType: Class-Specific Interface */
        0x06,                             /* bDescriptorSubtype: Union Functional Descriptor */
        0x00,                             /* bControlInterface: Interface number 0 (Control interface) */
        0x01,                             /* bSubordinateInterface0: Interface number 1 (Data interface) */
        /******************** CDC Ctrl Endpoint descriptor (IN) ********************/
        /* 36 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        CDC_CTRL_EPIN_ADDR,               /* bEndpointAddress: Endpoint Address (IN) */
        USB_EP_TYPE_INTERRUPT,            /* bmAttributes: Interrupt Endpoint */
        LOBYTE(CDC_CTRL_EPIN_SIZE),       /* wMaxPacketSize (low byte): 8 Byte max */
        HIBYTE(CDC_CTRL_EPIN_SIZE),       /* wMaxPacketSize (high byte): 8 Byte max */
        CDC_CTRL_FS_BINTERVAL,            /* bInterval: Polling Interval */
        /******************** Interface, CDC Data Descriptor (two endpoints) ********************/
        /* 43 */
        0x09,                             /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,          /* bDescriptorType: Interface */
        CHANGE_ME,                        /* bInterfaceNumber: Number of Interface */
        0x00,                             /* bAlternateSetting: Alternate setting */
        0x02,                             /* bNumEndpoints: Number of endpoints in Interface */
        USB_DEV_CLASS_CDC_DATA,           /* bInterfaceClass: CDC Data */
        0x00,                             /* bInterfaceSubClass : 1=BOOT, 0=no boot */
        0x00,                             /* bInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
        USB_IDX_INTERFACE_CDC_DATA_STR,   /* iInterface: Index of string descriptor */
        /******************** CDC Data Endpoint descriptor (OUT) ********************/
        /* 52 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        CDC_DATA_EPOUT_ADDR,              /* bEndpointAddress: Endpoint Address (OUT) */
        USB_EP_TYPE_BULK,                 /* bmAttributes: Bulk Endpoint */
        LOBYTE(CDC_DATA_EPOUT_SIZE),      /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(CDC_DATA_EPOUT_SIZE),      /* wMaxPacketSize (high byte): 64 Byte max */
        CDC_DATA_FS_BINTERVAL,            /* bInterval: Polling Interval */
        /******************** CDC Data Endpoint descriptor (IN) ********************/
        /* 59 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        CDC_DATA_EPIN_ADDR,               /* bEndpointAddress: Endpoint Address (IN) */
        USB_EP_TYPE_BULK,                 /* bmAttributes: Bulk Endpoint */
        LOBYTE(CDC_DATA_EPIN_SIZE),       /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(CDC_DATA_EPIN_SIZE),       /* wMaxPacketSize (high byte): 64 Byte max */
        CDC_DATA_FS_BINTERVAL,            /* bInterval: Polling Interval */
        /* 66 */
};

// FIDO Descriptor
FLASH uint8_t FidoDesc[] = {
        /******************** Interface, FIDO Descriptor (two endpoints) ********************/
        0x09,                             /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,          /* bDescriptorType: Interface */
        CHANGE_ME,                        /* bInterfaceNumber: Number of Interface */
        0x00,                             /* bAlternateSetting: Alternate setting */
        0x02,                             /* bNumEndpoints: Number of endpoints in Interface */
        USB_DEV_CLASS_HID,                /* bInterfaceClass: Human Interface Device */
        0x00,                             /* bInterfaceSubClass : 1=BOOT, 0=no boot */
        0x00,                             /* bInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
        USB_IDX_INTERFACE_FIDO_STR,       /* iInterface: Index of string descriptor */
        /******************** FIDO Device Descriptor ********************/
        /* 9 */
        0x09,                             /* bLength: HID Descriptor size */
        USB_DESC_TYPE_HID,                /* bDescriptorType: HID */
        0x11,                             /* bcdHID (low byte): HID Class Spec release number */
        0x01,                             /* bcdHID (high byte): HID Class Spec release number */
        0x00,                             /* bCountryCode: Hardware target country */
        0x01,                             /* bNumDescriptors: Number of HID class descriptors to follow */
        USB_DESC_TYPE_REPORT,             /* bDescriptorType: Report */
        LOBYTE(FIDO_REPORT_DESC_SIZE),    /* wDescriptorLength (low byte): Total length of Report descriptor */
        HIBYTE(FIDO_REPORT_DESC_SIZE),    /* wDescriptorLength (high byte): Total length of Report descriptor */
        /******************** FIDO Endpoint Descriptor (OUT) ********************/
        /* 18 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        FIDO_EPOUT_ADDR,                  /* bEndpointAddress: Endpoint Address (OUT) */
        USB_EP_TYPE_INTERRUPT,            /* bmAttributes: Interrupt endpoint */
        LOBYTE(FIDO_EPOUT_SIZE),          /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(FIDO_EPOUT_SIZE),          /* wMaxPacketSize (high byte): 64 Byte max */
        FIDO_FS_BINTERVAL,                /* bInterval: Polling Interval */
        /******************** FIDO Endpoint Descriptor (IN) ********************/
        /* 25 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        FIDO_EPIN_ADDR,                   /* bEndpointAddress: Endpoint Address (IN) */
        USB_EP_TYPE_INTERRUPT,            /* bmAttributes: Interrupt endpoint */
        LOBYTE(FIDO_EPIN_SIZE),           /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(FIDO_EPIN_SIZE),           /* wMaxPacketSize (high byte): 64 Byte max */
        FIDO_FS_BINTERVAL,                /* bInterval: Polling Interval */
        /* 32 */
};

// CCID Descriptor
FLASH uint8_t CcidDesc[] = {
        /******************** Interface, CCID Descriptor (two endpoints) ********************/
        0x09,                             /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,          /* bDescriptorType: Interface */
        CHANGE_ME,                        /* bInterfaceNumber: Number of Interface */
        0x00,                             /* bAlternateSetting: Alternate setting */
        0x02,                             /* bNumEndpoints: Number of endpoints in Interface */
        USB_DEV_CLASS_SMART_CARD,         /* bInterfaceClass: Smart Card */
        0x00,                             /* bInterfaceSubClass : Subclass code */
        0x00,                             /* bInterfaceProtocol : For Integrated Circuit(s) Cards Interface Devices (CCID): 00h
                                             Note: For competitiveness, values 01h and 02h are reserved for Integrated Circuit(s)
                                             Cards Devices (USB-ICC) and other values are reserved for future use. */
        USB_IDX_INTERFACE_CCID_STR,       /* iInterface: Index of string descriptor */
        /******************** CCID Device Descriptor ********************/
        /* 9 */
        0x36,                             /* bLength: Size of this descriptor in bytes */
        USB_DESC_TYPE_HID,                /* bDescriptorType: HID */
        LOBYTE(CCID_VALUE_BCDCCID),       /* bcdCCID (low byte)*/
        HIBYTE(CCID_VALUE_BCDCCID),       /* bcdCCID (high byte): 0x0110
                                             CCID Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 0210h) */
        0x00,                             /* bMaxSlotIndex: 0
                                             The index of the highest available slot on this device.
                                             All slots are consecutive starting at 00h.
                                             i.e. 0Fh = 16 slots on this device numbered 00h to 0Fh. */
        0x07,                             /* bVoltageSupport: 7
                                             This value indicates what voltages the CCID can supply to its slots.
                                             It is a bitwise OR operation performed on the following values:
                                             - 01h 5.0V
                                             - 02h 3.0V
                                             - 04h 1.8V
                                             Other bits are RFU */
        BYTE0(CCID_VALUE_DWPROTOCOLS),    /* dwProtocols */
        BYTE1(CCID_VALUE_DWPROTOCOLS),    /* dwProtocols */
        BYTE2(CCID_VALUE_DWPROTOCOLS),    /* dwProtocols */
        BYTE3(CCID_VALUE_DWPROTOCOLS),    /* dwProtocols: T=1
                                             RRRR –Upper Word- is RFU = 0000h
                                             PPPP –Lower Word- Encodes the supported protocol types.
                                             A ‘1’ in a given bit position indicates support for the associated ISO protocol.
                                             0001h = Protocol T=0
                                             0002h = Protocol T=1
                                             All other bits are reserved and must be set to zero.
                                             The field is intended to correspond to the PCSC specification definitions.
                                             See PCSC Part3. Table 3-1 Tag 0x0120.
                                             Example: 00000003h indicates support for T = 0 and T = 1.  */
        BYTE0(CCID_VALUE_DWDEFAULTCLOCK), /* dwDefaultClock */
        BYTE1(CCID_VALUE_DWDEFAULTCLOCK), /* dwDefaultClock */
        BYTE2(CCID_VALUE_DWDEFAULTCLOCK), /* dwDefaultClock */
        BYTE3(CCID_VALUE_DWDEFAULTCLOCK), /* dwDefaultClock: 3580 KHz
                                             Default ICC clock frequency in KHz. This is an integer value.
                                             Example: 3.58 MHz is encoded as the integer value 3580. (00000DFCh)
                                             This is used in ETU and waiting time calculations.
                                             It is the clock frequency used when reading the ATR data. */
        BYTE0(CCID_VALUE_DWMAXIUMUMCLOCK),/* dwMaximumClock */
        BYTE1(CCID_VALUE_DWMAXIUMUMCLOCK),/* dwMaximumClock */
        BYTE2(CCID_VALUE_DWMAXIUMUMCLOCK),/* dwMaximumClock */
        BYTE3(CCID_VALUE_DWMAXIUMUMCLOCK),/* dwMaximumClock: 3580 KHz
                                             Maximum supported ICC clock frequency in KHz. This is an integer value.
                                             Example: 14.32 MHz is encoded as the integer value 14320. (000037F0h) */
        0x00,                             /* bNumClockSupported: 0
                                             The number of clock frequencies that are supported by the CCID.
                                             If the value is 00h, the supported clock frequencies are assumed to be the default clock
                                             frequency defined by dwDefaultClock and the maximum clock frequency defined by dwMaximumClock.
                                             The reader must implement the command PC_to_RDR_SetDataRateAndClockFrequency if more than one
                                             clock frequency is supported. */
        BYTE0(CCID_VALUE_DWDATARATE),     /* dwDataRate */
        BYTE1(CCID_VALUE_DWDATARATE),     /* dwDataRate */
        BYTE2(CCID_VALUE_DWDATARATE),     /* dwDataRate */
        BYTE3(CCID_VALUE_DWDATARATE),     /* dwDataRate: 9600 bps
                                             Default ICC I/O data rate in bps. This is an integer value.
                                             Example: 9600 bps is encoded as the integer value 9600. (00002580h) */
        BYTE0(CCID_VALUE_DWMAXDATARATE),  /* dwMaxDataRate */
        BYTE1(CCID_VALUE_DWMAXDATARATE),  /* dwMaxDataRate */
        BYTE2(CCID_VALUE_DWMAXDATARATE),  /* dwMaxDataRate */
        BYTE3(CCID_VALUE_DWMAXDATARATE),  /* dwMaxDataRate: 9600 bps
                                             Maximum supported ICC I/O data rate in bps.
                                             Example: 115.2Kbps is encoded as the integer value 115200. (0001C200h) */
        0x00,                             /* bNumDataRatesSupported: 0
                                             The number of data rates that are supported by the CCID.
                                             If the value is 00h, all data rates between the default data rate dwDataRate and the
                                             maximum data rate dwMaxDataRate are supported. */
        BYTE0(CCID_VALUE_DWMAXIFSD),      /* dwMaxIFSD */
        BYTE1(CCID_VALUE_DWMAXIFSD),      /* dwMaxIFSD */
        BYTE2(CCID_VALUE_DWMAXIFSD),      /* dwMaxIFSD */
        BYTE3(CCID_VALUE_DWMAXIFSD),      /* dwMaxIFSD: 254
                                             Indicates the maximum IFSD (Information Field Size for Device) supported by CCID for protocol T=1. */
        BYTE0(CCID_VALUE_DWSYNCPROTOCOLS),/* dwSynchProtocols */
        BYTE1(CCID_VALUE_DWSYNCPROTOCOLS),/* dwSynchProtocols */
        BYTE2(CCID_VALUE_DWSYNCPROTOCOLS),/* dwSynchProtocols */
        BYTE3(CCID_VALUE_DWSYNCPROTOCOLS),/* dwSynchProtocols: 0
                                             RRRR-Upper Word- is RFU = 0000h
                                             PPPP-Lower Word- encodes the supported protocol types.
                                             A ‘1’ in a given bit position indicates support for the associated protocol.
                                             0001h indicates support for the 2-wire protocol
                                             0002h indicates support for the 3-wire protocol
                                             0004h indicates support for the I2C protocol
                                             All other values are outside of this specification, and must be handled by vendor-supplied drivers. */
        BYTE0(CCID_VALUE_DWMECHANICAL),   /* dwMechanical */
        BYTE1(CCID_VALUE_DWMECHANICAL),   /* dwMechanical */
        BYTE2(CCID_VALUE_DWMECHANICAL),   /* dwMechanical */
        BYTE3(CCID_VALUE_DWMECHANICAL),   /* dwMechanical: 0
                                             The value is a bitwise OR operation performed on the following values:
                                             - 00000000h No special characteristics
                                             - 00000001h Card accept mechanism
                                             - 00000002h Card ejection mechanism
                                             - 00000004h Card capture mechanism
                                             - 00000008h Card lock/unlock mechanism */
        BYTE0(CCID_VALUE_DWFEATURES),     /* dwFeatures */
        BYTE1(CCID_VALUE_DWFEATURES),     /* dwFeatures */
        BYTE2(CCID_VALUE_DWFEATURES),     /* dwFeatures */
        BYTE3(CCID_VALUE_DWFEATURES),     /* dwFeatures: 0x000400FE
                                                         Automatic parameter configuration based on ATR data
                                                         Automatic activation of ICC on inserting
                                                         Automatic ICC voltage selection
                                                         Automatic ICC clock frequency change according to active parameters provided by the Host or self determined
                                                         Automatic baud rate change according to active parameters provided by the Host or self determined
                                                         Automatic parameters negotiation made by the CCID
                                                         Automatic PPS made by the CCID according to the active parameters
                                                         Short and Extended APDU level exchange with CCID
                                             This value indicates what intelligent features the CCID has.
                                             The value is a bitwise OR operation performed on the following values:
                                             - 00000000h No special characteristics
                                             - 00000002h Automatic parameter configuration based on ATR data
                                             - 00000004h Automatic activation of ICC on inserting
                                             - 00000008h Automatic ICC voltage selection
                                             - 00000010h Automatic ICC clock frequency change according to active parameters provided by the Host or self determined
                                             - 00000020h Automatic baud rate change according to active parameters provided by the Host or self determined
                                             - 00000040h Automatic parameters negotiation made by the CCID (use of warm or cold resets or PPS according to a
                                                         manufacturer proprietary algorithm to select the communication parameters with the ICC)
                                             - 00000080h Automatic PPS made by the CCID according to the active parameters
                                             - 00000100h CCID can set ICC in clock stop mode
                                             - 00000200h NAD value other than 00 accepted (T=1 protocol in use)
                                             - 00000400h Automatic IFSD exchange as first exchange (T=1 protocol in use)
                                             Only one of the following values may be present to select a level of exchange:
                                             - 00010000h TPDU level exchanges with CCID
                                             - 00020000h Short APDU level exchange with CCID
                                             - 00040000h Short and Extended APDU level exchange with CCID
                                             - If none of those values is indicated the level of exchange is character.
                                             Only one of the values 00000040h and 00000080h may be present.
                                             When value 00000040h is present the host shall not try to change the FI, DI, and protocol currently selected.
                                             When an APDU level for exchanges is selected, one of the values 00000040h or 00000080h must be present, as well as the
                                             value 00000002h.
                                             To support selective suspend:
                                             - 00100000h USB Wake up signaling supported on card insertion and removal
                                             When bit 20th, as shown above, is set bit D5 in bmAttributes of the Standard Configuration Descriptor must be set to 1. */
        BYTE0(CCID_VALUE_DWMAXCCIDMESSAGELENGTH),/* dwMaxCCIDMessageLength */
        BYTE1(CCID_VALUE_DWMAXCCIDMESSAGELENGTH),/* dwMaxCCIDMessageLength */
        BYTE2(CCID_VALUE_DWMAXCCIDMESSAGELENGTH),/* dwMaxCCIDMessageLength */
        BYTE3(CCID_VALUE_DWMAXCCIDMESSAGELENGTH),/* dwMaxCCIDMessageLength: 3072
                                                    For extended APDU level the value shall be between 261 + 10 (header) and 65544 +10,
                                                    otherwise the minimum value is the wMaxPacketSize of the Bulk-OUT endpoint. */
        0xFF,                             /* bClassGetResponse: echo
                                             Significant only for CCID that offers an APDU level for exchanges.
                                             Indicates the default class value used by the CCID when it sends a Get Response command to perform the transportation of an APDU by T=0 protocol.
                                             Value FFh indicates that the CCID echoes the class of the APDU. */
        0xFF,                             /* bClassEnvelope: echo
                                             Significant only for CCID that offers an extended APDU level for exchanges.
                                             Indicates the default class value used by the CCID when it sends an Envelope command to perform the transportation of an extended APDU by T=0 protocol.
                                             Value FFh indicates that the CCID echoes the class of the APDU.*/
        LOBYTE(CCID_VALUE_WLCDLAYOUT),    /* wLcdLayout */
        HIBYTE(CCID_VALUE_WLCDLAYOUT),    /* wLcdLayout: none
                                             Number of lines and characters for the LCD display used to send messages for PIN entry.
                                             XX: number of lines
                                             YY: number of characters per line.
                                             XXYY=0000h no LCD. */
        0x00,                             /* bPINSupport: 0
                                             This value indicates what PIN support features the CCID has.
                                             The value is a bitwise OR operation performed on the following values:
                                             0x01 PIN Verification supported
                                             0x02 PIN Modification supported */
        0x01,                             /* bMaxCCIDBusySlots: 1
                                             Maximum number of slots which can be simultaneously busy.*/
        /******************** CCID Endpoint descriptor (Bulk-OUT) ********************/
        /* 63 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        CCID_BULK_EPOUT_ADDR,             /* bEndpointAddress: Endpoint Address (OUT) */
        USB_EP_TYPE_BULK,                 /* bmAttributes: Bulk Endpoint */
        LOBYTE(CCID_BULK_EPOUT_SIZE),     /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(CCID_BULK_EPOUT_SIZE),     /* wMaxPacketSize (high byte): 64 Byte max */
        CCID_BULK_FS_BINTERVAL,                /* bInterval: Polling Interval */
        /******************** CCID Endpoint descriptor (Bulk-IN) ********************/
        /* 70 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        CCID_BULK_EPIN_ADDR,              /* bEndpointAddress: Endpoint Address (IN) */
        USB_EP_TYPE_BULK,                 /* bmAttributes: Bulk Endpoint */
        LOBYTE(CCID_BULK_EPIN_SIZE),      /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(CCID_BULK_EPIN_SIZE),      /* wMaxPacketSize (high byte): 64 Byte max */
        CCID_BULK_FS_BINTERVAL,           /* bInterval: Polling Interval */
        /* 77 */
};

// DEBUG Descriptor
FLASH uint8_t DebugDesc[] = {
        /******************** Interface, DEBUG Descriptor (two endpoints) ********************/
        0x09,                             /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE,          /* bDescriptorType: Interface */
        CHANGE_ME,                        /* bInterfaceNumber: Number of Interface */
        0x00,                             /* bAlternateSetting: Alternate setting */
        0x02,                             /* bNumEndpoints: Number of endpoints in Interface */
        USB_DEV_CLASS_HID,                /* bInterfaceClass: Human Interface Device */
        0x00,                             /* bInterfaceSubClass : 1=BOOT, 0=no boot */
        0x00,                             /* bInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
        USB_IDX_INTERFACE_DEBUG_STR,      /* iInterface: Index of string descriptor */
        /******************** DEBUG Device Descriptor ********************/
        /* 9 */
        0x09,                             /* bLength: HID Descriptor size */
        USB_DESC_TYPE_HID,                /* bDescriptorType: HID */
        0x11,                             /* bcdHID (low byte): HID Class Spec release number */
        0x01,                             /* bcdHID (high byte): HID Class Spec release number */
        0x00,                             /* bCountryCode: Hardware target country */
        0x01,                             /* bNumDescriptors: Number of HID class descriptors to follow */
        USB_DESC_TYPE_REPORT,             /* bDescriptorType: Report */
        LOBYTE(DEBUG_REPORT_DESC_SIZE),   /* wDescriptorLength (low byte): Total length of Report descriptor */
        HIBYTE(DEBUG_REPORT_DESC_SIZE),   /* wDescriptorLength (high byte): Total length of Report descriptor */
        /******************** DEBUG Endpoint Descriptor (OUT) ********************/
        /* 18 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        DEBUG_EPOUT_ADDR,                 /* bEndpointAddress: Endpoint Address (OUT) */
        USB_EP_TYPE_INTERRUPT,            /* bmAttributes: Interrupt endpoint */
        LOBYTE(DEBUG_EPOUT_SIZE),         /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(DEBUG_EPOUT_SIZE),         /* wMaxPacketSize (high byte): 64 Byte max */
        DEBUG_FS_BINTERVAL,               /* bInterval: Polling Interval */
        /******************** DEBUG Endpoint Descriptor (IN) ********************/
        /* 25 */
        0x07,                             /* bLength: Endpoint Descriptor size */
        USB_DESC_TYPE_ENDPOINT,           /* bDescriptorType: Endpoint */
        DEBUG_EPIN_ADDR,                  /* bEndpointAddress: Endpoint Address (IN) */
        USB_EP_TYPE_INTERRUPT,            /* bmAttributes: Interrupt endpoint */
        LOBYTE(DEBUG_EPIN_SIZE),          /* wMaxPacketSize (low byte): 64 Byte max */
        HIBYTE(DEBUG_EPIN_SIZE),          /* wMaxPacketSize (high byte): 64 Byte max */
        DEBUG_FS_BINTERVAL,               /* bInterval: Polling Interval */
        /* 32 */
};

// FIDO Device Descriptor (copy from FidoDesc)
FLASH uint8_t FidoCfgDesc[] = {
        0x09,                             /* bLength: HID Descriptor size */
        USB_DESC_TYPE_HID,                /* bDescriptorType: HID */
        0x11,                             /* bcdHID (low byte): HID Class Spec release number */
        0x01,                             /* bcdHID (high byte): HID Class Spec release number */
        0x00,                             /* bCountryCode: Hardware target country */
        0x01,                             /* bNumDescriptors: Number of HID class descriptors to follow */
        USB_DESC_TYPE_REPORT,             /* bDescriptorType: Report */
        LOBYTE(FIDO_REPORT_DESC_SIZE),    /* wDescriptorLength (low byte): Total length of Report descriptor */
        HIBYTE(FIDO_REPORT_DESC_SIZE),    /* wDescriptorLength (high byte): Total length of Report descriptor */
};

FLASH uint8_t FidoReportDesc[] ={
        0x06, 0xD0, 0xF1,                 /* Usage Page (FIDO Alliance Page) */
        0x09, 0x01,                       /* Usage (U2F Authenticator Device) */
        0xA1, 0x01,                       /*   Collection (Application) */
        /* 7 */
        0x09, 0x20,                       /*     Usage (Input Report Data) */
        0x15, 0x00,                       /*     Logical Minimum (0) */
        0x26, 0xFF, 0x00,                 /*     Logical Maximum (255) */
        0x75, 0x08,                       /*     Report Size (8) */
        0x95, MAX_PACKET_SIZE,            /*     Report Count (64) */
        0x81, 0x02,                       /*     Input (Data, Variable, Absolute); */
        /* 20 */
        0x09, 0x21,                       /*     Usage (Output Report Data) */
        0x15, 0x00,                       /*     Logical Minimum (0) */
        0x26, 0xFF, 0x00,                 /*     Logical Maximum (255) */
        0x75, 0x08,                       /*     Report Size (8) */
        0x95, MAX_PACKET_SIZE,            /*     Report Count (64) */
        0x91, 0x02,                       /*     Output (Data, Variable, Absolute) */
        /* 33 */
        0x09, 0x07,                       /*     Usage (7, Reserved) */
        0x15, 0x00,                       /*     Logical Minimum (0) */
        0x26, 0xFF, 0x00,                 /*     Logical Maximum (255) */
        0x75, 0x08,                       /*     Report Size (8) */
        0x95, 0x08,                       /*     Report Count (8) */
        0xB1, 0x02,                       /*     Feature (2) (???) */
        /* 46 */
        0xC0                              /*   End Collection */
        /* 47 */
};

// DEBUG Device Descriptor (copy from DebugDesc)
FLASH uint8_t DebugCfgDesc[] = {
        0x09,                             /* bLength: HID Descriptor size */
        USB_DESC_TYPE_HID,                /* bDescriptorType: HID */
        0x11,                             /* bcdHID (low byte): HID Class Spec release number */
        0x01,                             /* bcdHID (high byte): HID Class Spec release number */
        0x00,                             /* bCountryCode: Hardware target country */
        0x01,                             /* bNumDescriptors: Number of HID class descriptors to follow */
        USB_DESC_TYPE_REPORT,             /* bDescriptorType: Report */
        LOBYTE(DEBUG_REPORT_DESC_SIZE),   /* wDescriptorLength (low byte): Total length of Report descriptor */
        HIBYTE(DEBUG_REPORT_DESC_SIZE),   /* wDescriptorLength (high byte): Total length of Report descriptor */
};

// DEBUG Report Descriptor
FLASH uint8_t DebugReportDesc[] ={
        0x06, 0x00, 0xFF,                 /* Usage Page (Vendor Defined 0xFF00) */
        0x09, 0x01,                       /* Usage (Vendor Usage 1) */
        0xA1, 0x01,                       /*   Collection (Application) */
        /* 7 */
        0x09, 0x02,                       /*     Usage (Output Report Data), 0x02 defines that the output report carries raw data from the host to the device. */
        0x15, 0x00,                       /*     Logical Minimum (0) */
        0x26, 0xFF, 0x00,                 /*     Logical Maximum (255) */
        0x75, 0x08,                       /*     Report Size (8 bits) */
        0x95, MAX_PACKET_SIZE,            /*     Report Count (64 bytes) */
        0x91, 0x02,                       /*     Output (Data, Variable, Absolute) */
        /* 20 */
        0x09, 0x03,                       /*     Usage (Input Report), 0x03 defines that the input report carries raw data for the host. */
        0x15, 0x00,                       /*     Logical Minimum (0) */
        0x26, 0xFF, 0x00,                 /*     Logical Maximum (255) */
        0x75, 0x08,                       /*     Report Size (8 bits) */
        0x95, MAX_PACKET_SIZE,            /*     Report Count (64 bytes) */
        0x81, 0x02,                       /*     Input (Data, Variable, Absolute) */
        /* 33 */
        0xC0                              /*   End Collection */
        /* 34 */
};

// String Descriptor (Language descriptor )
FLASH uint8_t LangDesc[] = {
        4,           // Length of this descriptor (in bytes)
        0x03,        // Descriptor type (String)
        0x09, 0x04,  // Language ID (English - US)
};

// CDC Parameters: The initial baud rate is 500000, 1 stop bit, no parity, 8 data bits.
FLASH uint8_t LineCoding[7] = { 0x20, 0xA1, 0x07, 0x00, /* Data terminal rate, in bits per second: 500000 */
                                                  0x00, /* Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits */
                                                  0x00, /* Parity: 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space */
                                                  0x08, /* Data bits (5, 6, 7, 8 or 16) */
                                };

#define UART_RX_BUF_SIZE     256 // Serial receive buffer

/** Communication UART */
volatile XDATA uint8_t UartRxBuf[UART_RX_BUF_SIZE] = { 0 };  // Serial receive buffer
volatile uint8_t UartRxBufInputPointer = 0;   // Circular buffer write pointer, bus reset needs to be initialized to 0
volatile uint8_t UartRxBufOutputPointer = 0;  // Take pointer out of circular buffer, bus reset needs to be initialized to 0
volatile uint8_t UartRxBufByteCount = 0;      // Number of unprocessed bytes remaining in the buffer

/** Debug UART */
#ifdef DEBUG_PRINT_HW
#define DEBUG_UART_RX_BUF_SIZE        8
XDATA uint8_t DebugUartRxBuf[DEBUG_UART_RX_BUF_SIZE] = { 0 };
volatile IDATA uint8_t DebugUartRxBufInputPointer = 0;
volatile IDATA uint8_t DebugUartRxBufOutputPointer = 0;
volatile IDATA uint8_t DebugUartRxBufByteCount = 0;
#endif

/** Endpoint handling */
volatile uint8_t UsbEp2ByteCount = 0;     // Represents the data received by USB endpoint 2 (CDC)
volatile uint8_t UsbEp3ByteCount = 0;     // Represents the data received by USB endpoint 3 (FIDO or CCID)
volatile uint8_t UsbEp4ByteCount = 0;     // Represents the data received by USB endpoint 4 (DEBUG)

volatile uint8_t Endpoint2UploadBusy = 0; // Whether the upload endpoint 2 (CDC) is busy
volatile uint8_t Endpoint3UploadBusy = 0; // Whether the upload endpoint 3 (FIDO or CCID) is busy
volatile uint8_t Endpoint4UploadBusy = 0; // Whether the upload endpoint 4 (DEBUG) is busy

/** CH552 variables */
uint8_t CH552DataAvailable = 0;

/** DEBUG variables */
uint8_t DebugDataAvailable = 0;

/** CDC variables */
uint8_t CdcDataAvailable = 0;
uint8_t CdcSendZeroLenPacket = 0;

/** FIDO variables */
uint8_t FidoDataAvailable = 0;

/** CCID variables */
uint8_t CcidDataAvailable = 0;

/** Frame data */
#define MAX_FRAME_SIZE    64
XDATA uint8_t FrameBuf[MAX_FRAME_SIZE] = { 0 };
uint8_t FrameBufLength = 0;

uint8_t FrameMode   = 0;
uint8_t FrameLength = 0;
uint8_t FrameRemainingBytes = 0;
uint8_t FrameStarted = 0;
uint8_t FrameDiscard = 0;
uint8_t DiscardDataAvailable = 0;

static void memcpy_local(void *dst, const void *src, uint8_t len);
static uint8_t increment_pointer(uint8_t p, uint8_t inc);
void cts_start(void);
void cts_stop(void);
void check_cts_stop(void);

/*******************************************************************************
 * Function Name  : USBDeviceCfg()
 * Description    : USB device mode configuration
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USBDeviceCfg()
{
    USB_CTRL = 0x00;                                       // Clear USB control register
    USB_CTRL &= ~bUC_HOST_MODE;                            // This bit selects the device mode
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // USB device and internal pull-up enable, automatically return to NAK before interrupt flag is cleared
    USB_DEV_AD = 0x00;                                     // Device address initialization
#if 0
    // USB_CTRL  |= bUC_LOW_SPEED;
    // UDEV_CTRL |= bUD_LOW_SPEED;                         // Select low speed 1.5M mode
#else
    USB_CTRL  &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                           // Select full speed 12M mode, the default mode
#endif
    UDEV_CTRL |= bUD_PD_DIS;                               // Disable DP / DM pull-down resistor
    UDEV_CTRL |= bUD_PORT_EN;                              // Enable physical port
}

/*******************************************************************************
 * Function Name  : USBDeviceIntCfg()
 * Description    : USB device mode interrupt initialization
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;  // Enable device suspend interrupt
    USB_INT_EN |= bUIE_TRANSFER; // Enable USB transfer completion interrupt
    USB_INT_EN |= bUIE_BUS_RST;  // Enable device mode USB bus reset interrupt
    USB_INT_FG |= (UIF_FIFO_OV | UIF_HST_SOF | UIF_SUSPEND | UIF_TRANSFER | UIF_BUS_RST ); // Clear interrupt flag
    IE_USB = 1;                  // Enable USB interrupt
    EA = 1;                      // Allow microcontroller interrupt
}

/*******************************************************************************
 * Function Name  : USBDeviceEndPointCfg()
 * Description    : USB device mode endpoint configuration, simulation compatible
 *                  HID device, in addition to endpoint 0 control transmission,
 *                  also includes endpoint 2 batch upload
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USBDeviceEndPointCfg()
{
    // TODO: Is casting the right thing here? What about endianness?
    UEP0_DMA = (uint16_t) Ep0Buffer; // Endpoint 0 data transfer address, Endpoint 4
    UEP1_DMA = (uint16_t) Ep1Buffer; // Endpoint 1 sends data transfer address
    UEP2_DMA = (uint16_t) Ep2Buffer; // Endpoint 2 IN data transfer address
    UEP3_DMA = (uint16_t) Ep3Buffer; // Endpoint 3 IN data transfer address

    UEP4_1_MOD = bUEP4_TX_EN | bUEP4_RX_EN | bUEP1_TX_EN;               // Endpoint 0 single 8-byte send and receive buffer
                                                                        // Endpoint 1, transmit enable
                                                                        // Endpoint 4, single buffer, transmit+receive enable
    UEP2_3_MOD = bUEP2_TX_EN | bUEP2_RX_EN | bUEP3_TX_EN | bUEP3_RX_EN; // Endpoint 2, single buffer, transmit+receive enable
                                                                        // Endpoint 3, single buffer, transmit+receive enable

    UEP0_CTRL =                 UEP_T_RES_NAK | UEP_R_RES_ACK; // Endpoint 0, manual flip, OUT transaction returns ACK, IN transaction returns NAK
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                 // Endpoint 1, automatically flips the synchronization flag, IN transaction returns NAK
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; // Endpoint 2, automatically flips the synchronization flag, IN transaction returns NAK, OUT returns ACK
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; // Endpoint 3, automatically flips the synchronization flag, IN transaction returns NAK, OUT returns ACK
    UEP4_CTRL =                 UEP_T_RES_NAK | UEP_R_RES_ACK; // Endpoint 4, manual flip, OUT transaction returns ACK, IN transaction returns NAK
}

void CreateCfgDescriptor(uint8_t ep_config)
{
    uint8_t num_iface = 0;   // Interface number

    FidoInterfaceNum  = 0xFF; // Set as invalid until we have parsed each interface
    CcidInterfaceNum  = 0xFF; // Set as invalid until we have parsed each interface
    DebugInterfaceNum = 0xFF; // Set as invalid until we have parsed each interface

    memset(ActiveCfgDesc, 0, MAX_CFG_DESC_SIZE); // Clean the descriptor

    uint8_t cfg_desc_size = sizeof(CfgDesc);
    memcpy(ActiveCfgDesc, CfgDesc, cfg_desc_size);
    ActiveCfgDescSize += cfg_desc_size;

    if (ep_config & IO_CDC) {
        uint8_t cdc_desc_size = sizeof(CdcDesc);
        memcpy(ActiveCfgDesc + ActiveCfgDescSize, CdcDesc, cdc_desc_size);
        ActiveCfgDesc[ActiveCfgDescSize + 10] = num_iface;
        num_iface++;
        ActiveCfgDesc[ActiveCfgDescSize + 45] = num_iface;
        num_iface++;
        ActiveCfgDescSize += cdc_desc_size;
    }

    if (ep_config & IO_FIDO) {
        uint8_t fido_desc_size = sizeof(FidoDesc);
        memcpy(ActiveCfgDesc + ActiveCfgDescSize, FidoDesc, fido_desc_size);
        ActiveCfgDesc[ActiveCfgDescSize + 2] = num_iface;
        FidoInterfaceNum = num_iface;
        num_iface++;
        ActiveCfgDescSize += fido_desc_size;
    }

    if (ep_config & IO_CCID) {
        uint8_t ccid_desc_size = sizeof(CcidDesc);
        memcpy(ActiveCfgDesc + ActiveCfgDescSize, CcidDesc, ccid_desc_size);
        ActiveCfgDesc[ActiveCfgDescSize + 2] = num_iface;
        CcidInterfaceNum = num_iface;
        num_iface++;
        ActiveCfgDescSize += ccid_desc_size;
    }

    if (ep_config & IO_DEBUG) {
        uint8_t debug_desc_size = sizeof(DebugDesc);
        memcpy(ActiveCfgDesc + ActiveCfgDescSize, DebugDesc, debug_desc_size);
        ActiveCfgDesc[ActiveCfgDescSize + 2] = num_iface;
        DebugInterfaceNum = num_iface;
        num_iface++;
        ActiveCfgDescSize += debug_desc_size;
    }

    ActiveCfgDesc[2] = ActiveCfgDescSize;
    ActiveCfgDesc[4] = num_iface;
}

/*******************************************************************************
 * Function Name  : Config_Uart1(uint8_t *cfg_uart)
 * Description    : Configure serial port 1 parameters
 * Input          : Serial port configuration parameters Four bit baud rate, stop bit, parity, data bit
 * Output         : None
 * Return         : None
 *******************************************************************************/
void Config_Uart1(uint8_t *cfg_uart)
{
    uint32_t uart1_baud = 0;
    *((uint8_t*) &uart1_baud) = cfg_uart[0];
    *((uint8_t*) &uart1_baud + 1) = cfg_uart[1];
    *((uint8_t*) &uart1_baud + 2) = cfg_uart[2];
    *((uint8_t*) &uart1_baud + 3) = cfg_uart[3];
    SBAUD1 = 256 - FREQ_SYS / 16 / uart1_baud;   // SBAUD1 = 256 - Fsys / 16 / baud rate
    IE_UART1 = 1; // Enable UART1 interrupt
}

void usb_irq_setup_handler(void)
{
    uint16_t len = USB_RX_LEN;
    printStrSetup("Setup ");

    if (len == (sizeof(USB_SETUP_REQ))) {
        SetupLen = ((uint16_t) UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
        len = 0; // Defaults to success and uploading 0 length
        SetupReq = UsbSetupBuf->bRequest;

        // Class-Specific Requests, i.e. HID request, CDC request etc.
        if ( ((UsbSetupBuf->bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS) ||
             ((UsbSetupBuf->bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_VENDOR)) {

            printStrSetup("Class/Vendor-Specific Request = ");
            printNumU8HexSetup(SetupReq);
            printStrSetup("\n\t");

            switch(SetupReq) {
            case USB_HID_REQ_TYPE_GET_REPORT:
                printStrSetup("HID GET_REPORT\n");
                break;
            case USB_HID_REQ_TYPE_GET_IDLE:
                printStrSetup("HID GET_IDLE\n");
                break;
            case USB_HID_REQ_TYPE_GET_PROTOCOL:
                printStrSetup("HID GET_PROTOCOL\n");
                break;
            case USB_HID_REQ_TYPE_SET_REPORT:
                printStrSetup("HID SET_REPORT\n");
                break;
            case USB_HID_REQ_TYPE_SET_IDLE:
                printStrSetup("HID SET_IDLE\n");
                break;
            case USB_HID_REQ_TYPE_SET_PROTOCOL:
                printStrSetup("HID SET_PROTOCOL\n");
                break;
            case USB_CDC_REQ_TYPE_SET_LINE_CODING:
                printStrSetup("CDC SET_LINE_CODING\n");
                break;
            case USB_CDC_REQ_TYPE_GET_LINE_CODING:
                printStrSetup("CDC GET_LINE_CODING\n");
                pDescr = LineCoding;
                len = sizeof(LineCoding);
                SetupLen = MIN(SetupLen, len); // Limit total length
                len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                SetupLen -= len;
                pDescr += len;
                break;
            case USB_CDC_REQ_TYPE_SET_CONTROL_LINE_STATE:
                printStrSetup("CDC SET_CONTROL_LINE_STATE\n");
                break;
            default:
                printStrSetup("Unsupported Request!\n");
                len = 0xFF; // Unsupported Request
                break;
            } // END switch(SetupReq)
        } // END Class/Vendor-Specific Requests

        // Standard Request
        else if (((UsbSetupBuf->bmRequestType & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)) {

            printStrSetup("Standard Request = ");
            printNumU8HexSetup(SetupReq);
            printStrSetup("\n\t");

            switch (SetupReq) {
            case USB_GET_DESCRIPTOR:
                printStrSetup("GET_DESCRIPTOR: wValueH = ");
                printNumU8HexSetup(UsbSetupBuf->wValueH);
                printStrSetup("\n\t\t");

                switch (UsbSetupBuf->wValueH) {
                case USB_DESC_TYPE_DEVICE:
                    printStrSetup("DEVICE\n");
                    pDescr = DevDesc; // Send the device descriptor to the buffer to be sent
                    len = sizeof(DevDesc);
                    SetupLen = MIN(SetupLen, len); // Limit total length
                    len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                    memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                    SetupLen -= len;
                    pDescr += len;
                    break;

                case USB_DESC_TYPE_DEVICE_QUALIFIER:
                    printStrSetup("DEVICE_QUALIFIER\n");
                    break;

                case USB_DESC_TYPE_CONFIGURATION:
                    printStrSetup("CONFIGURATION\n");
                    pDescr = ActiveCfgDesc; // Send the configuration descriptor to the buffer to be sent
                    len = ActiveCfgDescSize; // Dynamic value based on what endpoints are enabled
                    SetupLen = MIN(SetupLen, len); // Limit total length
                    len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                    memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                    SetupLen -= len;
                    pDescr += len;
                    break;

                case USB_DESC_TYPE_STRING:
                    printStrSetup("STRING: wValueL = ");
                    printNumU8HexSetup(UsbSetupBuf->wValueL);
                    printStrSetup("\n\t\t\t");

                    if (UsbSetupBuf->wValueL == USB_IDX_LANGID_STR) {
                        printStrSetup("LangDesc\n");
                        pDescr = LangDesc;
                        len = sizeof(LangDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_MFC_STR) {
                        printStrSetup("ManufDesc\n");
                        pDescr = ManufDesc;
                        len = sizeof(ManufDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_PRODUCT_STR) {
                        printStrSetup("ProdDesc\n");
                        pDescr = ProdDesc;
                        len = sizeof(ProdDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_SERIAL_STR) {
                        printStrSetup("SerialDesc\n");
                        pDescr = SerialDesc;
                        len = sizeof(SerialDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_INTERFACE_CDC_CTRL_STR) {
                        printStrSetup("CdcCtrlInterfaceDesc\n");
                        pDescr = CdcCtrlInterfaceDesc;
                        len = sizeof(CdcCtrlInterfaceDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_INTERFACE_CDC_DATA_STR) {
                        printStrSetup("CdcDataInterfaceDesc\n");
                        pDescr = CdcDataInterfaceDesc;
                        len = sizeof(CdcDataInterfaceDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_INTERFACE_FIDO_STR) {
                        printStrSetup("FidoHidInterfaceDesc\n");
                        pDescr = FidoInterfaceDesc;
                        len = sizeof(FidoInterfaceDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_INTERFACE_CCID_STR) {
                        printStrSetup("CcidInterfaceDesc\n");
                        pDescr = CcidInterfaceDesc;
                        len = sizeof(CcidInterfaceDesc);
                    } else if (UsbSetupBuf->wValueL == USB_IDX_INTERFACE_DEBUG_STR) {
                        printStrSetup("DebugInterfaceDesc\n");
                        pDescr = DebugInterfaceDesc;
                        len = sizeof(DebugInterfaceDesc);
                    } else {
                        printStrSetup("Unknown String!\n");
                        len = 0xFF; // Unsupported
                        break;      // Early exit
                    }
                    SetupLen = MIN(SetupLen, len); // Limit total length
                    len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                    memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                    SetupLen -= len;
                    pDescr += len;
                    break;

                case USB_DESC_TYPE_HID:
                    printStrSetup("HID: wValueL = ");
                    printNumU8HexSetup(UsbSetupBuf->wValueL);
                    printStrSetup("\n");

                    if (UsbSetupBuf->wIndexL == FidoInterfaceNum) { // Interface number for FIDO
                        printStrSetup("FidoCfgDesc\n");
                        pDescr = FidoCfgDesc;
                        len = sizeof(FidoCfgDesc);
                    } else if (UsbSetupBuf->wIndexL == DebugInterfaceNum) { // Interface number for DEBUG
                        printStrSetup("DebugCfgDesc\n");
                        pDescr = DebugCfgDesc;
                        len = sizeof(DebugCfgDesc);
                    } else {
                        printStrSetup("Unknown HID Interface!\n");
                        len = 0xFF; // Unsupported
                        break;      // Early exit
                    }
                    SetupLen = MIN(SetupLen, len); // Limit total length
                    len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                    memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                    SetupLen -= len;
                    pDescr += len;
                    break;

                case USB_DESC_TYPE_REPORT:
                    printStrSetup("REPORT: wIndexL = ");
                    printNumU8HexSetup(UsbSetupBuf->wIndexL);
                    printStrSetup("\n");

                    if (UsbSetupBuf->wIndexL == FidoInterfaceNum) { // Interface number for FIDO
                        printStrSetup("FidoReportDesc\n");
                        pDescr = FidoReportDesc;
                        len = sizeof(FidoReportDesc);
                    } else if (UsbSetupBuf->wIndexL == DebugInterfaceNum) { // Interface number for DEBUG
                        printStrSetup("DebugReportDesc\n");
                        pDescr = DebugReportDesc;
                        len = sizeof(DebugReportDesc);
                    } else {
                        printStrSetup("Unknown Report!\n");
                        len = 0xFF; // Unknown Report
                        break;      // Early exit
                    }
                    SetupLen = MIN(SetupLen, len); // Limit total length
                    len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                    memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                    SetupLen -= len;
                    pDescr += len;
                    break;

                case USB_DESC_TYPE_DEBUG:
                    printStrSetup("DEBUG\n");
                    break;

                default:
                    printStrSetup("Unknown descriptor!\n");
                    len = 0xFF; // Unknown descriptor
                    break;
                } // END switch (UsbSetupBuf->wValueH)
                break;

            case USB_SET_ADDRESS:
                printStrSetup("SET_ADDRESS\n");
                SetupLen = UsbSetupBuf->wValueL; // Temporary storage of USB device address
                break;

            case USB_GET_CONFIGURATION:
                printStrSetup("GET_CONFIGURATION\n");
                Ep0Buffer[0] = UsbConfig;
                if (SetupLen >= 1) {
                    len = 1;
                }
                break;

            case USB_SET_CONFIGURATION:
                printStrSetup("SET_CONFIGURATION\n");
                UsbConfig = UsbSetupBuf->wValueL;
                break;

            case USB_GET_INTERFACE:
                printStrSetup("GET_INTERFACE\n");
                break;

            case USB_CLEAR_FEATURE:
                printStrSetup("CLEAR_FEATURE\n");
                if ((UsbSetupBuf->bmRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {  // Remove device
                    if ((((uint16_t) UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
                        if (CfgDesc[7] & 0x20) {
                            // Wake
                        } else {
                            printStrSetup("Operation failed\n");
                            len = 0xFF; // Operation failed
                        }
                    } else {
                        printStrSetup("Operation failed\n");
                        len = 0xFF; // Operation failed
                    }
                } else if ((UsbSetupBuf->bmRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) { // Endpoint
                    switch (UsbSetupBuf->wIndexL) {
                    case 0x84:
                        UEP4_CTRL = (UEP4_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; // Set endpoint 4 IN (TX) NAK
                        break;
                    case 0x04:
                        UEP4_CTRL = (UEP4_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK; // Set endpoint 4 OUT (RX) ACK
                        break;
                    case 0x83:
                        UEP3_CTRL = (UEP3_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; // Set endpoint 3 IN (TX) NAK
                        break;
                    case 0x03:
                        UEP3_CTRL = (UEP3_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK; // Set endpoint 3 OUT (RX) ACK
                        break;
                    case 0x82:
                        UEP2_CTRL = (UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; // Set endpoint 2 IN (TX) NACK
                        break;
                    case 0x02:
                        UEP2_CTRL = (UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK; // Set endpoint 2 OUT (RX) ACK
                        break;
                    case 0x81:
                        UEP1_CTRL = (UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; // Set endpoint 1 IN (TX) NACK
                        break;
                    case 0x01:
                        UEP1_CTRL = (UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK; // Set endpoint 1 OUT (RX) ACK
                        break;
                    default:
                        printStrSetup("Unsupported endpoint\n");
                        len = 0xFF; // Unsupported endpoint
                        break;
                    } // END switch (UsbSetupBuf->wIndexL)
                } else {
                    printStrSetup("Unsupported\n");
                    len = 0xFF; // It's not that the endpoint doesn't support it
                }
                break;

            case USB_SET_FEATURE:
                printStrSetup("SET_FEATURE\n");
                if (( UsbSetupBuf->bmRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) { // Set up the device
                    if ((((uint16_t) UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01) {
                        if (CfgDesc[7] & 0x20) {
                            printStrSetup("Suspend\n");
                            while (XBUS_AUX & bUART0_TX) {
                                ; // Wait for sending to complete
                            }
                            SAFE_MOD = 0x55;
                            SAFE_MOD = 0xAA;
                            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO; // USB or RXD0/1 can be woken up when there is a signal
                            PCON |= PD; // Sleep
                            SAFE_MOD = 0x55;
                            SAFE_MOD = 0xAA;
                            WAKE_CTRL = 0x00;
                        } else {
                            len = 0xFF; // Operation failed
                        }
                    } else {
                        len = 0xFF; // Operation failed
                    }
                } else if (( UsbSetupBuf->bmRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) { // Set endpoint
                    if ((((uint16_t) UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00) {
                        switch (((uint16_t) UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL) {
                        case 0x84:
                            UEP4_CTRL = (UEP4_CTRL & ~bUEP_T_TOG) | UEP_T_RES_STALL; // Set endpoint 4 IN (TX) Stall (error)
                            break;
                        case 0x04:
                            UEP4_CTRL = (UEP4_CTRL & ~bUEP_R_TOG) | UEP_R_RES_STALL; // Set endpoint 4 OUT (RX) Stall (error)
                            break;
                        case 0x83:
                            UEP3_CTRL = (UEP3_CTRL & ~bUEP_T_TOG) | UEP_T_RES_STALL; // Set endpoint 3 IN (TX) Stall (error)
                            break;
                        case 0x03:
                            UEP3_CTRL = (UEP3_CTRL & ~bUEP_R_TOG) | UEP_R_RES_STALL; // Set endpoint 3 OUT (RX) Stall (error)
                            break;
                        case 0x82:
                            UEP2_CTRL = (UEP2_CTRL & ~bUEP_T_TOG) | UEP_T_RES_STALL; // Set endpoint 2 IN (TX) Stall (error)
                            break;
                        case 0x02:
                            UEP2_CTRL = (UEP2_CTRL & ~bUEP_R_TOG) | UEP_R_RES_STALL; // Set endpoint 2 OUT (RX) Stall (error)
                            break;
                        case 0x81:
                            UEP1_CTRL = (UEP1_CTRL & ~bUEP_T_TOG) | UEP_T_RES_STALL; // Set endpoint 1 IN (TX) Stall (error)
                            break;
                        case 0x01:
                            UEP1_CTRL = (UEP1_CTRL & ~bUEP_R_TOG) | UEP_R_RES_STALL; // Set endpoint 1 OUT (RX) Stall (error)
                        default:
                            len = 0xFF; // Operation failed
                            break;
                        }
                    } else {
                        printStrSetup("Operation failed\n");
                        len = 0xFF; // Operation failed
                    }
                } else {
                    printStrSetup("Operation failed\n");
                    len = 0xFF; // Operation failed
                }
                break;

            case USB_GET_STATUS:
                printStrSetup("GET_STATUS\n");
                Ep0Buffer[0] = 0x00;
                Ep0Buffer[1] = 0x00;
                if (SetupLen >= 2) {
                    len = 2;
                } else {
                    len = SetupLen;
                }
                break;

            default:
                len = 0xFF; // Operation failed
                printStrSetup("Unsupported\n");
                break;
            } // END switch (SetupReq)
        } // END Standard request

        // Unknown Request
        else {
            printStrSetup("Unknown Request Type!\n");
            len = 0xFF; // Operation failed
        } // END Unknown Request

    } else {
        len = 0xFF; // Packet length error
    }

    if (len == 0xFF) {
        SetupReq = 0xFF;
        UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
    } else if (len <= DEFAULT_EP0_SIZE) { // Upload data or status phase returns 0 length packet
        UEP0_T_LEN = len;
        UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // The default packet is DATA1, return response ACK
    } else {
        UEP0_T_LEN = 0; // Although it has not yet reached the status stage, it is preset to upload 0-length data packets in advance to prevent the host from entering the status stage early.
        UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // The default data packet is DATA1, and the response ACK is returned
    }
}

/*******************************************************************************
 * Function Name  : DeviceInterrupt()
 * Description    : CH559USB interrupt processing function
 *******************************************************************************/
#ifdef BUILD_CODE
#define IRQ_USB __interrupt(INT_NO_USB)
#else
#define IRQ_USB
#endif

void DeviceInterrupt(void)IRQ_USB // USB interrupt service routine, using register set 1
{
    uint16_t len;

    if (UIF_TRANSFER) { // Check USB transfer complete flag
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {

        case UIS_TOKEN_SETUP | 0: // SETUP routine
            usb_irq_setup_handler();
            break;

        case UIS_TOKEN_IN | 0: // Endpoint 0 IN (TX)
            switch (SetupReq) {
            case USB_GET_DESCRIPTOR:
                /* Continue sending descriptor in multiple packets if needed. Started from SETUP routine */
                len = (SetupLen >= DEFAULT_EP0_SIZE) ? DEFAULT_EP0_SIZE : SetupLen; // The length of this transmission
                memcpy(Ep0Buffer, pDescr, len); // Copy upload data
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG; // Sync flag flip
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = (USB_DEV_AD & bUDA_GP_BIT) | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0; // The status phase is completed and interrupted or the 0-length data packet is forced to be uploaded to end the control transmission.
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;

        case UIS_TOKEN_IN | 1: // Endpoint 1 IN (TX)
            UEP1_T_LEN = 0;    // Transmit length must be cleared (Endpoint 1)
            UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK; // Default answer NAK
            break;

        case UIS_TOKEN_IN | 2: // Endpoint 2 IN (TX)
            UEP2_T_LEN = 0;    // Transmit length must be cleared (Endpoint 2)
            UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK; // Default answer NAK
            Endpoint2UploadBusy = 0; // Clear busy flag
            break;

        case UIS_TOKEN_IN | 3: // Endpoint 3 IN (TX)
            UEP3_T_LEN = 0;    // Transmit length must be cleared (Endpoint 3)
            UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK; // Default answer NAK
            Endpoint3UploadBusy = 0; // Clear busy flag
            break;

        case UIS_TOKEN_IN | 4: // Endpoint 4 IN (TX)
            UEP4_T_LEN = 0;    // Transmit length must be cleared (Endpoint 4)
            UEP4_CTRL = (UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK; // Default answer NAK
            UEP4_CTRL ^= bUEP_T_TOG; // Sync flag flip
            Endpoint4UploadBusy = 0; // Clear busy flag
            break;

        case UIS_TOKEN_OUT | 0: // Endpoint 0 OUT (RX)
            switch (SetupReq) {
            case USB_CDC_REQ_TYPE_SET_LINE_CODING:
                /* We ignore line coding here because baudrate to the FPGA should not change */
                if (U_TOG_OK) {
                    UEP0_T_LEN = 0;
                    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK; // Prepare to upload 0 packages
                }
                break;
            default:
                UEP0_T_LEN = 0;
                UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_NAK; // Status phase, responds to IN with NAK
                break;
            }
            break;

        case UIS_TOKEN_OUT | 1: // Endpoint 1 OUT (RX), Disabled for now.
            // Out-of-sync packets will be dropped
            if (U_TOG_OK) {
                //UsbEpXByteCount = USB_RX_LEN;                              // Length of received data
                //UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK; // NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
            }
            break;

        case UIS_TOKEN_OUT | 2: // Endpoint 2 OUT (RX)
            // Out-of-sync packets will be dropped
            if (U_TOG_OK) {
                UsbEp2ByteCount = USB_RX_LEN;                              // Length of received data
                if (UsbEp2ByteCount == 0) {
                    // If zero, assume it is a zero-length packet. Ignore and
                    // wait for next frame
                    break;
                }
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK; // NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
            }
            break;

        case UIS_TOKEN_OUT | 3: // Endpoint 3 OUT (RX)
            // Out-of-sync packets will be dropped
            if (U_TOG_OK) {
                UsbEp3ByteCount = USB_RX_LEN;                              // Length of received data
                UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK; // NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
            }
            break;

        case UIS_TOKEN_OUT | 4: // Endpoint 4 OUT (RX)
            // Out-of-sync packets will be dropped
            if (U_TOG_OK) {
                UsbEp4ByteCount = USB_RX_LEN;                              // Length of received data
                UEP4_CTRL = (UEP4_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK; // NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
                UEP4_CTRL ^= bUEP_R_TOG;                                   // Sync flag flip
            }
            break;

        default:
            break;
        }

        UIF_TRANSFER = 0; // Writing 0 clears the interrupt

    } else if (UIF_BUS_RST) { // Check device mode USB bus reset interrupt

        printStrSetup("Reset\n");

        UEP0_CTRL =                 UEP_T_RES_NAK | UEP_R_RES_ACK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        UEP4_CTRL =                 UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;               // Writing 0 clears the interrupt
        UIF_BUS_RST = 0;                // Clear interrupt flag

        UartRxBufInputPointer = 0;      // Circular buffer input pointer
        UartRxBufOutputPointer = 0;     // Circular buffer read pointer
        UartRxBufByteCount = 0;         // The number of bytes remaining in the current buffer to be fetched
        UsbEp2ByteCount = 0;            // USB endpoint 2 (CDC) received length
        UsbEp3ByteCount = 0;            // USB endpoint 3 (FIDO) received length
        UsbEp4ByteCount = 0;            // USB endpoint 4 (DEBUG) received length
        Endpoint2UploadBusy = 0;        // Clear busy flag
        Endpoint3UploadBusy = 0;        // Clear busy flag
        Endpoint4UploadBusy = 0;        // Clear busy flag

        FrameMode = 0;

        UsbConfig = 0;                  // Clear configuration values

    } else if (UIF_SUSPEND) { // Check USB bus suspend/wake completed

        UIF_SUSPEND = 0;

        if (USB_MIS_ST & bUMS_SUSPEND) { // Hang

            printStrSetup("Suspend\n");

            while (XBUS_AUX & bUART0_TX) {
                ; // Wait for sending to complete
            }

            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO; // Can be woken up when there is a signal from USB or RXD0/1
            PCON |= PD; // Sleep
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;

        }
    } else { // Unexpected IRQ, should not happen
        printStrSetup("Unexpected IRQ\n");
        USB_INT_FG = 0xFF; // Clear interrupt flag
    }
}

/*******************************************************************************
 * Function Name  : Uart0_ISR()
 * Description    : Serial debug port receiving interrupt function to realize circular buffer receiving
 *******************************************************************************/
#ifdef DEBUG_PRINT_HW
#ifdef BUILD_CODE
#define IRQ_UART0 __interrupt(INT_NO_UART0)
#else
#define IRQ_UART0
#endif

void Uart0_ISR(void)IRQ_UART0
{
    // Check if data has been received
    if (RI) {
        DebugUartRxBuf[DebugUartRxBufInputPointer++] = SBUF;
        if (DebugUartRxBufInputPointer >= DEBUG_UART_RX_BUF_SIZE) {
            DebugUartRxBufInputPointer = 0; // Reset write pointer
        }
        RI = 0;
    }
}

uint8_t debug_uart_byte_count()
{
    uint8_t  in = DebugUartRxBufInputPointer;
    uint8_t out = DebugUartRxBufOutputPointer;

    if (in < out) {
        in = in + DEBUG_UART_RX_BUF_SIZE;
    }
    return in - out;
}
#endif

/*******************************************************************************
 * Function Name  : Uart1_ISR()
 * Description    : Serial port receiving interrupt function to realize circular buffer receiving
 *******************************************************************************/
#ifdef BUILD_CODE
#define IRQ_UART1 __interrupt(INT_NO_UART1)
#else
#define IRQ_UART1
#endif

void Uart1_ISR(void)IRQ_UART1
{
    // Check if data has been received
    if (U1RI) {
        // bufsize is 256, will wrap naturally
        UartRxBuf[UartRxBufInputPointer++] = SBUF1;
        // if (UartRxBufInputPointer >= UART_RX_BUF_SIZE) {
        //     UartRxBufInputPointer = 0; // Reset write pointer
        // }

        check_cts_stop();

        U1RI = 0;
    }
}

uint8_t uart_byte_count()
{
    uint8_t in = UartRxBufInputPointer;
    uint8_t out = UartRxBufOutputPointer;

    if (in >= out) {
        return (in - out);
    } else {
        return (UART_RX_BUF_SIZE - (out - in));
    }
}

// Local byte-wise copy function.
// The SDCC mcs51 memcpy() uses fixed registers and is not reentrant, so it can
// be corrupted if interrupted by an ISR.
// This implementation is reentrant, given that source and destination buffers
// are not accessed concurrently. Use this instead of memcpy() if it could be
// interrupted.
static void memcpy_local(void *dst, const void *src, uint8_t len)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (len--) *d++ = *s++;
}

// Copy data from a circular buffer
void inline circular_copy(uint8_t *dest, uint8_t *src, uint8_t start_pos, uint8_t length) {

    // Calculate the remaining space from start_pos to end of buffer
    uint8_t remaining_space = 256 - start_pos;

    if (length <= remaining_space) {
        // If the length to copy doesn't exceed the remaining space, do a single memcpy
        memcpy_local(dest, src + start_pos, length);
    } else {
        // If the length to copy exceeds the remaining space, split the copy
        memcpy_local(dest, src + start_pos, remaining_space);                // Copy from start_pos to end of buffer
        memcpy_local(dest + remaining_space, src, length - remaining_space); // Copy the rest from the beginning of buffer
    }
}

// Function to increment a pointer and wrap around the buffer. Can only handle
// a buffer_size of 256 bytes.
static inline uint8_t increment_pointer(uint8_t p, uint8_t inc)
{
    return (p + inc);
}

void cts_start(void)
{
    gpio_p1_5_unset(); // Signal to FPGA to send more data
}

void cts_stop(void)
{
    gpio_p1_5_set(); // Signal to FPGA to not send more data
}

void check_cts_stop(void)
{
    if (uart_byte_count() >= 133) // UartRxBuf is filled to 95% or more
    {
        cts_stop();
    }
}

void main()
{
    CfgFsys();     // CH559 clock selection configuration
    mDelaymS(5);   // Modify the main frequency and wait for the internal crystal to stabilize, which must be added
#ifdef DEBUG_PRINT_HW
    mInitSTDIO();  // Serial port 0, can be used for debugging
#endif
    UART1Setup();  // For communication with FPGA
    UART1Clean();  // Clean register from spurious data

    printStrSetup("\nStartup\n");

    uint8_t ActiveEndpoints = RESET_KEEP;

    // Always enable CDC endpoint
    if ((ActiveEndpoints & IO_CDC) == 0x0) {
        ActiveEndpoints |= IO_CDC;
    }

    // Always enable CH552 endpoint
    if ((ActiveEndpoints & IO_CH552) == 0x0) {
        ActiveEndpoints |= IO_CH552;
    }

    // FIDO and CCID can't be enabled at the same time. Disable both!
    if ((ActiveEndpoints & IO_FIDO) && (ActiveEndpoints & IO_CCID)) {
        ActiveEndpoints &= ~(IO_FIDO | IO_CCID);
    }

    CreateCfgDescriptor(ActiveEndpoints);

    USBDeviceCfg();
    USBDeviceEndPointCfg(); // Endpoint configuration
    USBDeviceIntCfg();      // Interrupt initialization

    UEP0_T_LEN = 0;         // Transmit length must be cleared (Endpoint 0)
    UEP1_T_LEN = 0;         // Transmit length must be cleared (Endpoint 1)
    UEP2_T_LEN = 0;         // Transmit length must be cleared (Endpoint 2)
    UEP3_T_LEN = 0;         // Transmit length must be cleared (Endpoint 3)
    UEP4_T_LEN = 0;         // Transmit length must be cleared (Endpoint 4)

    gpio_init_p1_4_in();    // Init GPIO p1.4 to input mode for FPGA_CTS
    gpio_init_p1_5_out();   // Init GPIO p1.5 to output mode for CH552_CTS
    cts_start();            // Signal OK to send

    while (1) {
        if (UsbConfig) {

            // Check if Endpoint 2 (CDC) has received data
            if (UsbEp2ByteCount) {

                CH554UART1SendByte(IO_CDC);  // Send CDC mode header
                CH554UART1SendByte(UsbEp2ByteCount);  // Send length
                CH554UART1SendBuffer(Ep2Buffer, UsbEp2ByteCount);
                UsbEp2ByteCount = 0;
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK; // Enable Endpoint 2 to ACK again
            }

            // Check if Endpoint 3 (FIDO or CCID) has received data
            if (UsbEp3ByteCount) {

                if (ActiveEndpoints & IO_FIDO) {
                    CH554UART1SendByte(IO_FIDO); // Send FIDO mode header
                } else if (ActiveEndpoints & IO_CCID) {
                    CH554UART1SendByte(IO_CCID); // Send CCID mode header
                }

                CH554UART1SendByte(UsbEp3ByteCount); // Send length (always 64 bytes for FIDO, variable for CCID)
                CH554UART1SendBuffer(Ep3Buffer, UsbEp3ByteCount);
                UsbEp3ByteCount = 0;
                UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK; // Enable Endpoint 3 to ACK again
            }

            // Check if Endpoint 4 (DEBUG) has received data
            if (UsbEp4ByteCount) {

                CH554UART1SendByte(IO_DEBUG); // Send DEBUG mode header
                CH554UART1SendByte(UsbEp4ByteCount); // Send length (always 64 bytes)
                CH554UART1SendBuffer(Ep0Buffer+64, UsbEp4ByteCount); // Endpoint 4 receive is at address UEP0_DMA+64
                UsbEp4ByteCount = 0;
                UEP4_CTRL = (UEP4_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK; // Enable Endpoint 4 to ACK again
            }

            UartRxBufByteCount = uart_byte_count(); // Check amount of data in buffer

            if ((UartRxBufByteCount >= 2) && !FrameStarted) {  // If we have data and the header is not yet validated
                FrameMode = UartRxBuf[UartRxBufOutputPointer]; // Extract frame mode
                if ((FrameMode == IO_CDC)   ||
                    (FrameMode == IO_FIDO)  ||
                    (FrameMode == IO_CCID)  ||
                    (FrameMode == IO_DEBUG) ||
                    (FrameMode == IO_CH552)) {

                    FrameLength = UartRxBuf[increment_pointer(UartRxBufOutputPointer,
                                                              1
                                                              )]; // Extract frame length
                    FrameRemainingBytes = FrameLength;
                    UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                               2
                                                               ); // Start at valid data so skip the mode and length byte
                    UartRxBufByteCount -= 2; // Subtract the frame mode and frame length bytes from the total byte count
                    FrameStarted = 1;

                    // Mark that we should discard data if destination for the frame is not active
                    if ((FrameMode & ActiveEndpoints) == 0) {
                        FrameDiscard = 1;
                    }

                } else { // Invalid frame mode

                    cts_stop();

                    // Reset CH552 to start from a known state
                    SAFE_MOD = 0x55;
                    SAFE_MOD = 0xAA;
                    GLOBAL_CFG = bSW_RESET;
                    while (1)
                        ;
                }
            }

            // Copy CDC data from UartRxBuf to FrameBuf
            if (FrameStarted && !FrameDiscard && !CdcDataAvailable) {
                if (FrameMode == IO_CDC) {
                    if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      MAX_FRAME_SIZE);
                        FrameBufLength = MAX_FRAME_SIZE;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   MAX_FRAME_SIZE
                                                                   );
                        FrameRemainingBytes -= MAX_FRAME_SIZE;
                        CdcDataAvailable = 1;
                        cts_start();
                    }
                    else if ((FrameRemainingBytes < MAX_FRAME_SIZE) &&
                             (UartRxBufByteCount >= FrameRemainingBytes)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      FrameRemainingBytes);
                        FrameBufLength = FrameRemainingBytes;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   FrameRemainingBytes
                                                                   );
                        FrameRemainingBytes -= FrameRemainingBytes;
                        CdcDataAvailable = 1;
                        cts_start();
                    }
                }
            }

            // Copy FIDO data from UartRxBuf to FrameBuf
            if (FrameStarted && !FrameDiscard && !FidoDataAvailable) {
                if (FrameMode == IO_FIDO) {
                    if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      MAX_FRAME_SIZE);
                        FrameBufLength = MAX_FRAME_SIZE;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   MAX_FRAME_SIZE
                                                                   );
                        FrameRemainingBytes -= MAX_FRAME_SIZE;
                        FidoDataAvailable = 1;
                        cts_start();
                    }
                }
            }

            // Copy CCID data from UartRxBuf to FrameBuf
            if (FrameStarted && !FrameDiscard && !CcidDataAvailable) {
                if (FrameMode == IO_CCID) {
                    if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      MAX_FRAME_SIZE);
                        FrameBufLength = MAX_FRAME_SIZE;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   MAX_FRAME_SIZE
                                                                   );
                        FrameRemainingBytes -= MAX_FRAME_SIZE;
                        CcidDataAvailable = 1;
                        cts_start();
                    }
                    else if ((FrameRemainingBytes < MAX_FRAME_SIZE) &&
                             (UartRxBufByteCount >= FrameRemainingBytes)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      FrameRemainingBytes);
                        FrameBufLength = FrameRemainingBytes;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   FrameRemainingBytes
                                                                   );
                        FrameRemainingBytes -= FrameRemainingBytes;
                        CcidDataAvailable = 1;
                        cts_start();
                    }
                }
            }

            // Copy DEBUG data from UartRxBuf to FrameBuf
            if (FrameStarted && !FrameDiscard && !DebugDataAvailable) {
                if (FrameMode == IO_DEBUG) {
                    if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      MAX_FRAME_SIZE);
                        FrameBufLength = MAX_FRAME_SIZE;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   MAX_FRAME_SIZE
                                                                   );
                        FrameRemainingBytes -= MAX_FRAME_SIZE;
                        DebugDataAvailable = 1;
                        cts_start();
                    }
                    else if ((FrameRemainingBytes < MAX_FRAME_SIZE) &&
                             (UartRxBufByteCount >= FrameRemainingBytes)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      FrameRemainingBytes);
                        FrameBufLength = FrameRemainingBytes;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   FrameRemainingBytes
                                                                   );
                        FrameRemainingBytes -= FrameRemainingBytes;
                        DebugDataAvailable = 1;
                        cts_start();
                    }
                }
            }

            // Copy CH552 data from UartRxBuf to FrameBuf
            if (FrameStarted && !FrameDiscard && !CH552DataAvailable) {
                if (FrameMode == IO_CH552) {
                    if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      MAX_FRAME_SIZE);
                        FrameBufLength = MAX_FRAME_SIZE;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   MAX_FRAME_SIZE
                                                                   );
                        FrameRemainingBytes -= MAX_FRAME_SIZE;
                        CH552DataAvailable = 1;
                        cts_start();
                    }
                    else if ((FrameRemainingBytes < MAX_FRAME_SIZE) &&
                             (UartRxBufByteCount >= FrameRemainingBytes)) {
                        circular_copy(FrameBuf,
                                      UartRxBuf,
                                      UartRxBufOutputPointer,
                                      FrameRemainingBytes);
                        FrameBufLength = FrameRemainingBytes;
                        // Update output pointer
                        UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                                   FrameRemainingBytes
                                                                   );
                        FrameRemainingBytes -= FrameRemainingBytes;
                        CH552DataAvailable = 1;
                        cts_start();
                    }
                }
            }

            // Discard frame
            if (FrameStarted && FrameDiscard && !DiscardDataAvailable) {
                if ((FrameRemainingBytes >= MAX_FRAME_SIZE) &&
                    (UartRxBufByteCount >= MAX_FRAME_SIZE)) {
                    // Update output pointer
                    UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                               MAX_FRAME_SIZE
                                                               );
                    FrameRemainingBytes -= MAX_FRAME_SIZE;
                    DiscardDataAvailable = 1;
                    cts_start();
                }
                else if ((FrameRemainingBytes < MAX_FRAME_SIZE) &&
                        (UartRxBufByteCount >= FrameRemainingBytes)) {
                    // Update output pointer
                    UartRxBufOutputPointer = increment_pointer(UartRxBufOutputPointer,
                                                               FrameRemainingBytes
                                                               );
                    FrameRemainingBytes -= FrameRemainingBytes;
                    DiscardDataAvailable = 1;
                    cts_start();
                }
            }

            // Check if we should upload data to Endpoint 2 (CDC)
            if (CdcDataAvailable && !Endpoint2UploadBusy) {

                // Write upload endpoint
                memcpy_local(Ep2Buffer + MAX_PACKET_SIZE, /* Copy to IN buffer of Endpoint 2 */
                       FrameBuf,
                       FrameBufLength);

                Endpoint2UploadBusy = 1; // Set busy flag
                UEP2_T_LEN = FrameBufLength; // Set the number of data bytes that Endpoint 2 is ready to send
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK; // Answer ACK

                if (FrameBufLength == 64) {
                    // Terminate all 64-byte frames
                    CdcSendZeroLenPacket = 1;
                }

                CdcDataAvailable = 0;
                FrameBufLength = 0;

                if (FrameRemainingBytes == 0) {
                    // Complete frame sent, get next header and data
                    FrameStarted = 0;
                }
            }

            if (CdcSendZeroLenPacket && !Endpoint2UploadBusy) {
                // Transmit zero-length packet to terminate 64-byte frames
                // Only applicable to CDC (bulk transfers)

                Endpoint2UploadBusy = 1; // Set busy flag
                UEP2_T_LEN = 0; // Set the number of data bytes that Endpoint 2 is ready to send
                UEP2_CTRL = (UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK; // Answer ACK
                CdcSendZeroLenPacket = 0;
            }

            // Check if we should upload data to Endpoint 3 (FIDO)
            if (FidoDataAvailable && !Endpoint3UploadBusy) {

                // Write upload endpoint
                memcpy_local(Ep3Buffer + MAX_PACKET_SIZE, /* Copy to IN buffer of Endpoint 3 */
                       FrameBuf,
                       FrameBufLength);

                Endpoint3UploadBusy = 1; // Set busy flag
                UEP3_T_LEN = MAX_PACKET_SIZE; // Set the number of data bytes that Endpoint 3 is ready to send
                UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK; // Answer ACK

                FidoDataAvailable = 0;
                FrameBufLength = 0;

                // Get next header and data
                FrameStarted = 0;
            }

            // Check if we should upload data to Endpoint 3 (CCID)
            if (CcidDataAvailable && !Endpoint3UploadBusy) {

                // Write upload endpoint
                memcpy_local(Ep3Buffer + MAX_PACKET_SIZE, /* Copy to IN buffer of Endpoint 3 */
                       FrameBuf,
                       FrameBufLength);

                Endpoint3UploadBusy = 1; // Set busy flag
                UEP3_T_LEN = FrameBufLength; // Set the number of data bytes that Endpoint 3 is ready to send
                UEP3_CTRL = (UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK; // Answer ACK

                CcidDataAvailable = 0;
                FrameBufLength = 0;

                // Get next header and data
                FrameStarted = 0;
            }

            // Check if we should upload data to Endpoint 4 (DEBUG)
            if (DebugDataAvailable && !Endpoint4UploadBusy) {

                if (FrameBufLength == MAX_PACKET_SIZE) {
                    // Write upload endpoint
                    memcpy_local(Ep0Buffer + 128, /* Copy to IN (TX) buffer of Endpoint 4 */
                           FrameBuf,
                           FrameBufLength);
                } else {
                    memset(Ep0Buffer + 128, 0, MAX_PACKET_SIZE);
                    // Write upload endpoint
                    memcpy_local(Ep0Buffer + 128, /* Copy to IN (TX) buffer of Endpoint 4 */
                           FrameBuf,
                           FrameBufLength);
                }

                Endpoint4UploadBusy = 1; // Set busy flag
                UEP4_T_LEN = MAX_PACKET_SIZE; // Set the number of data bytes that Endpoint 4 is ready to send
                UEP4_CTRL = (UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK; // Answer ACK

                DebugDataAvailable = 0;
                FrameBufLength = 0;

                if (FrameRemainingBytes == 0) {
                    // Complete frame sent, get next header and data
                    FrameStarted = 0;
                }
            }

            // Check if we should handle CH552 data
            if (CH552DataAvailable) {

                // Check command range
                if (FrameBuf[0] < CH552_CMD_MAX) {
                    switch (FrameBuf[0]) {
                    case SET_ENDPOINTS:
                        cts_stop(); // Stop UART data from FPGA
                        RESET_KEEP = FrameBuf[1]; // Save endpoints to persistent register
                        SAFE_MOD = 0x55; // Start reset sequence
                        SAFE_MOD = 0xAA;
                        GLOBAL_CFG = bSW_RESET;
                        while (1)
                            ;
                        break;
                    default:
                        break;
                    } // END switch(FrameBuf[0])
                }

                CH552DataAvailable = 0;
                FrameBufLength = 0;

                memset(FrameBuf, 0, MAX_FRAME_SIZE);

                if (FrameRemainingBytes == 0) {
                    // Complete frame sent, get next header and data
                    FrameStarted = 0;
                }
            }

            if (DiscardDataAvailable) {

                DiscardDataAvailable = 0;
                printStr("Frame discarded!\n");

                if (FrameRemainingBytes == 0) {
                    // Complete frame discarded, get next header and data
                    FrameStarted = 0;
                    // Stop discarding frames
                    FrameDiscard = 0;
                }
            }

        } /* END if (UsbConfig) */
    } /* END while (1) */
}
