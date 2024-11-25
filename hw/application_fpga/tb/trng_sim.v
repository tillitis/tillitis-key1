//======================================================================
//
// trng_sim.v
// ----------
// TRNG simulation of the application_fpga.
//
//
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module trng_sim (
    input wire clk,
    input wire reset_n,

    input  wire          cs,
    input  wire          we,
    input  wire [ 7 : 0] address,
    /* verilator lint_off UNUSED */
    input  wire [31 : 0] write_data,
    /* verilator lint_on UNUSED */
    output wire [31 : 0] read_data,
    output wire          ready
);


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  // API
  localparam ADDR_STATUS = 8'h09;
  localparam ADDR_ENTROPY = 8'h20;

  // Total number of ROSCs will be 2 x NUM_ROSC.
  localparam SAMPLE_CYCLES = 16'h1000;
  localparam NUM_ROSC = 16;
  localparam SKIP_BITS = 32;

  localparam CTRL_SAMPLE1 = 0;
  localparam CTRL_SAMPLE2 = 1;
  localparam CTRL_DATA_READY = 2;


  //----------------------------------------------------------------
  // Registers with associated wires.
  //----------------------------------------------------------------
  reg [15 : 0] cycle_ctr_reg;
  reg [15 : 0] cycle_ctr_new;
  reg cycle_ctr_done;
  reg cycle_ctr_rst;

  reg [7 : 0] bit_ctr_reg;
  reg [7 : 0] bit_ctr_new;
  reg bit_ctr_inc;
  reg bit_ctr_rst;
  reg bit_ctr_we;

  reg [31 : 0] entropy_reg;
  reg [31 : 0] entropy_new;
  reg entropy_we;

  reg [1 : 0] sample1_reg;
  reg [1 : 0] sample1_new;
  reg sample1_we;

  reg [1 : 0] sample2_reg;
  reg [1 : 0] sample2_new;
  reg sample2_we;

  reg data_ready_reg;
  reg data_ready_new;
  reg data_ready_we;

  reg [1 : 0] trng_ctrl_reg;
  reg [1 : 0] trng_ctrl_new;
  reg trng_ctrl_we;

  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] tmp_read_data;
  reg tmp_ready;

  /* verilator lint_off UNOPTFLAT */
  wire [(NUM_ROSC - 1) : 0] f;
  /* verilator lint_on UNOPTFLAT */

  /* verilator lint_off UNOPTFLAT */
  wire [(NUM_ROSC - 1) : 0] g;
  /* verilator lint_on UNOPTFLAT */

  // Simulation of TRNG with 32-bit LFSR with polynomial: x^32 + x^22 + x^2 + x + 1
  wire feedback = entropy_reg[31] ^ entropy_reg[21] ^ entropy_reg[1] ^ entropy_reg[0];
  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //---------------------------------------------------------------
  // reg_update
  //---------------------------------------------------------------
  always @(posedge clk) begin : reg_update
    if (!reset_n) begin
      cycle_ctr_reg  <= 16'h0;
      bit_ctr_reg    <= 8'h0;
      sample1_reg    <= 2'h0;
      sample2_reg    <= 2'h0;
      entropy_reg    <= 32'hDEADBEEF; // Reset LFSR to a non-zero seed
      data_ready_reg <= 1'h1;
      trng_ctrl_reg  <= CTRL_SAMPLE1;
    end
    else begin
      if (cs) begin
        if (!we) begin
          if (address == ADDR_ENTROPY) begin
            entropy_reg <= {entropy_reg[30:0], feedback};  // Shift left with feedback
          end
        end
      end
    end
  end


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @* begin : api
    bit_ctr_rst   = 1'h0;
    tmp_read_data = 32'h0;
    tmp_ready     = 1'h0;

    if (cs) begin
      tmp_ready = 1'h1;

      if (!we) begin
        if (address == ADDR_STATUS) begin
          tmp_read_data = {31'h0, data_ready_reg};
        end

        if (address == ADDR_ENTROPY) begin
          tmp_read_data = entropy_reg;
          bit_ctr_rst   = 1'h1;
        end
      end
    end
  end  // api


endmodule  // trng

//======================================================================
// EOF trng_sim.v
//======================================================================
