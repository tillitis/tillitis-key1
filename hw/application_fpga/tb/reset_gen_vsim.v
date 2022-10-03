//======================================================================
//
// reset_gen_vsim.v
// ----------------
// Reset generator Verilator simulation of the application_fpga.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module reset_gen #(parameter RESET_CYCLES = 200)
                 (
	          input wire  clk,
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


  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign rst_n = rst_n_reg;


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
