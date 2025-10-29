// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STATE_H
#define STATE_H

enum state {
	FW_STATE_INITIAL,
	FW_STATE_WAITCOMMAND,
	FW_STATE_LOADING,
	FW_STATE_LOAD_FLASH,
	FW_STATE_LOAD_FLASH_MGMT,
	FW_STATE_START,
	FW_STATE_FAIL,
	FW_STATE_MAX,
};
#endif
