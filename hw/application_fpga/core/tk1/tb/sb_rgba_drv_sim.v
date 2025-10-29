//======================================================================
//
// sb_rgba_drv_sim.v
// -------------
// Dummy version of the SB_RGBA_DRV hard macro in Lattice iCE40 UP
// devices. This is just to be able to build the testbench. The only
// functionality we need is to be able to set the LEDs.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2023 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

`default_nettype none

module SB_RGBA_DRV (
    input wire RGBLEDEN,
    input wire RGB0PWM,
    input wire RGB1PWM,
    input wire RGB2PWM,

    /* verilator lint_off UNUSEDSIGNAL */
    input wire CURREN,
    /* verilator lint_on UNUSEDSIGNAL */

    output wire RGB0,
    output wire RGB1,
    output wire RGB2
);

  /* verilator lint_off UNUSEDPARAM */
  parameter CURRENT_MODE = 1;
  parameter RGB0_CURRENT = 8'h0;
  parameter RGB1_CURRENT = 8'h0;
  parameter RGB2_CURRENT = 8'h0;
  /* verilator lint_on UNUSEDPARAM */

  assign RGB0 = RGB0PWM & RGBLEDEN;
  assign RGB1 = RGB1PWM & RGBLEDEN;
  assign RGB2 = RGB2PWM & RGBLEDEN;

endmodule  // SB_RGBA_DRV

//======================================================================
// EOF sb_rgba_drv_sim.v
//======================================================================
