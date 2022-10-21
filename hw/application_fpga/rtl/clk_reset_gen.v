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
  /* verilator lint_off PINMISSING */

  // Use the FPGA internal High Frequency OSCillator as clock source.
  // 00: 48MHz, 01: 24MHz, 10: 12MHz, 11: 6MHz
  SB_HFOSC #(.CLKHF_DIV("0b10")
	     ) hfosc_inst (.CLKHFPU(1'b1),.CLKHFEN(1'b1),.CLKHF(hfosc_clk));


  // Use a PLL to generate a new clock frequency based on the HFOSC clock.
  //
  // Given FEEDBACK_PATH=="SIMPLE", clock calculation according to 3.5.2 in
  // FPGA-TN-02052-1-4-iCE40-sysCLOCK-PLL-Design-User-Guide.pdf
  // https://www.latticesemi.com/view_document?document_id=47778 follows:
  //
  // F_pllout == (F_referenceclk * (DIVF + 1)) / (2^DIVQ * (DIVR + 1))
  //
  // Given the 12 MHz HFOSC clock set above, we get a final 18 MHz:
  //
  // (12000000 * (47 + 1)) / (2^5 * (0 + 1)) = 18000000
  SB_PLL40_CORE #(
		  .FEEDBACK_PATH("SIMPLE"),
		  .DIVR(4'b0000),	// DIVR =  0
		  .DIVF(7'b0101111),	// DIVF = 47
		  .DIVQ(3'b101),	// DIVQ =  5
		  .FILTER_RANGE(3'b001)	// FILTER_RANGE = 1
		  ) pll_inst (
			 .RESETB(1'b1),
			 .BYPASS(1'b0),
			 .REFERENCECLK(hfosc_clk),
			 .PLLOUTCORE(pll_clk)
			 );


  // Use a Global Buffer to distribute the clock.
  SB_GB gb_inst (
		 .USER_SIGNAL_TO_GLOBAL_BUFFER (pll_clk),
		 .GLOBAL_BUFFER_OUTPUT (clk)
		 );

  /* verilator lint_on PINMISSING */


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
