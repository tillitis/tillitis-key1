/*
 * Copyright (C) 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

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
