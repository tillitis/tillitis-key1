//======================================================================
//
// SB_LUT4.v
// ---------
// Simulation model of the SB_LUT4 macro used to buil the sim target.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`default_nettype none

module SB_LUT4 (
    input  wire I0,
    output wire O
);

  parameter LUT_INIT = 16'h0;

  assign O = ~I0;

endmodule  // SB_LUT4

//======================================================================
// EOF SB_LUT4.v
//======================================================================
