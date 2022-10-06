//======================================================================
//
// firo.v
// ------
// Fibonacci Ring Oscillator with state sampling.
// The Fibonacci depth is 10 bits, and the bits are always sampled.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module firo(
            input wire  clk,
            output wire entropy
            );

  parameter POLY = 10'b1111111111;


  //----------------------------------------------------------------
  // Registers and wires.
  //----------------------------------------------------------------
  reg entropy_reg;

  /* verilator lint_off UNOPTFLAT */
  wire [10 : 0] f;
  /* verilator lint_on UNOPTFLAT */


  //---------------------------------------------------------------
  // Combinational loop inverters.
  //---------------------------------------------------------------
  /* verilator lint_off PINMISSING */
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv1  (.I0(f[0]), .O(f[1]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv2  (.I0(f[1]), .O(f[2]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv3  (.I0(f[2]), .O(f[3]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv4  (.I0(f[3]), .O(f[4]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv5  (.I0(f[4]), .O(f[5]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv6  (.I0(f[5]), .O(f[6]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv7  (.I0(f[6]), .O(f[7]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv8  (.I0(f[7]), .O(f[8]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv9  (.I0(f[8]), .O(f[9]));
  (* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv10 (.I0(f[9]), .O(f[10]));
  /* verilator lint_on PINMISSING */


  //---------------------------------------------------------------
  // parameterized feedback logic.
  //---------------------------------------------------------------
  assign f[0] = (POLY[0] & f[1]) ^ (POLY[1] & f[2]) ^
                (POLY[2] & f[3]) ^ (POLY[3] & f[4]) ^
                (POLY[4] & f[5]) ^ (POLY[5] & f[6]) ^
                (POLY[6] & f[7]) ^ (POLY[7] & f[8]) ^
                (POLY[8] & f[9]) ^ (POLY[9] & f[10]);


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign entropy = entropy_reg;


  //---------------------------------------------------------------
  // reg_update
  //---------------------------------------------------------------
  always @(posedge clk)
    begin : reg_update
      entropy_reg <= ^f;
    end

endmodule // firo

//======================================================================
// EOF firo.v
//======================================================================
