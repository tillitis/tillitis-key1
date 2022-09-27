//======================================================================
//
// clk_reset_gen.v
// -----------
// Clock and reset generator used in the Tillitis Key 1 design.
// This module instantiate the internal SB_HFOSC clock source in the
// Lattice ice40 UP device. It then connects it to the PLL, and
// finally connects the output from the PLL to the global clock net.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module clk_reset_gen #(parameter RESET_CYCLES = 200)
  (
   output wire clk,
   output wire rst_n
   );


  //----------------------------------------------------------------
  // Registers with associated wires.
  //----------------------------------------------------------------
  reg [7 : 0] rst_ctr_reg = 8'h0;
  reg [7 : 0] rst_ctr_new;
  reg         rst_ctr_we;

  reg         rst_n_reg = 1'h0;
  reg         rst_n_new;

  wire        hfosc_clk;
  wire        pll_clk;


  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign rst_n = rst_n_reg;


  //----------------------------------------------------------------
  // Core instantiations.
  //----------------------------------------------------------------
  // Use the FPGA internal High Frequency OSCillator as clock source.
  // 00: 48MHz, 01: 24MHz, 10: 12MHz, 11: 6MHz
  /* verilator lint_off PINMISSING */
  SB_HFOSC #(.CLKHF_DIV("0b10")
	     ) u_hfosc (.CLKHFPU(1'b1),.CLKHFEN(1'b1),.CLKHF(hfosc_clk));
  /* verilator lint_on PINMISSING */


  // PLL to generate a new clock frequency based on the HFOSC clock.
  /* verilator lint_off PINMISSING */
  SB_PLL40_CORE #(
		  .FEEDBACK_PATH("SIMPLE"),
		  .DIVR(4'b0000),	// DIVR =  0
		  .DIVF(7'b0101111),	// DIVF = 47
		  .DIVQ(3'b101),	// DIVQ =  5
		  .FILTER_RANGE(3'b001)	// FILTER_RANGE = 1
		  ) uut (
			 .RESETB(1'b1),
			 .BYPASS(1'b0),
			 .REFERENCECLK(hfosc_clk),
			 .PLLOUTCORE(pll_clk)
			 );
  /* verilator lint_on PINMISSING */

  // Use a global buffer to distribute the clock.
  SB_GB SB_GB_i (
		 .USER_SIGNAL_TO_GLOBAL_BUFFER (pll_clk),
		 .GLOBAL_BUFFER_OUTPUT (clk)
		 );

  //----------------------------------------------------------------
  // reg_update.
  //----------------------------------------------------------------
    always @(posedge clk)
      begin : reg_update
        rst_n_reg <= rst_n_new;

        if (rst_ctr_we)
          rst_ctr_reg <= rst_ctr_new;
      end


  //----------------------------------------------------------------
  // rst_logic.
  //----------------------------------------------------------------
  always @*
    begin : rst_logic
      rst_n_new   = 1'h1;
      rst_ctr_new = 8'h0;
      rst_ctr_we  = 1'h0;

      if (rst_ctr_reg < RESET_CYCLES) begin
        rst_n_new   = 1'h0;
        rst_ctr_new = rst_ctr_reg + 1'h1;
        rst_ctr_we  = 1'h1;
      end
    end

endmodule // reset_gen

//======================================================================
// EOF reset_gen.v
//======================================================================
