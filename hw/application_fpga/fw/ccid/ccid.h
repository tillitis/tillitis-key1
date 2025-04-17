/*
 * Copyright (C) 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CCID_H
#define CCID_H

#include <stdint.h>

#define BUFSIZE 64

#define BYTE0(x)   ((uint8_t)( (x)        & 0x000000FFU))  // Least significant byte
#define BYTE1(x)   ((uint8_t)(((x) >>  8) & 0x000000FFU))  // Second byte
#define BYTE2(x)   ((uint8_t)(((x) >> 16) & 0x000000FFU))  // Third byte
#define BYTE3(x)   ((uint8_t)(((x) >> 24) & 0x000000FFU))  // Most significant byte

#define SET_BMICCSTATUS(x)      ((uint8_t)(  (x) & 0x03U)      )
#define SET_BMCOMMANDSTATUS(x)  ((uint8_t)( ((x) & 0x03U) << 6))

// Incoming commands
#define PC_TO_RDR_ICC_POWER_ON 						0x62
#define PC_TO_RDR_ICC_POWER_OFF 					0x63
#define PC_TO_RDR_GET_SLOT_STATUS 					0x65
#define PC_TO_RDR_XFR_BLOCK 						0x6F
#define PC_TO_RDR_GET_PARAMETERS 					0x6C
#define PC_TO_RDR_RESET_PARAMETERS 					0x6D
#define PC_TO_RDR_SET_PARAMETERS 					0x61
#define PC_TO_RDR_ESCAPE 							0x6B
#define PC_TO_RDR_ICC_CLOCK 						0x6E
#define PC_TO_RDR_T0_APDU 							0x6A
#define PC_TO_RDR_SECURE 							0x69
#define PC_TO_RDR_MECHANICAL 						0x71
#define PC_TO_RDR_ABORT 							0x72
#define PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY 0x73

// Base length of incoming commands
#define PC_TO_RDR_ICC_POWER_ON_LEN						10
#define PC_TO_RDR_ICC_POWER_OFF_LEN						10
#define PC_TO_RDR_GET_SLOT_STATUS_LEN					10
#define PC_TO_RDR_XFR_BLOCK_LEN							10
#define PC_TO_RDR_GET_PARAMETERS_LEN					10
#define PC_TO_RDR_RESET_PARAMETERS_LEN					10
#define PC_TO_RDR_SET_PARAMETERS_LEN					10
#define PC_TO_RDR_SET_PARAMETERS_T0_LEN					5
#define PC_TO_RDR_SET_PARAMETERS_T1_LEN					7
#define PC_TO_RDR_ESCAPE_LEN							10
#define PC_TO_RDR_ICC_CLOCK_LEN							10
#define PC_TO_RDR_T0_APDU_LEN							10
#define PC_TO_RDR_SECURE_LEN							10
#define PC_TO_RDR_MECHANICAL_LEN						10
#define PC_TO_RDR_ABORT_LEN								10
#define PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY_LEN	18


// Outgoing commands
#define RDR_TO_PC_DATA_BLOCK						0x80
#define RDR_TO_PC_SLOT_STATUS						0x81
#define RDR_TO_PC_PARAMETERS						0x82
#define RDR_TO_PC_ESCAPE							0x83
#define RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY		0x84

// Base length of outgoing commands
#define RDR_TO_PC_DATABLOCK_LEN						10
#define RDR_TO_PC_SLOT_STATUS_LEN					10
#define RDR_TO_PC_PARAMETERS_LEN					10
#define RDR_TO_PC_PARAMETERS_T0_LEN					5
#define RDR_TO_PC_PARAMETERS_T1_LEN					7
#define RDR_TO_PC_ESCAPE_LEN						10
#define RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUENCY_LEN	18

// Error codes when bmCommandStatus = 1
#define ERROR_CMD_ABORTED                 0xFF
#define ERROR_ICC_MUTE                    0xFE
#define ERROR_XFR_PARITY_ERROR            0xFD
#define ERROR_XFR_OVERRUN                 0xFC
#define ERROR_HW_ERROR                    0xFB
#define ERROR_BAD_ATR_TS                  0xF8
#define ERROR_BAD_ATR_TCK                 0xF7
#define ERROR_ICC_PROTOCOL_NOT_SUPPORTED  0xF6
#define ERROR_ICC_CLASS_NOT_SUPPORTED     0xF5
#define ERROR_PROCEDURE_BYTE_CONFLICT     0xF4
#define ERROR_DEACTIVATED_PROTOCOL        0xF3
#define ERROR_BUSY_WITH_AUTO_SEQUENCE     0xF2
#define ERROR_PIN_TIMEOUT                 0xF0
#define ERROR_PIN_CANCELLED               0xEF
#define ERROR_CMD_SLOT_BUSY               0xE0
#define ERROR_SLOT_NOT_EXISTING           0x05
#define ERROR_COMMAND_NOT_SUPPORTED       0x00

// RDR_to_PC_DataBlock
#define ERROR_BAD_WLEVELPARAMETER         0x08
#define ERROR_BPOWERSELECT_ERROR          0x07

// RDR_to_PC_Parameters
#define ERROR_PROTOCOL_INVALID_OR_NOT_SUPPORTED                      0x07
#define ERROR_FI_DI_PAIR_INVALID_OR_NOT_SUPPORTED                    0x0A
#define ERROR_INVALID_TCCKTS_PARAMETER                               0x0B
#define ERROR_GUARD_TIME_NOT_SUPPORTED                               0x0C
#define ERROR_WI_INVALID_OR_NOT_SUPPORTED                            0x0D
#define ERROR_BWI_OR_CWI_INVALID_OR_NOT_SUPPORTED                    0x0D
#define ERROR_CLOCK_STOP_SUPPORT_REQUESTED_INVALID_OR_NOT_SUPPORTED  0x0E
#define ERROR_IFSC_SIZE_INVALID_OR_NOT_SUPPORTED                     0x0F
#define ERROR_NAD_VALUE_INVALID_OR_NOT_SUPPORTED                     0x10

// PC_to_RDR_SetParameters:bClockStop
#define CLOCK_STOP_IS_NOT_ALLOWED           0x00
#define CLOCK_STOP_WITH_SIGNAL_LOW          0x01
#define CLOCK_STOP_WITH_SIGNAL_HIGH         0x02
#define CLOCK_STOP_WITH_EITHER_HIGH_OR_LOW  0x03

// RDR_to_PC_SlotStatus:bClockStatus
#define CLOCK_STATUS_RUNNING                      0x00
#define CLOCK_STATUS_STOPPED_IN_STATE_L           0x01
#define CLOCK_STATUS_STOPPED_IN_STATE_H           0x02
#define CLOCK_STATUS_STOPPED_IN_AN_UNKNOWN_STATE  0x03

// PC_to_RDR_IccClock:bClockCommand
#define CLOCK_COMMAND_RESTART  0x00
#define CLOCK_COMMAND_STOP     0x01  // Stop in the state shown in the bClockStop field of the PC_to_RDR_SetParameters command and RDR_to_PC_Parameters message.

void PC_to_RDR_IccPowerOn(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_IccPowerOff(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_GetSlotStatus(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_XfrBlock(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_GetParameters(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_ResetParameters(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_SetParameters(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_Escape(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_IccClock(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_T0APDU(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_Secure(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_Mechanical(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_Abort(uint8_t *cmdbuf, uint8_t length);
void PC_to_RDR_SetDataRateAndClockFrequency(uint8_t *cmdbuf, uint8_t length);

void RDR_to_PC_DataBlock(uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError, uint8_t bChainParameter, uint8_t *abData);
void RDR_to_PC_SlotStatus(uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError);
void RDR_to_PC_Parameters(uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError, uint8_t bProtocolNum);
void RDR_to_PC_Escape(uint32_t dwLength, uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError, uint8_t *abData);
void RDR_to_PC_DataRateAndClockFrequency(uint8_t bSlot, uint8_t bSeq, uint8_t bStatus, uint8_t bError, uint32_t dwClockFrequency, uint32_t dwDataRate);

void handle_ccid(uint8_t *cmdbuf, uint8_t length);

#endif
