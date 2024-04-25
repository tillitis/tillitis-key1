//======================================================================
//
// tb_SB_LUT4.v
// ---------
// Simulation model for the SB_LUT4 primitive.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module SB_LUT4 #(
	parameter [15 : 0] LUT_INIT = 16'b1100110011001100)
      (
       input wire I0,
       input wire I1,
       input wire I2,
       input wire I3,
       output wire O
      );

  assign O = LUT_INIT[{I3, I2, I1, I0}];

endmodule // SB_LUT4

//======================================================================
// EOF tb_SB_LUT4.v
//======================================================================
