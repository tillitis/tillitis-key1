// SPDX-FileCopyrightText: 1999 WCH <wch-ic.com>
// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

/*--------------------------------------------------------------------------
CH554.H
Header file for CH554 microcontrollers.
****************************************
**  Copyright  (C)  W.ch  1999-2014   **
**  Web:              http://wch.cn   **
****************************************
--------------------------------------------------------------------------*/


#ifndef __USB_DEF__
#define __USB_DEF__

#include "mem.h"

/*----- USB constant and structure define --------------------------------*/

/* USB PID */
#ifndef USB_PID_SETUP
#define USB_PID_NULL            0x00    /* reserved PID */
#define USB_PID_SOF             0x05
#define USB_PID_SETUP           0x0D
#define USB_PID_IN              0x09
#define USB_PID_OUT             0x01
#define USB_PID_ACK             0x02
#define USB_PID_NAK             0x0A
#define USB_PID_STALL           0x0E
#define USB_PID_DATA0           0x03
#define USB_PID_DATA1           0x0B
#define USB_PID_PRE             0x0C
#endif

/* USB standard device request code */
#ifndef USB_GET_DESCRIPTOR
#define USB_GET_STATUS          0x00
#define USB_CLEAR_FEATURE       0x01
#define USB_SET_FEATURE         0x03
#define USB_SET_ADDRESS         0x05
#define USB_GET_DESCRIPTOR      0x06
#define USB_SET_DESCRIPTOR      0x07
#define USB_GET_CONFIGURATION   0x08
#define USB_SET_CONFIGURATION   0x09
#define USB_GET_INTERFACE       0x0A
#define USB_SET_INTERFACE       0x0B
#define USB_SYNCH_FRAME         0x0C
#endif

/* USB hub class request code */
#ifndef HUB_GET_DESCRIPTOR
#define HUB_GET_STATUS          0x00
#define HUB_CLEAR_FEATURE       0x01
#define HUB_GET_STATE           0x02
#define HUB_SET_FEATURE         0x03
#define HUB_GET_DESCRIPTOR      0x06
#define HUB_SET_DESCRIPTOR      0x07
#endif

/* Bit define for USB request type */
#ifndef USB_REQ_TYPE_MASK
#define USB_REQ_TYPE_IN         0x80    /* Control IN, device to host */
#define USB_REQ_TYPE_OUT        0x00    /* Control OUT, host to device */
#define USB_REQ_TYPE_READ       0x80    /* Control read, device to host */
#define USB_REQ_TYPE_WRITE      0x00    /* Control write, host to device */
#define USB_REQ_TYPE_MASK       0x60    /* Bit mask of request type */
#define USB_REQ_TYPE_STANDARD   0x00
#define USB_REQ_TYPE_CLASS      0x20
#define USB_REQ_TYPE_VENDOR     0x40
#define USB_REQ_TYPE_RESERVED   0x60
#define USB_REQ_RECIP_MASK      0x1F    /* Bit mask of request recipient */
#define USB_REQ_RECIP_DEVICE    0x00
#define USB_REQ_RECIP_INTERF    0x01
#define USB_REQ_RECIP_ENDP      0x02
#define USB_REQ_RECIP_OTHER     0x03
#endif

/* USB HID class request code */
#ifndef USB_HID_REQ_TYPE
#define USB_HID_REQ_TYPE_GET_REPORT    0x01
#define USB_HID_REQ_TYPE_GET_IDLE      0x02
#define USB_HID_REQ_TYPE_GET_PROTOCOL  0x03
#define USB_HID_REQ_TYPE_SET_REPORT    0x09
#define USB_HID_REQ_TYPE_SET_IDLE      0x0A
#define USB_HID_REQ_TYPE_SET_PROTOCOL  0x0B
#endif

/* USB CDC class request code */
#ifndef USB_CDC_REQ_TYPE
#define USB_CDC_REQ_TYPE_SET_LINE_CODING         0x20
#define USB_CDC_REQ_TYPE_GET_LINE_CODING         0x21
#define USB_CDC_REQ_TYPE_SET_CONTROL_LINE_STATE  0x22
#endif

/* USB request type for hub class request */
#ifndef HUB_GET_HUB_DESCRIPTOR
#define HUB_CLEAR_HUB_FEATURE   0x20
#define HUB_CLEAR_PORT_FEATURE  0x23
#define HUB_GET_BUS_STATE       0xA3
#define HUB_GET_HUB_DESCRIPTOR  0xA0
#define HUB_GET_HUB_STATUS      0xA0
#define HUB_GET_PORT_STATUS     0xA3
#define HUB_SET_HUB_DESCRIPTOR  0x20
#define HUB_SET_HUB_FEATURE     0x20
#define HUB_SET_PORT_FEATURE    0x23
#endif

/* Hub class feature selectors */
#ifndef HUB_PORT_RESET
#define HUB_C_HUB_LOCAL_POWER    0
#define HUB_C_HUB_OVER_CURRENT   1
#define HUB_PORT_CONNECTION      0
#define HUB_PORT_ENABLE          1
#define HUB_PORT_SUSPEND         2
#define HUB_PORT_OVER_CURRENT    3
#define HUB_PORT_RESET           4
#define HUB_PORT_POWER           8
#define HUB_PORT_LOW_SPEED       9
#define HUB_C_PORT_CONNECTION    16
#define HUB_C_PORT_ENABLE        17
#define HUB_C_PORT_SUSPEND       18
#define HUB_C_PORT_OVER_CURRENT  19
#define HUB_C_PORT_RESET         20
#endif

/* USB descriptor type */
#ifndef USB_DESC_TYPE_DEVICE
#define USB_DESC_TYPE_DEVICE                                         0x01 // USB 1.1
#define USB_DESC_TYPE_CONFIGURATION                                  0x02 // USB 1.1
#define USB_DESC_TYPE_STRING                                         0x03 // USB 1.1
#define USB_DESC_TYPE_INTERFACE                                      0x04 // USB 1.1
#define USB_DESC_TYPE_ENDPOINT                                       0x05 // USB 1.1
#define USB_DESC_TYPE_DEVICE_QUALIFIER                               0x06 // USB 2.0
#define USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION                      0x07 // USB 2.0
#define USB_DESC_TYPE_INTERFACE_POWER                                0x08 // USB 2.0
#define USB_DESC_TYPE_OTG                                            0x09 // USB 2.0
#define USB_DESC_TYPE_DEBUG                                          0x0A // USB 2.0
#define USB_DESC_TYPE_INTERFACE_ASSOCIATION                          0x0B // USB 2.0
#define USB_DESC_TYPE_BOS                                            0x0F // USB 3.x (Binary Object Store)
#define USB_DESC_TYPE_DEVICE_CAPABILITY                              0x10 // USB 3.x
#define USB_DESC_TYPE_HID                                            0x21 // HID 1.11
#define USB_DESC_TYPE_REPORT                                         0x22 // HID 1.11
#define USB_DESC_TYPE_PHYSICAL                                       0x23 // HID 1.11
#define USB_DESC_TYPE_CS_INTERFACE                                   0x24 // Class-Specific (Audio, HID, CDC, etc.)
#define USB_DESC_TYPE_CS_ENDPOINT                                    0x25 // Class-Specific (Audio, Not HID normally, CDC, etc.)
#define USB_DESC_TYPE_SUPERSPEED_USB_ENDPOINT_COMPANION              0x30 // USB 3.x
#define USB_DESC_TYPE_SUPERSPEEDPLUS_ISOCHRONOUS_ENDPOINT_COMPANION  0x31 // USB 3.x
#endif

/* USB device class */
#ifndef USB_DEV_CLASS
                                                   // Descriptor Usage; Description                             ; Examples
#define USB_DEV_CLASS_RESERVED                0x00 // Device          ; Unspecified                             ; Device class is unspecified, interface descriptors are used to determine needed drivers
#define USB_DEV_CLASS_AUDIO                   0x01 // Interface       ; Audio                                   ; Speaker, microphone, sound card, MIDI
#define USB_DEV_CLASS_CDC_CONTROL             0x02 // Both            ; Communications and CDC control          ; UART and RS-232 serial adapter, modem, Wi-Fi adapter, Ethernet adapter. Used together with class 0Ah (CDC-Data) below
#define USB_DEV_CLASS_HID                     0x03 // Interface       ; Human interface device (HID)            ; Keyboard, mouse, joystick
#define USB_DEV_CLASS_PHYSICAL                0x05 // Interface       ; Physical interface device (PID)         ; Force feedback joystick
#define USB_DEV_CLASS_IMAGE                   0x06 // Interface       ; Media (PTP/MTP)                         ; Scanner, Camera
#define USB_DEV_CLASS_PRINTER                 0x07 // Interface       ; Printer                                 ; Laser printer, inkjet printer, CNC machine
#define USB_DEV_CLASS_MASS_STORAGE            0x08 // Interface       ; USB mass storage, USB Attached SCSI     ; USB flash drive, memory card reader, digital audio player, digital camera, external drive
#define USB_DEV_CLASS_HUB                     0x09 // Device          ; USB hub                                 ; High speed USB hub
#define USB_DEV_CLASS_CDC_DATA                0x0A // Interface       ; CDC-Data                                ; Used together with class 02h (Communications and CDC Control) above
#define USB_DEV_CLASS_SMART_CARD              0x0B // Interface       ; Smart card                              ; USB smart card reader
#define USB_DEV_CLASS_CONTENT_SECURITY        0x0D // Interface       ; Content security                        ; Fingerprint reader
#define USB_DEV_CLASS_VIDEO                   0x0E // Interface       ; Video                                   ; Webcam
#define USB_DEV_CLASS_PERSONAL_HEALTHCARE     0x0F // Interface       ; Personal healthcare device class (PHDC) ; Pulse monitor (watch)
#define USB_DEV_CLASS_AUDIO_VIDEO DEVICES     0x10 // Interface       ; Audio/Video (AV)                        ; Webcam, TV
#define USB_DEV_CLASS_BILLBOARD               0x11 // Device          ; Billboard                               ; Describes USB-C alternate modes supported by device
#define USB_DEV_CLASS_USB_TYPE_C_BRIDGE       0x12 // Interface       ;                                         ;
#define USB_DEV_CLASS_USB_BULK_DISPLAY        0x13 // Interface       ;                                         ;
#define USB_DEV_CLASS_MCTP_OVER_USB           0x14 // Interface       ;                                         ;
#define USB_DEV_CLASS_I3C_DEVICE_CLASS        0x3C // Interface       ;                                         ;
#define USB_DEV_CLASS_DIAGNOSTIC_DEVICE       0xDC // Both            ; Diagnostic device                       ; USB compliance testing device
#define USB_DEV_CLASS_WIRELESS_CONTROLLER     0xE0 // Interface       ; Wireless Controller                     ; Bluetooth adapter, Microsoft RNDIS
#define USB_DEV_CLASS_MISCELLANEOUS           0xEF // Both            ; Miscellaneous                           ; ActiveSync device
#define USB_DEV_CLASS_APPLICATION_SPECIFIC    0xFE // Interface       ; Application-specific                    ; IrDA Bridge, Test & Measurement Class (USBTMC), USB DFU (Device Firmware Upgrade)
#define USB_DEV_CLASS_VENDOR_SPECIFIC         0xFF // Both            ; Vendor-specific                         ; Indicates that a device needs vendor-specific drivers
#endif

/* USB endpoint type and attributes */
#ifndef USB_EP_TYPE_MASK
#define USB_EP_TYPE_CONTROL         0x00
#define USB_EP_TYPE_ISOCHRONOUS     0x01
#define USB_EP_TYPE_BULK            0x02
#define USB_EP_TYPE_INTERRUPT       0x03

#define USB_EP_DIR_MASK             0x80
#define USB_EP_ADDR_MASK            0x0F
#define USB_EP_TYPE_MASK            0x03
#endif

/* USB string index */
#ifndef USB_IDX_STR
#define USB_IDX_LANGID_STR      0x00
#define USB_IDX_MFC_STR         0x01
#define USB_IDX_PRODUCT_STR     0x02
#define USB_IDX_SERIAL_STR      0x03
#define USB_IDX_INTERFACE_CDC_CTRL_STR   0x04
#define USB_IDX_INTERFACE_CDC_DATA_STR   0x05
#define USB_IDX_INTERFACE_FIDO_STR       0x06
#define USB_IDX_INTERFACE_DEBUG_STR      0x07
#endif

#ifndef USB_DEVICE_ADDR
#define USB_DEVICE_ADDR         0x02
#endif
#ifndef DEFAULT_EP0_SIZE
#define DEFAULT_EP0_SIZE        64      /* Default maximum packet size for Endpoint 0 */
#endif
#ifndef DEFAULT_EP1_SIZE
#define DEFAULT_EP1_SIZE        8       /* Default maximum packet size for Endpoint 1 */
#endif
#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE         64      /* Maximum packet size */
#endif
#ifndef USB_BO_CBW_SIZE
#define USB_BO_CBW_SIZE         0x1F
#define USB_BO_CSW_SIZE         0x0D
#endif
#ifndef USB_BO_CBW_SIG0
#define USB_BO_CBW_SIG0         0x55
#define USB_BO_CBW_SIG1         0x53
#define USB_BO_CBW_SIG2         0x42
#define USB_BO_CBW_SIG3         0x43
#define USB_BO_CSW_SIG0         0x55
#define USB_BO_CSW_SIG1         0x53
#define USB_BO_CSW_SIG2         0x42
#define USB_BO_CSW_SIG3         0x53
#endif

typedef struct _USB_SETUP_REQ {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint8_t wValueL;
    uint8_t wValueH;
    uint8_t wIndexL;
    uint8_t wIndexH;
    uint8_t wLengthL;
    uint8_t wLengthH;
} USB_SETUP_REQ, *PUSB_SETUP_REQ;

typedef USB_SETUP_REQ XDATA *PXUSB_SETUP_REQ;

typedef struct _USB_DEVICE_DESCR {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bcdUSBL;
    uint8_t bcdUSBH;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t idVendorL;
    uint8_t idVendorH;
    uint8_t idProductL;
    uint8_t idProductH;
    uint8_t bcdDeviceL;
    uint8_t bcdDeviceH;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} USB_DEV_DESCR, *PUSB_DEV_DESCR;

typedef USB_DEV_DESCR XDATA *PXUSB_DEV_DESCR;

typedef struct _USB_CONFIG_DESCR {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t wTotalLengthL;
    uint8_t wTotalLengthH;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t MaxPower;
} USB_CFG_DESCR, *PUSB_CFG_DESCR;

typedef USB_CFG_DESCR XDATA *PXUSB_CFG_DESCR;

typedef struct _USB_INTERF_DESCR {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} USB_ITF_DESCR, *PUSB_ITF_DESCR;

typedef USB_ITF_DESCR XDATA *PXUSB_ITF_DESCR;

typedef struct _USB_ENDPOINT_DESCR {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint8_t wMaxPacketSizeL;
    uint8_t wMaxPacketSizeH;
    uint8_t bInterval;
} USB_ENDP_DESCR, *PUSB_ENDP_DESCR;

typedef USB_ENDP_DESCR XDATA *PXUSB_ENDP_DESCR;

typedef struct _USB_CONFIG_DESCR_LONG {
    USB_CFG_DESCR   cfg_descr;
    USB_ITF_DESCR   itf_descr;
    USB_ENDP_DESCR  endp_descr[1];
} USB_CFG_DESCR_LONG, *PUSB_CFG_DESCR_LONG;

typedef USB_CFG_DESCR_LONG XDATA *PXUSB_CFG_DESCR_LONG;

typedef struct _USB_HUB_DESCR {
    uint8_t bDescLength;
    uint8_t bDescriptorType;
    uint8_t bNbrPorts;
    uint8_t wHubCharacteristicsL;
    uint8_t wHubCharacteristicsH;
    uint8_t bPwrOn2PwrGood;
    uint8_t bHubContrCurrent;
    uint8_t DeviceRemovable;
    uint8_t PortPwrCtrlMask;
} USB_HUB_DESCR, *PUSB_HUB_DESCR;

typedef USB_HUB_DESCR XDATA *PXUSB_HUB_DESCR;

typedef struct _USB_HID_DESCR {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bcdHIDL;
    uint8_t bcdHIDH;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bDescriptorTypeX;
    uint8_t wDescriptorLengthL;
    uint8_t wDescriptorLengthH;
} USB_HID_DESCR, *PUSB_HID_DESCR;

typedef USB_HID_DESCR XDATA *PXUSB_HID_DESCR;

typedef struct _UDISK_BOC_CBW {           /* Command of BulkOnly USB-FlashDisk */
    uint8_t mCBW_Sig0;
    uint8_t mCBW_Sig1;
    uint8_t mCBW_Sig2;
    uint8_t mCBW_Sig3;
    uint8_t mCBW_Tag0;
    uint8_t mCBW_Tag1;
    uint8_t mCBW_Tag2;
    uint8_t mCBW_Tag3;
    uint8_t mCBW_DataLen0;
    uint8_t mCBW_DataLen1;
    uint8_t mCBW_DataLen2;
    uint8_t mCBW_DataLen3;                /* MSB byte of data length, always is 0 */
    uint8_t mCBW_Flag;                    /* Transfer direction and etc. */
    uint8_t mCBW_LUN;
    uint8_t mCBW_CB_Len;                  /* Length of command block */
    uint8_t mCBW_CB_Buf[16];              /* Command block buffer */
} UDISK_BOC_CBW, *PUDISK_BOC_CBW;

typedef UDISK_BOC_CBW XDATA *PXUDISK_BOC_CBW;

typedef struct _UDISK_BOC_CSW {           /* Status of BulkOnly USB-FlashDisk */
    uint8_t mCSW_Sig0;
    uint8_t mCSW_Sig1;
    uint8_t mCSW_Sig2;
    uint8_t mCSW_Sig3;
    uint8_t mCSW_Tag0;
    uint8_t mCSW_Tag1;
    uint8_t mCSW_Tag2;
    uint8_t mCSW_Tag3;
    uint8_t mCSW_Residue0;                /* Return: remainder bytes */
    uint8_t mCSW_Residue1;
    uint8_t mCSW_Residue2;
    uint8_t mCSW_Residue3;                /* MSB byte of remainder length, always is 0 */
    uint8_t mCSW_Status;                  /* Return: result status */
} UDISK_BOC_CSW, *PUDISK_BOC_CSW;

typedef UDISK_BOC_CSW XDATA *PXUDISK_BOC_CSW;

#endif  // __USB_DEF__
