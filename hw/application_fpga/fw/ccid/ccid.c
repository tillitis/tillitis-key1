/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdint.h>
#include <stdbool.h>

#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "../tk1/proto.h"

#include "ccid.h"

uint8_t global_bProtocolNum = 0;

#define NUM_PROTO 2 // Can be T0 or T1
#define NUM_SLOTS 3

// T=0 and T=1 Protocol
uint8_t g_bmFindexDindex[NUM_SLOTS][NUM_PROTO] = { 0 };
uint8_t g_bmTCCKS[NUM_SLOTS][NUM_PROTO] = { 0 };
uint8_t g_bGuardTime[NUM_SLOTS][NUM_PROTO] = { 0 };
uint8_t g_bWaitingInteger[NUM_SLOTS][NUM_PROTO] = { 0 };
uint8_t g_bClockStop[NUM_SLOTS][NUM_PROTO] = { 0 };

// Only T=1 Protocol
uint8_t g_bIFSC[NUM_SLOTS] = { 0 };
uint8_t g_bNadValue[NUM_SLOTS] = { 0 };

uint8_t g_bClockStatus = 0x0; // 00h = Clock running

// Done
void PC_to_RDR_IccPowerOn(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot           = cmdbuf[5];   // Slot number
	uint8_t bSeq            = cmdbuf[6];   // Sequence number
	uint8_t bPowerSelect    = cmdbuf[7];
	uint8_t abData[]        = {0x3B, 0xF8, 0x13, 0x00, 0x00, 0x81, 0x31, 0xFE, 0x15, 0x59, 0x75, 0x62, 0x69, 0x6B, 0x65, 0x79, 0x34, 0xD4}; // ATR
	uint32_t dwLength       = sizeof(abData);

	uint8_t bChainParameter = 0x0; // The response APDU begins and ends in this command
	uint8_t bStatus         = 0x0;
	uint8_t bError          = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	(void)bPowerSelect;

	error:
	// Send response
	RDR_to_PC_DataBlock(dwLength, bSlot, bSeq, bStatus, bError, bChainParameter, abData);
}

// Done
void PC_to_RDR_IccPowerOff(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot   = cmdbuf[5];
	uint8_t bSeq    = cmdbuf[6];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	error:
	// Send response
	RDR_to_PC_SlotStatus(bSlot, bSeq, bStatus, bError);
}

// Done
void PC_to_RDR_GetSlotStatus(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot   = cmdbuf[5];
	uint8_t bSeq    = cmdbuf[6];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	error:
	// Send response
	RDR_to_PC_SlotStatus(bSlot, bSeq, bStatus, bError);
}

void PC_to_RDR_XfrBlock(uint8_t *cmdbuf, uint8_t length)
{
	uint32_t dwLength        =  cmdbuf[1]        |
                               (cmdbuf[2] <<  8) |
                               (cmdbuf[3] << 16) |
                               (cmdbuf[4] << 24); // Size of abData field of this message (Little-endian format)
	uint8_t bSlot            =  cmdbuf[5];        // Slot number
	uint8_t bSeq             =  cmdbuf[6];        // Sequence number
	uint8_t bBWI             =  cmdbuf[7];        // Used to extend the CCIDs Block Waiting Timeout for this current transfer.
                                                  // The CCID will timeout the block after "this number multiplied by the Block Waiting Time" has expired.
	uint16_t wLevelParameter =  cmdbuf[8] |
                               (cmdbuf[9] << 8);  // Use changes depending on the exchange level reported by the class descriptor in dwFeatures field:
                                                  // Character level: Size of expected data to be returned by the bulk-IN endpoint,
                                                  // TPDU level, RFU, = 0000h
                                                  // Short APDU level, RFU, = 00000h
                                                  // Extended APDU level: Indicates if APDU begins or ends in this command:
                                                  // 0000h - the command APDU begins and ends with this command
                                                  // 0001h - the command APDU begins with this command, and continue in the next PC_to_RDR_XfrBlock
                                                  // 0002h - this abData field continues a command APDU and ends the APDU command
                                                  // 0003h - the abData field continues a command APDU and another block is to follow
                                                  // 0010h - empty abData field, continuation of response APDU is expected in the next RDR_to_PC_DataBlock
	uint8_t *abData         = &cmdbuf[10];
	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	uint32_t responseLength = 0x0;
	uint8_t bChainParameter = 0x0;
	uint8_t responseData[1] = { 0 };

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	// Send APDU to ICC and get response
	// TODO
	(void)bBWI;
	(void)wLevelParameter;
	(void)abData;
	(void)dwLength;

//	debug_puts("length = ");
//	debug_putinthex(dwLength);
//	debug_lf();
//	debug_hexdump(abData,dwLength);

	error:
	// Send response back to host
	RDR_to_PC_DataBlock(responseLength, bSlot, bSeq, bStatus, bError, bChainParameter, responseData);
}

// Done
void PC_to_RDR_GetParameters(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot   = cmdbuf[5];
	uint8_t bSeq    = cmdbuf[6];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	error:
	// Send response
	RDR_to_PC_Parameters(bSlot, bSeq, bStatus, bError, global_bProtocolNum);
}


void PC_to_RDR_ResetParameters(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot   = cmdbuf[5];
	uint8_t bSeq    = cmdbuf[6];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	global_bProtocolNum = 0x00;

	error:
	// Send response
	RDR_to_PC_Parameters(bSlot, bSeq, bStatus, bError, global_bProtocolNum);
}

// Done
void PC_to_RDR_SetParameters(uint8_t *cmdbuf, uint8_t length)
{
	uint32_t dwLength    =  cmdbuf[1]        |
                           (cmdbuf[2] <<  8) |
                           (cmdbuf[3] << 16) |
                           (cmdbuf[4] << 24); // Size of abData field of this message (Little-endian format)
	uint8_t bSlot        =  cmdbuf[5];
	uint8_t bSeq         =  cmdbuf[6];
	uint8_t bProtocolNum =  cmdbuf[7];
	uint16_t abRFU       =  cmdbuf[8] |
                           (cmdbuf[9] << 8); // Reserved for Future Use

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	if (bProtocolNum > 1) {
		bStatus = SET_BMICCSTATUS(0) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_PROTOCOL_INVALID_OR_NOT_SUPPORTED;
		goto error;
	}

	if ((bProtocolNum == 0) && (dwLength != 5)) {
		bStatus = SET_BMICCSTATUS(0) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_PROTOCOL_INVALID_OR_NOT_SUPPORTED;
		goto error;
	}

	if ((bProtocolNum == 1) && (dwLength != 7)) {
		bStatus = SET_BMICCSTATUS(0) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_PROTOCOL_INVALID_OR_NOT_SUPPORTED;
		goto error;
	}

	(void)abRFU;

	// T=0 Protocol
	if (bProtocolNum == 0) {
		g_bmFindexDindex[bSlot][0]  = cmdbuf[10];
		g_bmTCCKS[bSlot][0]         = cmdbuf[11];
		g_bGuardTime[bSlot][0]      = cmdbuf[12];
		g_bWaitingInteger[bSlot][0] = cmdbuf[13];
		g_bClockStop[bSlot][0]      = cmdbuf[14];
	}

	// T=1 Protocol
	else if (bProtocolNum == 1) {
		g_bmFindexDindex[bSlot][1]  = cmdbuf[10];
		g_bmTCCKS[bSlot][1]         = cmdbuf[11];
		g_bGuardTime[bSlot][1]      = cmdbuf[12];
		g_bWaitingInteger[bSlot][1] = cmdbuf[13];
		g_bClockStop[bSlot][1]      = cmdbuf[14];
		g_bIFSC[bSlot]              = cmdbuf[15];
		g_bNadValue[bSlot]          = cmdbuf[16];
	}

	error:
	// Send response
	RDR_to_PC_Parameters(bSlot, bSeq, bStatus, bError, bProtocolNum);
}

void PC_to_RDR_Escape(uint8_t *cmdbuf, uint8_t length)
{
	// RDR_to_PC_Escape
}

void PC_to_RDR_IccClock(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot         = cmdbuf[5];
	uint8_t bSeq          = cmdbuf[6];
	uint8_t bClockCommand = cmdbuf[7];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	if (g_bClockStop[bSlot][0] != CLOCK_STOP_IS_NOT_ALLOWED) {
		if (bClockCommand == CLOCK_COMMAND_STOP) {
			switch (g_bClockStop[bSlot][0]) {
				case CLOCK_STOP_WITH_SIGNAL_LOW:
					g_bClockStatus = CLOCK_STATUS_STOPPED_IN_STATE_L;
					break;
				case CLOCK_STOP_WITH_SIGNAL_HIGH:
					g_bClockStatus = CLOCK_STATUS_STOPPED_IN_STATE_H;
					break;
				case CLOCK_STOP_WITH_EITHER_HIGH_OR_LOW:
					g_bClockStatus = CLOCK_STATUS_STOPPED_IN_AN_UNKNOWN_STATE;
					break;
				default:
					break;
			}
		} else if (bClockCommand == CLOCK_COMMAND_RESTART) {
			g_bClockStatus = CLOCK_STATUS_RUNNING;
		}
	} else {
		bStatus = SET_BMICCSTATUS(1) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_BUSY_WITH_AUTO_SEQUENCE;
		goto error;
	}

	error:
	// Send response back to host
	RDR_to_PC_SlotStatus(bSlot, bSeq, bStatus, bError);
}

void PC_to_RDR_T0APDU(uint8_t *cmdbuf, uint8_t length)
{
	uint8_t bSlot   = cmdbuf[5];
	uint8_t bSeq    = cmdbuf[6];
	uint8_t bmChanges = cmdbuf[7];
	uint8_t bClassGetResponse = cmdbuf[8];
	uint8_t bClassEnvelope = cmdbuf[9];

	uint8_t bStatus = 0x0;
	uint8_t bError  = 0x0;

	if (bSlot > (NUM_SLOTS-1)) {
		bStatus = SET_BMICCSTATUS(2) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_SLOT_NOT_EXISTING;
		goto error;
	}

	if (bClassGetResponse != 0xFF) { // Protocol not managed
		bStatus = SET_BMICCSTATUS(1) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_ICC_PROTOCOL_NOT_SUPPORTED;
		goto error;
	}

	if (bClassEnvelope != 0xFF) { // Protocol not managed
		bStatus = SET_BMICCSTATUS(1) | SET_BMCOMMANDSTATUS(1);
		bError = ERROR_ICC_PROTOCOL_NOT_SUPPORTED;
		goto error;
	}

	// Send to ICC
	(void)bmChanges;

	error:
	// Send response back to host
	RDR_to_PC_SlotStatus(bSlot, bSeq, bStatus, bError);
}

void PC_to_RDR_Secure(uint8_t *cmdbuf, uint8_t length)
{
	// RDR_to_PC_DataBlock
}

void PC_to_RDR_Mechanical(uint8_t *cmdbuf, uint8_t length)
{
	// RDR_to_PC_SlotStatus
}

void PC_to_RDR_Abort(uint8_t *cmdbuf, uint8_t length)
{
	// RDR_to_PC_SlotStatus
}

void PC_to_RDR_SetDataRateAndClockFrequency(uint8_t *cmdbuf, uint8_t length)
{
	// RDR_to_PC_DataRateAndClockFrequency
}


void RDR_to_PC_DataBlock(uint32_t dwLength,
						uint8_t bSlot,
						uint8_t bSeq,
						uint8_t bStatus,
						uint8_t bError,
						uint8_t bChainParameter,
						uint8_t *abData)
{
	uint32_t buflen = RDR_TO_PC_DATABLOCK_LEN + dwLength;
	uint8_t respbuf[buflen];
	memset(respbuf, 0, buflen);

	// clang-format off
	respbuf[0] = RDR_TO_PC_DATA_BLOCK; // bMessageType: Bit 7 set indicates that a data block is being sent from the CCID
	respbuf[1] = BYTE0(dwLength);      // dwLength: Size of abData field of this message
	respbuf[2] = BYTE1(dwLength);      // dwLength: Size of abData field of this message
	respbuf[3] = BYTE2(dwLength);      // dwLength: Size of abData field of this message
	respbuf[4] = BYTE3(dwLength);      // dwLength: Size of abData field of this message
	respbuf[5] = bSlot;                // bSlot: Identifies the slot number for this command
	respbuf[6] = bSeq;                 // bSeq: Sequence number for the corresponding command
	respbuf[7] = bStatus;              // bStatus:
                                       //         bmCommandStatus: D7..6: 0 - Processed without error
                                       //                                 1 - Failed (error code provided by the error register)
                                       //                                 2 - Time Extension is requested
                                       //                                 3 - RFU
                                       //         bmRFU:           D5..2: RFU
                                       //         bmICCStatus:     D1..0: 0 - An ICC is present and active (power is on and stable, RST is inactive)
                                       //                                 1 - An ICC is present and inactive (not activated or shut down by hardware error)
                                       //                                 2 - No ICC is present
                                       //                                 3 - RFU */
	respbuf[8] = bError;               // bError: 0xFF - CMD_ABORTED      - Host aborted the current activity
                                       //         0xFE - ICC_MUTE         - CCID timed out while talking to the ICC
                                       //         0xFD - XFR_PARITY_ERROR - Parity error while talking to the ICC
                                       //         0xFC - XFR_OVERRUN      - Overrun error while talking to the ICC
                                       //         0xFB - HW_ERROR         - An all inclusive hardware error occurred
                                       //         0xF8 - BAD_ATR_TS
                                       //         0xF7 - BAD_ATR_TCK
                                       //         0xF6 - ICC_PROTOCOL_NOT_SUPPORTED
                                       //         0xF5 - ICC_CLASS_NOT_SUPPORTED
                                       //         0xF4 - PROCEDURE_BYTE_CONFLICT
                                       //         0xF3 - DEACTIVATED_PROTOCOL
                                       //         0xF2 - BUSY_WITH_AUTO_SEQUENCE - Automatic Sequence Ongoing
                                       //         0xF0 - PIN_TIMEOUT
                                       //         0xEF - PIN_CANCELLED
                                       //         0xE0 - CMD_SLOT_BUSY - A second command was sent to a slot which was already processing a command.
                                       //         0xC0 to 0x81         - User Defined
                                       //         0x80                 - Reserved for future use
                                       //         0x7F to 0x01         - Index of not supported / incorrect message parameter
                                       //         0x00                 - Command not supported
	respbuf[9] = bChainParameter;      // bChainParameter: Depends on the exchange level reported by the class descriptor in dwFeatures field:
                                       //                  For "Character level", "TPDU level" and "Short APDU level", this field is RFU and =00h.
                                       //                  Extended APDU level, indicates if the response is complete, to be continued or if the
                                       //                  command APDU can continue.
                                       //                  0x00: The response APDU begins and ends in this command
                                       //                  0x01: The response APDU begins with this command and is to continue
                                       //                  0x02: This abData field continues the response APDU and ends the response APDU
                                       //                  0x03: This abData field continues the response APDU and another block is to follow
                                       //                  0x10: Empty abData field, continuation of the command APDU is expected in next
                                       //                        PC_to_RDR_XfrBlock command.
	memcpy(respbuf + RDR_TO_PC_DATABLOCK_LEN, abData, dwLength); // abData: This field contains the data returned by the CCID.
                                                                 //         Depending on the exchange level, this may be data and status “as is” from the ICC,
                                                                 //         or the CCID may “filter” the data and status before sending it to the host.
                                                                 //         It can be 0-65,538 bytes in length.
	// clang-format on

	write(IO_CCID, respbuf, buflen);
}

void RDR_to_PC_SlotStatus(uint8_t bSlot,
		uint8_t bSeq,
		uint8_t bStatus,
		uint8_t bError)
{
	uint32_t buflen = RDR_TO_PC_SLOT_STATUS_LEN;
	uint8_t respbuf[buflen];
	memset(respbuf, 0, buflen);

	// clang-format off
	respbuf[0] = RDR_TO_PC_SLOT_STATUS; // bMessageType: Bit 7 set indicates that a data block is being sent from the CCID
	respbuf[1] = 0x0;                   // dwLength: 0
	respbuf[2] = 0x0;                   // dwLength: 0
	respbuf[3] = 0x0;                   // dwLength: 0
	respbuf[4] = 0x0;                   // dwLength: 0
	respbuf[5] = bSlot;                 // bSlot: Identifies the slot number for this command
	respbuf[6] = bSeq;                  // bSeq: Sequence number for the corresponding command
	respbuf[7] = bStatus;               // bStatus:
                                        //         bmCommandStatus: D7..6: 0 - Processed without error
                                        //                                 1 - Failed (error code provided by the error register)
                                        //                                 2 - Time Extension is requested
                                        //                                 3 - RFU
                                        //         bmRFU:           D5..2: RFU
                                        //         bmICCStatus:     D1..0: 0 - An ICC is present and active (power is on and stable, RST is inactive)
                                        //                                 1 - An ICC is present and inactive (not activated or shut down by hardware error)
                                        //                                 2 - No ICC is present
                                        //                                 3 - RFU */
	respbuf[8] = bError;                // bError: 0xFF - CMD_ABORTED      - Host aborted the current activity
                                        //         0xFE - ICC_MUTE         - CCID timed out while talking to the ICC
                                        //         0xFD - XFR_PARITY_ERROR - Parity error while talking to the ICC
                                        //         0xFC - XFR_OVERRUN      - Overrun error while talking to the ICC
                                        //         0xFB - HW_ERROR         - An all inclusive hardware error occurred
                                        //         0xF8 - BAD_ATR_TS
                                        //         0xF7 - BAD_ATR_TCK
                                        //         0xF6 - ICC_PROTOCOL_NOT_SUPPORTED
                                        //         0xF5 - ICC_CLASS_NOT_SUPPORTED
                                        //         0xF4 - PROCEDURE_BYTE_CONFLICT
                                        //         0xF3 - DEACTIVATED_PROTOCOL
                                        //         0xF2 - BUSY_WITH_AUTO_SEQUENCE - Automatic Sequence Ongoing
                                        //         0xF0 - PIN_TIMEOUT
                                        //         0xEF - PIN_CANCELLED
                                        //         0xE0 - CMD_SLOT_BUSY - A second command was sent to a slot which was already processing a command.
                                        //         0xC0 to 0x81         - User Defined
                                        //         0x80                 - Reserved for future use
                                        //         0x7F to 0x01         - Index of not supported / incorrect message parameter
                                        //         0x00                 - Command not supported
	respbuf[9] = g_bClockStatus;        // bClockStatus: value = 00h Clock running
                                        //                       01h Clock stopped in state L
                                        //                       02h Clock stopped in state H
                                        //                       03h Clock stopped in an unknown state
                                        //                       All other values are RFU. */
	// clang-format on

	write(IO_CCID, respbuf, buflen);
}

void RDR_to_PC_Parameters(uint8_t bSlot,
		uint8_t bSeq,
		uint8_t bStatus,
		uint8_t bError,
		uint8_t bProtocolNum)
{
	uint32_t dwLength = (bProtocolNum == 0) ? RDR_TO_PC_PARAMETERS_T0_LEN : RDR_TO_PC_PARAMETERS_T1_LEN;
	uint32_t buflen = RDR_TO_PC_PARAMETERS_LEN + dwLength;
	uint8_t respbuf[buflen];
	memset(respbuf, 0, buflen);

	// clang-format off
	respbuf[0] = RDR_TO_PC_PARAMETERS; // bMessageType
	respbuf[1] = BYTE0(dwLength);      // dwLength: Size of abProtocolDataStructure field of this message
	respbuf[2] = BYTE1(dwLength);      // dwLength: Size of abProtocolDataStructure field of this message
	respbuf[3] = BYTE2(dwLength);      // dwLength: Size of abProtocolDataStructure field of this message
	respbuf[4] = BYTE3(dwLength);      // dwLength: Size of abProtocolDataStructure field of this message
	respbuf[5] = bSlot;                // bSlot: Identifies the slot number for this command
	respbuf[6] = bSeq;                 // bSeq: Sequence number for the corresponding command
	respbuf[7] = bStatus;              // bStatus:
                                       //         bmCommandStatus: D7..6: 0 - Processed without error
                                       //                                 1 - Failed (error code provided by the error register)
                                       //                                 2 - Time Extension is requested
                                       //                                 3 - RFU
                                       //         bmRFU:           D5..2: RFU
                                       //         bmICCStatus:     D1..0: 0 - An ICC is present and active (power is on and stable, RST is inactive)
                                       //                                 1 - An ICC is present and inactive (not activated or shut down by hardware error)
                                       //                                 2 - No ICC is present
                                       //                                 3 - RFU */
	respbuf[8] = bError;               // bError: 0xFF - CMD_ABORTED      - Host aborted the current activity
                                       //         0xFE - ICC_MUTE         - CCID timed out while talking to the ICC
                                       //         0xFD - XFR_PARITY_ERROR - Parity error while talking to the ICC
                                       //         0xFC - XFR_OVERRUN      - Overrun error while talking to the ICC
                                       //         0xFB - HW_ERROR         - An all inclusive hardware error occurred
                                       //         0xF8 - BAD_ATR_TS
                                       //         0xF7 - BAD_ATR_TCK
                                       //         0xF6 - ICC_PROTOCOL_NOT_SUPPORTED
                                       //         0xF5 - ICC_CLASS_NOT_SUPPORTED
                                       //         0xF4 - PROCEDURE_BYTE_CONFLICT
                                       //         0xF3 - DEACTIVATED_PROTOCOL
                                       //         0xF2 - BUSY_WITH_AUTO_SEQUENCE - Automatic Sequence Ongoing
                                       //         0xF0 - PIN_TIMEOUT
                                       //         0xEF - PIN_CANCELLED
                                       //         0xE0 - CMD_SLOT_BUSY - A second command was sent to a slot which was already processing a command.
                                       //         0xC0 to 0x81         - User Defined
                                       //         0x80                 - Reserved for future use
                                       //         0x7F to 0x01         - Index of not supported / incorrect message parameter
                                       //         0x00                 - Command not supported
	respbuf[9] = bProtocolNum;         // bProtocolNum: Specifies what protocol data structure follows.
                                       //               00h = Structure for protocol T=0
                                       //               01h = Structure for protocol T=1
                                       //               The following values are reserved for future use.
                                       //               80h = Structure for 2-wire protocol
                                       //               81h = Structure for 3-wire protocol
                                       //               82h = Structure for I2C protocol
	// T=0 Protocol
	if (bProtocolNum == 0) {
		respbuf[10] = g_bmFindexDindex[bSlot][0];  // bmFindexDindex: B7-4 - FI - Index into the table 7 in ISO/IEC 7816-3:1997 selecting a clock rate conversion factor
                                                   //                 B3-0 - DI - Index into the table 8 in ISO/IEC 7816-3:1997 selecting a baud rate conversion factor
		respbuf[11] = g_bmTCCKS[bSlot][0];         // bmTCCKST0: For T=0, Possible values: 0x00, 0x02
                                                   //                     B7-2 - 000000b
                                                   //                     B1   - Convention used (b1=0 for direct, b1=1 for inverse)
                                                   //                     B0   - 0b
		respbuf[12] = g_bGuardTime[bSlot][0];      // bGuardTimeT0: Possible values: 0x00-0xFF
                                                   //               Extra Guardtime between two characters.
                                                   //               Add 0 to 254 etu to the normal guardtime of 12 etu.
                                                   //               0xFF is the same as 0x00.
		respbuf[13] = g_bWaitingInteger[bSlot][0]; // bWaitingIntegerT0: Possible values: 0x00-0xFF
                                                   //                    WI for T=0 used to define WWT
		respbuf[14] = g_bClockStop[bSlot][0];      // bClockStop: ICC Clock Stop Support, Possible values: 0x00-0x03
                                                   //             00 = Stopping the Clock is not allowed
                                                   //             01 = Stop with Clock signal Low
                                                   //             02 = Stop with Clock signal High
                                                   //             03 = Stop with Clock either High or Low
	}
	// T=1 Protocol
	else if (bProtocolNum == 1) {
		respbuf[10] = g_bmFindexDindex[bSlot][1];  // bmFindexDindex: B7-4 - FI - Index into the table 7 in ISO/IEC 7816-3:1997 selecting a clock rate conversion factor
                                                   //                 B3-0 - DI - Index into the table 8 in ISO/IEC 7816-3:1997 selecting a baud rate conversion factor
		respbuf[11] = g_bmTCCKS[bSlot][1];         // bmTCCKST1: For T=1, Possible values: 0x10, 0x11, 0x12, 0x13
                                                   //                     B7-2 - 000100b
                                                   //                     B1 - Convention used (b1=0 for direct, b1=1 for inverse)
                                                   //                     B0 - Checksum type (b0=0 for LRC, b0=1 for CRC)
		respbuf[12] = g_bGuardTime[bSlot][1];      // bGuardTimeT1: Possible values: 0x00-0xFF
                                                   //               Extra Guardtime (0 to 254 etu between two characters).
                                                   //               If value is 0xFF, then guardtime is reduced by 1.
		respbuf[13] = g_bWaitingInteger[bSlot][1]; // bmWaitingIntegersT1: Possible values: 0x00-0x9F
                                                   //                      B7-4 = BWI
                                                   //                      B3-0 = CWI
		respbuf[14] = g_bClockStop[bSlot][1];      // bClockStop: ICC Clock Stop Support, Possible values: 0x00-0x03
                                                   //             00 = Stopping the Clock is not allowed
                                                   //             01 = Stop with Clock signal Low
                                                   //             02 = Stop with Clock signal High
                                                   //             03 = Stop with Clock either High or Low
		respbuf[15] = g_bIFSC[bSlot];              // bIFSC: Size of negotiated IFSC, Possible values: 0x00-0xFE
		respbuf[16] = g_bNadValue[bSlot];          // bNadValue: Nad value used by CCID, Possible values: 0x00-0xFF
	}
	// clang-format on

	write(IO_CCID, respbuf, buflen);
}

void RDR_to_PC_Escape(uint32_t dwLength,
		uint8_t bSlot,
		uint8_t bSeq,
		uint8_t bStatus,
		uint8_t bError,
		uint8_t *abData)
{
	uint32_t buflen = RDR_TO_PC_ESCAPE_LEN + dwLength;
	uint8_t respbuf[buflen];
	memset(respbuf, 0, buflen);

	// clang-format off
	respbuf[0] = RDR_TO_PC_ESCAPE; // bMessageType: Indicates that a data block is being sent from the CCID
	respbuf[1] = BYTE0(dwLength);  // dwLength: Size of abData field of this message
	respbuf[2] = BYTE1(dwLength);  // dwLength: Size of abData field of this message
	respbuf[3] = BYTE2(dwLength);  // dwLength: Size of abData field of this message
	respbuf[4] = BYTE3(dwLength);  // dwLength: Size of abData field of this message
	respbuf[5] = bSlot;            // bSlot: Identifies the slot number for this command
	respbuf[6] = bSeq;             // bSeq: Sequence number for the corresponding command
	respbuf[7] = bStatus;          // bStatus:
                                   //         bmCommandStatus: D7..6: 0 - Processed without error
                                   //                                 1 - Failed (error code provided by the error register)
                                   //                                 2 - Time Extension is requested
                                   //                                 3 - RFU
                                   //         bmRFU:           D5..2: RFU
                                   //         bmICCStatus:     D1..0: 0 - An ICC is present and active (power is on and stable, RST is inactive)
                                   //                                 1 - An ICC is present and inactive (not activated or shut down by hardware error)
                                   //                                 2 - No ICC is present
                                   //                                 3 - RFU */
	respbuf[8] = bError;           // bError: 0xFF - CMD_ABORTED      - Host aborted the current activity
                                   //         0xFE - ICC_MUTE         - CCID timed out while talking to the ICC
                                   //         0xFD - XFR_PARITY_ERROR - Parity error while talking to the ICC
                                   //         0xFC - XFR_OVERRUN      - Overrun error while talking to the ICC
                                   //         0xFB - HW_ERROR         - An all inclusive hardware error occurred
                                   //         0xF8 - BAD_ATR_TS
                                   //         0xF7 - BAD_ATR_TCK
                                   //         0xF6 - ICC_PROTOCOL_NOT_SUPPORTED
                                   //         0xF5 - ICC_CLASS_NOT_SUPPORTED
                                   //         0xF4 - PROCEDURE_BYTE_CONFLICT
                                   //         0xF3 - DEACTIVATED_PROTOCOL
                                   //         0xF2 - BUSY_WITH_AUTO_SEQUENCE - Automatic Sequence Ongoing
                                   //         0xF0 - PIN_TIMEOUT
                                   //         0xEF - PIN_CANCELLED
                                   //         0xE0 - CMD_SLOT_BUSY - A second command was sent to a slot which was already processing a command.
                                   //         0xC0 to 0x81         - User Defined
                                   //         0x80                 - Reserved for future use
                                   //         0x7F to 0x01         - Index of not supported / incorrect message parameter
                                   //         0x00                 - Command not supported
	respbuf[9] = 0x0;              // bRFU: Reserved for Future Use, Possible values: 0x00
	memcpy(respbuf + RDR_TO_PC_ESCAPE_LEN, abData, dwLength); // abData: Data sent from CCID
	// clang-format on

	write(IO_CCID, respbuf, buflen);
}

void RDR_to_PC_DataRateAndClockFrequency(uint8_t bSlot,
		uint8_t bSeq,
		uint8_t bStatus,
		uint8_t bError,
		uint32_t dwClockFrequency,
		uint32_t dwDataRate)
{
	uint32_t dwLength = 8;
	uint32_t buflen = RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY_LEN;
	uint8_t respbuf[buflen];
	memset(respbuf, 0, buflen);

	// clang-format off
	respbuf[0] = RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY; // bMessageType: Bit 7 set indicates that a data block is being sent from the CCID
	respbuf[1] = BYTE0(dwLength);          // dwLength: Message-specific data length
	respbuf[2] = BYTE1(dwLength);          // dwLength: Message-specific data length
	respbuf[3] = BYTE2(dwLength);          // dwLength: Message-specific data length
	respbuf[4] = BYTE3(dwLength);          // dwLength: Message-specific data length
	respbuf[5] = bSlot;                    // bSlot: Identifies the slot number for this command
	respbuf[6] = bSeq;                     // bSeq: Sequence number for the corresponding command
	respbuf[7] = bStatus;                  // bStatus:
                                           //         bmCommandStatus: D7..6: 0 - Processed without error
                                           //                                 1 - Failed (error code provided by the error register)
                                           //                                 2 - Time Extension is requested
                                           //                                 3 - RFU
                                           //         bmRFU:           D5..2: RFU
                                           //         bmICCStatus:     D1..0: 0 - An ICC is present and active (power is on and stable, RST is inactive)
                                           //                                 1 - An ICC is present and inactive (not activated or shut down by hardware error)
                                           //                                 2 - No ICC is present
                                           //                                 3 - RFU */
	respbuf[8] = bError;                   // bError: 0xFF - CMD_ABORTED      - Host aborted the current activity
                                           //         0xFE - ICC_MUTE         - CCID timed out while talking to the ICC
                                           //         0xFD - XFR_PARITY_ERROR - Parity error while talking to the ICC
                                           //         0xFC - XFR_OVERRUN      - Overrun error while talking to the ICC
                                           //         0xFB - HW_ERROR         - An all inclusive hardware error occurred
                                           //         0xF8 - BAD_ATR_TS
                                           //         0xF7 - BAD_ATR_TCK
                                           //         0xF6 - ICC_PROTOCOL_NOT_SUPPORTED
                                           //         0xF5 - ICC_CLASS_NOT_SUPPORTED
                                           //         0xF4 - PROCEDURE_BYTE_CONFLICT
                                           //         0xF3 - DEACTIVATED_PROTOCOL
                                           //         0xF2 - BUSY_WITH_AUTO_SEQUENCE - Automatic Sequence Ongoing
                                           //         0xF0 - PIN_TIMEOUT
                                           //         0xEF - PIN_CANCELLED
                                           //         0xE0 - CMD_SLOT_BUSY - A second command was sent to a slot which was already processing a command.
                                           //         0xC0 to 0x81         - User Defined
                                           //         0x80                 - Reserved for future use
                                           //         0x7F to 0x01         - Index of not supported / incorrect message parameter
                                           //         0x00                 - Command not supported
	respbuf[9] = 0x0;                      // bRFU: Reserved for Future Use, Possible values: 0x00
	respbuf[10] = BYTE0(dwClockFrequency); // dwClockFrequency: Current setting of the ICC clock frequency in KHz. This is an integer value
	respbuf[11] = BYTE1(dwClockFrequency); // dwClockFrequency: Current setting of the ICC clock frequency in KHz. This is an integer value
	respbuf[12] = BYTE2(dwClockFrequency); // dwClockFrequency: Current setting of the ICC clock frequency in KHz. This is an integer value
	respbuf[13] = BYTE3(dwClockFrequency); // dwClockFrequency: Current setting of the ICC clock frequency in KHz. This is an integer value
	respbuf[14] = BYTE0(dwDataRate);       // dwDataRate: Current setting of the ICC data rate in bps. This is an integer value
	respbuf[10] = BYTE1(dwDataRate);       // dwDataRate: Current setting of the ICC data rate in bps. This is an integer value
	respbuf[10] = BYTE2(dwDataRate);       // dwDataRate: Current setting of the ICC data rate in bps. This is an integer value
	respbuf[10] = BYTE3(dwDataRate);       // dwDataRate: Current setting of the ICC data rate in bps. This is an integer value
	// clang-format on

	write(IO_CCID, respbuf, buflen);
}

void send_error_response(uint8_t errorCode)
{
	uint8_t bSlot = 0;  // Assuming single-slot operation
	uint8_t bSeq = 0;  // Sequence number can be 0 (unknown)
	uint8_t bStatus = SET_BMICCSTATUS(1) | SET_BMCOMMANDSTATUS(1);  // Error status
	uint8_t bChainParameter = 0;  // No chaining
	uint8_t responseData[1] = { 0 }; // No data, just an error response
	uint32_t responseLength = 0; // No additional data

	// Send an error response using the standard RDR_to_PC_DataBlock format
	RDR_to_PC_DataBlock(responseLength, bSlot, bSeq, bStatus, errorCode, bChainParameter, responseData);
}

void handle_ccid(uint8_t *cmdbuf, uint8_t length)
{
	// Ensure command buffer is valid and has enough data
	if (length < 10) {
		//send_error_response(ERROR_CMD_ABORTED);
		return;
	}

	switch (cmdbuf[0]) {
		case PC_TO_RDR_ICC_POWER_ON:
			PC_to_RDR_IccPowerOn(cmdbuf, length);
			break;

		case PC_TO_RDR_ICC_POWER_OFF:
			PC_to_RDR_IccPowerOff(cmdbuf, length);
			break;

		case PC_TO_RDR_GET_SLOT_STATUS:
			PC_to_RDR_GetSlotStatus(cmdbuf, length);
			break;

		case PC_TO_RDR_XFR_BLOCK:
			PC_to_RDR_XfrBlock(cmdbuf, length);
			break;

		case PC_TO_RDR_GET_PARAMETERS:
			PC_to_RDR_GetParameters(cmdbuf, length);
			break;

		case PC_TO_RDR_RESET_PARAMETERS:
			PC_to_RDR_ResetParameters(cmdbuf, length);
			break;

		case PC_TO_RDR_SET_PARAMETERS:
			PC_to_RDR_SetParameters(cmdbuf, length);
			break;

		case PC_TO_RDR_ESCAPE:
			PC_to_RDR_Escape(cmdbuf, length);
			break;

		case PC_TO_RDR_ICC_CLOCK:
			PC_to_RDR_IccClock(cmdbuf, length);
			break;

		case PC_TO_RDR_T0_APDU:
			PC_to_RDR_T0APDU(cmdbuf, length);
			break;

		case PC_TO_RDR_SECURE:
			PC_to_RDR_Secure(cmdbuf, length);
			break;

		case PC_TO_RDR_MECHANICAL:
			PC_to_RDR_Mechanical(cmdbuf, length);
			break;

		case PC_TO_RDR_ABORT:
			PC_to_RDR_Abort(cmdbuf, length);
			break;

		case PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY:
			PC_to_RDR_SetDataRateAndClockFrequency(cmdbuf, length);
			break;

		default:
			break;
	}
}
