/********************************** (C) COPYRIGHT *******************************
* File Name          : CDC.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/03/01
* Description        : CH554 as CDC device to serial port, select serial port 1
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ch554.h>
#include <ch554_usb.h>
#include <debug.h>

__xdata __at (0x0000) uint8_t  Ep0Buffer[DEFAULT_ENDP0_SIZE];      // Endpoint 0 OUT & IN buffer, must be an even address
__xdata __at (0x0040) uint8_t  Ep1Buffer[DEFAULT_ENDP1_SIZE];       //Endpoint 1 upload buffer
__xdata __at (0x0080) uint8_t  Ep2Buffer[2*MAX_PACKET_SIZE];        //Endpoint 2 IN & OUT buffer, must be an even address

uint16_t SetupLen;
uint8_t   SetupReq,Count,UsbConfig;
const uint8_t *  pDescr;                                                       //USB configuration flag
USB_SETUP_REQ   SetupReqBuf;                                                   //Temporary Setup package
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

#define  SET_LINE_CODING                0X20            // Configures DTE rate, stop-bits, parity, and number-of-character
#define  GET_LINE_CODING                0X21            // This request allows the host to find out the currently configured line coding.
#define  SET_CONTROL_LINE_STATE         0X22            // This request generates RS-232/V.24 style control signals.


/*设备描述符*/
__code uint8_t DevDesc[] = {0x12,0x01,0x10,0x01,0x02,0x00,0x00,DEFAULT_ENDP0_SIZE,
//                            0x86,0x1a,  // VID
//                            0x22,0x57,  // PID
                            0x07,0x12,  // VID
                            0x87,0x88,  // PID
                            0x00,0x01,0x01,0x02,
                            0x03,0x01
                           };
__code uint8_t CfgDesc[] ={
    0x09,0x02,0x43,0x00,0x02,0x01,0x00,0xa0,0x32,             //Configuration descriptor (two interfaces)
// The following is the interface 0 (CDC interface) descriptor
    0x09,0x04,0x00,0x00,0x01,0x02,0x02,0x01,0x00, // CDC interface descriptor (one endpoint)
    //The following is the function descriptor
    0x05,0x24,0x00,0x10,0x01,                                 //Function descriptor (header)
    0x05,0x24,0x01,0x00,0x00,                                 //Management descriptor (no data interface) 03 01
    0x04,0x24,0x02,0x02,                                      //stand by,Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State
    0x05,0x24,0x06,0x00,0x01,                                 //CDC interface numbered 0; data interface numbered 1
    0x07,0x05,0x81,0x03,0x08,0x00,0xFF,                       //Interrupt upload endpoint descriptor
    //The following is the interface 1 (data interface) descriptor
    0x09,0x04,0x01,0x00,0x02,0x0a,0x00,0x00,0x00,             //Data interface descriptor
    0x07,0x05,0x02,0x02,0x40,0x00,0x00,                       //Endpoint descriptor
    0x07,0x05,0x82,0x02,0x40,0x00,0x00,                       //Endpoint descriptor
};
/*字符串描述符*/
unsigned char  __code LangDes[]={0x04,0x03,0x09,0x04};           //Language descriptor

#include "usb_strings.h"

//unsigned char  __code SerDes[]={                                 //Serial number string descriptor
//                                                                 0x14,0x03,
//                                                                 0x32,0x00,0x30,0x00,0x31,0x00,0x37,0x00,0x2D,0x00,
//                                                                 0x32,0x00,0x2D,0x00,
//                                                                 0x32,0x00,0x35,0x00
//                               };
//unsigned char  __code Prod_Des[]={                                //Product string descriptor
//                                                                  0x14,0x03,
//                                                                  0x43,0x00,0x48,0x00,0x35,0x00,0x35,0x00,0x34,0x00,0x5F,0x00,
//                                                                  0x43,0x00,0x44,0x00,0x43,0x00,
//                                 };
//unsigned char  __code Manuf_Des[]={
//    0x0A,0x03,
//    0x5F,0x6c,0xCF,0x82,0x81,0x6c,0x52,0x60,
//};

//cdc参数
__xdata uint8_t LineCoding[7]={0x00,0xe1,0x00,0x00,0x00,0x00,0x08};   //The initial baud rate is 57600, 1 stop bit, no parity, 8 data bits.

#define UART_REV_LEN  64                 //Serial receive buffer size
__idata uint8_t Receive_Uart_Buf[UART_REV_LEN];   //Serial receive buffer
volatile __idata uint8_t Uart_Input_Point = 0;   //Circular buffer write pointer, bus reset needs to be initialized to 0
volatile __idata uint8_t Uart_Output_Point = 0;  //Take pointer out of circular buffer, bus reset needs to be initialized to 0
volatile __idata uint8_t UartByteCount = 0;      //Number of bytes remaining in the current buffer


volatile __idata uint8_t USBByteCount = 0;      //Represents the data received by the USB endpoint
volatile __idata uint8_t USBBufOutPoint = 0;    //Fetch data pointer

volatile __idata uint8_t UpPoint2_Busy  = 0;   //Whether the upload endpoint is busy


/*******************************************************************************
* Function Name  : USBDeviceCfg()
* Description    : USB device mode configuration
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceCfg()
{
    USB_CTRL = 0x00;                                                           //Clear USB control register
    USB_CTRL &= ~bUC_HOST_MODE;                                                //This bit selects the device mode
    USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB device and internal pull-up enable, automatically return to NAK before interrupt flag is cleared
    USB_DEV_AD = 0x00;                                                         //Device address initialization
    //     USB_CTRL |= bUC_LOW_SPEED;
    //     UDEV_CTRL |= bUD_LOW_SPEED;                                                //Select low speed 1.5M mode
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                             //Select full speed 12M mode, the default mode
    UDEV_CTRL = bUD_PD_DIS;  // Disable DP / DM pull-down resistor
    UDEV_CTRL |= bUD_PORT_EN;                                                  //Enable physical port
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
    USB_INT_EN |= bUIE_SUSPEND;                                               //Enable device suspend interrupt
    USB_INT_EN |= bUIE_TRANSFER;                                              //Enable USB transfer completion interrupt
    USB_INT_EN |= bUIE_BUS_RST;                                               //Enable device mode USB bus reset interrupt
    USB_INT_FG |= 0x1F;                                                       //Clear interrupt flag
    IE_USB = 1;                                                               //Enable USB interrupt
    EA = 1;                                                                   //Allow microcontroller interrupt
}
/*******************************************************************************
* Function Name  : USBDeviceEndPointCfg()
* Description    : USB device mode endpoint configuration, simulation compatible HID device, in addition to endpoint 0 control transmission, also includes endpoint 2 batch upload
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceEndPointCfg()
{
    // TODO: Is casting the right thing here? What about endianness?
    UEP1_DMA = (uint16_t) Ep1Buffer;                                                      //Endpoint 1 sends data transfer address
    UEP2_DMA = (uint16_t) Ep2Buffer;                                                      //Endpoint 2 IN data transfer address
    UEP2_3_MOD = 0xCC;                                                         //Endpoint 2/3 single buffer transceiver enable
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //Endpoint 2 automatically flips the synchronization flag, IN transaction returns NAK, OUT returns ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                                 //Endpoint 1 automatically flips the synchronization flag, IN transaction returns NAK
    UEP0_DMA = (uint16_t) Ep0Buffer;                                                      //Endpoint 0 data transfer address
    UEP4_1_MOD = 0X40;                                                         //Endpoint 1 upload buffer; endpoint 0 single 64-byte send and receive buffer
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //Manual flip, OUT transaction returns ACK, IN transaction returns NAK
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
    uint32_t uart1_buad = 0;
    *((uint8_t *)&uart1_buad) = cfg_uart[0];
    *((uint8_t *)&uart1_buad+1) = cfg_uart[1];
    *((uint8_t *)&uart1_buad+2) = cfg_uart[2];
    *((uint8_t *)&uart1_buad+3) = cfg_uart[3];
    SBAUD1 = 256 - FREQ_SYS/16/uart1_buad; //  SBAUD1 = 256 - Fsys / 16 / baud rate
    IE_UART1 = 1;
}
/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB interrupt processing function
*******************************************************************************/
void DeviceInterrupt(void) __interrupt (INT_NO_USB)                       //USB interrupt service routine, using register set 1
{
    uint16_t len;
    if(UIF_TRANSFER)                                                            //USB transfer complete flag
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 1:                                                  //endpoint 1# Endpoint interrupts upload
            UEP1_T_LEN = 0;
            UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Default answer NAK
            break;
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# Endpoint bulk upload
        {
            UEP2_T_LEN = 0;                                                    //The pre-used sending length must be cleared
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Default answer NAK
            UpPoint2_Busy = 0;                                                  //clear busy flag
        }
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 3# Endpoint batch download
            if ( U_TOG_OK )                                                     // Out-of-sync packets will be dropped
            {
                USBByteCount = USB_RX_LEN;                                      // Grads length of recieved data
                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //NAK after receiving a packet of data, the main function finishes processing, and the main function modifies the response mode
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                                //SETUP routine
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = ((uint16_t)UsbSetupBuf->wLengthH<<8) | (UsbSetupBuf->wLengthL);
                len = 0;                                                      // Defaults to success and uploading 0 length
                SetupReq = UsbSetupBuf->bRequest;
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )// non-standard request
                {
                    switch( SetupReq )
                    {
                    case GET_LINE_CODING:   //0x21  currently configured
                        pDescr = LineCoding;
                        len = sizeof(LineCoding);
                        len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;  // The length of this transmission
                        memcpy(Ep0Buffer,pDescr,len);
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case SET_CONTROL_LINE_STATE:  //0x22  generates RS-232/V.24 style control signals
                        break;
                    case SET_LINE_CODING:      //0x20  Configure
                        break;
                    default:
                        len = 0xFF;  								 					                 /*Command not supported*/
                        break;
                    }
                }
                else                                                             //Standard request
                {
                    switch(SetupReq)                                             //Request code
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                       // device descriptor
                            pDescr = DevDesc;                                         //Send the device descriptor to the buffer to be sent
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                        //configuration descriptor
                            pDescr = CfgDesc;                                          //Send the device descriptor to the buffer to be sent
                            len = sizeof(CfgDesc);
                            break;
                        case 3:
                            if(UsbSetupBuf->wValueL == 0)
                            {
                                pDescr = LangDes;
                                len = sizeof(LangDes);
                            }
                            else if(UsbSetupBuf->wValueL == 1)
                            {
                                pDescr = Manuf_Des;
                                len = sizeof(Manuf_Des);
                            }
                            else if(UsbSetupBuf->wValueL == 2)
                            {
                                pDescr = Prod_Des;
                                len = sizeof(Prod_Des);
                            }
                            else
                            {
                                pDescr = SerDes;
                                len = sizeof(SerDes);
                            }
                            break;
                        default:
                            len = 0xff;                                                //Unsupported command or error
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //Limit total length
                        }
                        len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;                            //This transmission length
                        memcpy(Ep0Buffer,pDescr,len);                                  //Load upload data
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                              //Temporary storage of USB device address
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;
                    case USB_GET_INTERFACE:
                        break;
                    case USB_CLEAR_FEATURE:                                            //Clear Feature
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* Remove device */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* Wake */
                                }
                                else
                                {
                                    len = 0xFF;                                        /* operation failed */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                            /* operation failed */
                            }
                        }
                        else if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// endpoint
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x83:
                                UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x03:
                                UEP3_CTRL = UEP3_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x01:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;                                         // Unsupported endpoint
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                                // It's not that the endpoint doesn't support it
                        }
                        break;
                    case USB_SET_FEATURE:                                          /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_DEVICE )                  /* Set up the device */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* hibernate */
#ifdef DE_PRINTF
                                    printf( "suspend\n" );                                                             //sleep state
#endif
                                    while ( XBUS_AUX & bUART0_TX )
                                    {
                                        ;    //等待发送完成
                                    }
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //USB or RXD0/1 can be woken up when there is a signal
                                    PCON |= PD;                                                                 // sleep
                                    SAFE_MOD = 0x55;
                                    SAFE_MOD = 0xAA;
                                    WAKE_CTRL = 0x00;
                                }
                                else
                                {
                                    len = 0xFF;                                        /* operation failed */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                            /* operation failed */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == USB_REQ_RECIP_ENDP )             /* Set endpoint */
                        {
                            if( ( ( ( uint16_t )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( uint16_t )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x83:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* Set endpoint 3 IN STALL */
                                    break;
                                case 0x03:
                                    UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 3 OUT Stall */
                                    break;
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* Set endpoint 2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* Set endpoint 1 IN STALL */
                                    break;
                                case 0x01:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* Set endpoint 1 OUT Stall */
                                default:
                                    len = 0xFF;                                    /* operation failed */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                      /* operation failed  */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                          /* operation failed */
                        }
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                    //operation failed
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                         //Packet length error
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= DEFAULT_ENDP0_SIZE)                                                       //Upload data or status phase returns 0 length packet
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//The default packet is DATA1，Return response ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //Although it has not yet reached the status stage, it is preset to upload 0-length data packets in advance to prevent the host from entering the status stage early.
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //The default data packet is DATA1, and the response ACK is returned
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= DEFAULT_ENDP0_SIZE ? DEFAULT_ENDP0_SIZE : SetupLen;                                 //The length of this transmission
                memcpy( Ep0Buffer, pDescr, len );                                   //Load upload data
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                             //Sync flag flip
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //The status phase is completed and interrupted or the 0-length data packet is forced to be uploaded to end the control transmission.
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            if(SetupReq ==SET_LINE_CODING)  // Set serial port properties
            {
                if( U_TOG_OK )
                {
                    memcpy(LineCoding,UsbSetupBuf,USB_RX_LEN);
                    Config_Uart1(LineCoding);
                    UEP0_T_LEN = 0;
                    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // Prepare to upload 0 packages
                }
            }
            else
            {
                UEP0_T_LEN = 0;
                UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_NAK;  // Status phase, responds to IN with NAK
            }
            break;



        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //Writing 0 clears the interrupt
    }
    if(UIF_BUS_RST)                                                                 //Device mode USB bus reset interrupt
    {
#ifdef DE_PRINTF
        printf( "reset\n" );                                                             //sleep state
#endif
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                             //clear interrupt flag
        Uart_Input_Point = 0;   //Circular buffer input pointer
        Uart_Output_Point = 0;  //Circular buffer read pointer
        UartByteCount = 0;      //The number of bytes remaining in the current buffer to be fetched
        USBByteCount = 0;       //USB endpoint received length
        UsbConfig = 0;          //Clear configuration values
        UpPoint2_Busy = 0;
    }
    if (UIF_SUSPEND)                                                                 //USB bus suspend/wake completed
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //hang
        {
#ifdef DE_PRINTF
            printf( "suspend\n" );                                                             //sleep state
#endif
            while ( XBUS_AUX & bUART0_TX )
            {
                ;    //Wait for sending to complete
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //Can be woken up when there is a signal from USB or RXD0/1
            PCON |= PD;                                                                 //sleep
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //Unexpected interruption, impossible situation
        USB_INT_FG = 0xFF;                                                             //clear interrupt flag

    }
}
/*******************************************************************************
* Function Name  : Uart1_ISR()
* Description    : Serial port receiving interrupt function to realize circular buffer receiving
*******************************************************************************/
void Uart1_ISR(void) __interrupt (INT_NO_UART1)
{
    if(U1RI)   //data received
    {
        Receive_Uart_Buf[Uart_Input_Point++] = SBUF1;
        if(Uart_Input_Point>=UART_REV_LEN) {
            Uart_Input_Point = 0;           //Write pointer
        }
        U1RI = 0;
    }

}

uint8_t uart_byte_count() {
    uint8_t in = Uart_Input_Point;
    uint8_t out = Uart_Output_Point;

    if (in < out) {
        in = in + UART_REV_LEN;
    }

    return in - out;
}

//main function
main()
{
    uint8_t length;
    uint8_t Uart_Timeout = 0;
    uint8_t USB_output_buffer[64] = {0};
    uint8_t USB_output_buffer_remain = 0;
    CfgFsys( );                                                           // CH559 clock selection configuration
    mDelaymS(5);                                                          // Modify the main frequency and wait for the internal crystal to stabilize, which must be added
    mInitSTDIO( );                                                        // Serial port 0, can be used for debugging
    UART1Setup( );                                                        // For CDC

#ifdef DE_PRINTF
    printf("start ...\n");
#endif
    USBDeviceCfg();
    USBDeviceEndPointCfg();                                               // Endpoint configuration
    USBDeviceIntCfg();                                                    //Interrupt initialization
    UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;                                                       //Pre-use send length must be cleared
    UEP2_T_LEN = 0;                                                       //Pre-use send length must be cleared

    // Enable GPIO debugging on p1.4 and p1.5
    // gpio_init();
    // gpio_unset(0x10);
    // gpio_unset(0x20);

    while(1)
    {
        if(UsbConfig)
        {
            if(USBByteCount)   // USB receiving endpoint has data
            {
                memcpy(USB_output_buffer, Ep2Buffer, USBByteCount);
                USB_output_buffer_remain = USBByteCount;
                USBBufOutPoint = 0;
                USBByteCount = 0;
            }

            if(USB_output_buffer_remain)
            {
                CH554UART1SendByte(USB_output_buffer[USBBufOutPoint++]);
                USB_output_buffer_remain--;

                if(USB_output_buffer_remain==0) {
                    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
                }
            }

            UartByteCount = uart_byte_count();
            if(UartByteCount) {
                Uart_Timeout++;
            }

            if(!UpPoint2_Busy)   // The endpoint is not busy (the first packet of data after idle, only used to trigger upload)
            {
                length = UartByteCount;
                if(length>0)
                {
                    if(length>39 || Uart_Timeout>100)
                    {

                        Uart_Timeout = 0;
                        // if we reach a wrap-around, just transmit from index to end of buffer.
                        // The rest goes in next packet, i.e., not handling wrap-around.
                        if(Uart_Output_Point+length>UART_REV_LEN) {
                            length = UART_REV_LEN-Uart_Output_Point;
                        }
                        // write upload endpoint
                        memcpy(Ep2Buffer+MAX_PACKET_SIZE,&Receive_Uart_Buf[Uart_Output_Point],length);

                        Uart_Output_Point+=length;

                        if (Uart_Output_Point>=UART_REV_LEN) {
                            Uart_Output_Point = 0;
                        }

                        UEP2_T_LEN = length; // Pre-use send length must be cleared
                        UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK; // Answer ACK
                        UpPoint2_Busy = 1;

                        // Should according to the USB-spec check if
			// length == 64, if so we should send a
			// zero-length USB packet. This is very
			// unlikley to happen.
                    }
                }
            }
	    // Should have a timeout if the transfer for some reason
	    // fails to reset UpPoint2_Busy. But does not seem to
	    // happen.
        }
    }
}
