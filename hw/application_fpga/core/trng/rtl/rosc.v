//======================================================================
//
// rosc.v
// ------
// Digital ring oscillator based entropy generator.
// Use this as a source of entropy, for example as seeds.
// Do **NOT** use directly as random number in any security
// related use cases.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module rosc(
	    input wire           clk,
	    input wire           reset_n,

	    input wire           cs,
	    input wire           we,
	    input wire  [7 : 0]  address,
	    /* verilator lint_off UNUSED */
	    input wire  [31 : 0] write_data,
	    /* verilator lint_on UNUSED */
	    output wire [31 : 0] read_data,
	    output wire          ready
	   );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  // API
  localparam ADDR_STATUS       = 8'h09;
  localparam STATUS_READY_BIT  = 0;
  localparam ADDR_ENTROPY      = 8'h20;

  // Total number of ROSCs will be 2 x NUM_ROSC.
  localparam SAMPLE_CYCLES = 16'h1000;
  localparam NUM_ROSC      = 16;
  localparam SKIP_BITS     = 32;

  localparam CTRL_SAMPLE1    = 0;
  localparam CTRL_SAMPLE2    = 1;
  localparam CTRL_DATA_READY = 2;


  //----------------------------------------------------------------
  // Registers with associated wires.
  //----------------------------------------------------------------
  reg [15 : 0] cycle_ctr_reg;
  reg [15 : 0] cycle_ctr_new;
  reg          cycle_ctr_done;
  reg          cycle_ctr_rst;

  reg [7 : 0]  bit_ctr_reg;
  reg [7 : 0]  bit_ctr_new;
  reg          bit_ctr_inc;
  reg          bit_ctr_rst;
  reg          bit_ctr_we;

  reg [31 : 0] entropy_reg;
  reg [31 : 0] entropy_new;
  reg          entropy_we;

  reg [1 : 0]  sample1_reg;
  reg [1 : 0]  sample1_new;
  reg          sample1_we;

  reg [1 : 0]  sample2_reg;
  reg [1 : 0]  sample2_new;
  reg          sample2_we;

  reg          data_ready_reg;
  reg          data_ready_new;
  reg          data_ready_we;

  reg [1 : 0]  rosc_ctrl_reg;
  reg [1 : 0]  rosc_ctrl_new;
  reg          rosc_ctrl_we;

  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg  [31 : 0] tmp_read_data;
  reg           tmp_ready;

  /* verilator lint_off UNOPTFLAT */
  wire [(NUM_ROSC - 1) : 0] f;
  /* verilator lint_on UNOPTFLAT */

  /* verilator lint_off UNOPTFLAT */
  wire [(NUM_ROSC - 1) : 0] g;
  /* verilator lint_on UNOPTFLAT */


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //----------------------------------------------------------------
  // oscillators.
  //
  // 32 single inverters, each connect to itself.
  //----------------------------------------------------------------
  genvar i;
  generate
    for(i = 0 ; i < NUM_ROSC ; i = i + 1)
      begin: oscillators
	/* verilator lint_off PINMISSING */
	(* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv_f (.I0(f[i]), .O(f[i]));

	(* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv_g (.I0(g[i]), .O(g[i]));
	/* verilator lint_off PINMISSING */
      end
  endgenerate


  //---------------------------------------------------------------
  // reg_update
  //---------------------------------------------------------------
  always @(posedge clk)
     begin : reg_update
       if (!reset_n) begin
         cycle_ctr_reg  <= 16'h0;
	 bit_ctr_reg    <= 8'h0;
	 sample1_reg    <= 2'h0;
	 sample2_reg    <= 2'h0;
	 entropy_reg    <= 32'h0;
	 data_ready_reg <= 1'h0;
	 rosc_ctrl_reg  <= CTRL_SAMPLE1;
       end

       else begin
         cycle_ctr_reg <= cycle_ctr_new;

         if (bit_ctr_we) begin
           bit_ctr_reg <= bit_ctr_new;
         end

	 if (sample1_we) begin
	   sample1_reg <= sample1_new;
	 end

	 if (sample2_we) begin
	   sample2_reg <= sample2_new;
	 end

         if (entropy_we) begin
           entropy_reg <= entropy_new;
         end

	 if (data_ready_we) begin
	   data_ready_reg <= data_ready_new;
	 end

	 if (rosc_ctrl_we) begin
	   rosc_ctrl_reg <= rosc_ctrl_new;
	 end
       end
     end


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @*
    begin : api
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
    end // api


  //----------------------------------------------------------------
  // bit_ctr_logic
  //----------------------------------------------------------------
  always @*
    begin : bit_ctr_logic
      bit_ctr_new    = 8'h0;
      bit_ctr_we     = 1'h0;
      data_ready_new = 1'h0;
      data_ready_we  = 1'h0;

      if (bit_ctr_rst) begin
	bit_ctr_new    = 8'h0;
	bit_ctr_we     = 1'h1;
	data_ready_new = 1'h0;
	data_ready_we  = 1'h1;
      end
      else if (bit_ctr_inc) begin
	bit_ctr_new = bit_ctr_reg + 1'h1;
	bit_ctr_we  = 1'h1;

	if (bit_ctr_reg == SKIP_BITS) begin
	  data_ready_new = 1'h1;
	  data_ready_we  = 1'h1;
	end
      end
    end


  //----------------------------------------------------------------
  // cycle_ctr_logic
  //----------------------------------------------------------------
  always @*
    begin : cycle_ctr_logic
      cycle_ctr_new  = cycle_ctr_reg + 1'h1;
      cycle_ctr_done = 1'h0;

      if (cycle_ctr_rst) begin
	cycle_ctr_new = 16'h0;
      end

      if (cycle_ctr_reg == SAMPLE_CYCLES) begin
	cycle_ctr_done = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // rosc_ctrl_logic
  //----------------------------------------------------------------
  always @*
    begin : rosc_ctrl_logic
      reg xor_f;
      reg xor_g;
      reg xor_sample1;
      reg xor_sample2;

      sample1_we     = 1'h0;
      sample2_we     = 1'h0;
      entropy_we     = 1'h0;
      cycle_ctr_rst  = 1'h0;
      bit_ctr_inc    = 1'h0;
      rosc_ctrl_new  = CTRL_SAMPLE1;
      rosc_ctrl_we   = 1'h0;

      xor_f       = ^f;
      xor_g       = ^g;
      xor_sample1 = ^sample1_reg;
      xor_sample2 = ^sample2_reg;

      sample1_new = {sample1_reg[0], xor_f};
      sample2_new = {sample2_reg[0], xor_g};
      entropy_new = {entropy_reg[30 : 0], xor_sample1 ^ xor_sample2};

      case (rosc_ctrl_reg)
	CTRL_SAMPLE1: begin
	  if (cycle_ctr_done) begin
	    cycle_ctr_rst = 1'h1;
	    sample1_we    = 1'h1;
	    sample2_we    = 1'h1;
	    rosc_ctrl_new = CTRL_SAMPLE2;
	    rosc_ctrl_we  = 1'h1;
	  end
	end

	CTRL_SAMPLE2: begin
	  if (cycle_ctr_done) begin
	    cycle_ctr_rst = 1'h1;
	    sample1_we    = 1'h1;
	    sample2_we    = 1'h1;
	    rosc_ctrl_new = CTRL_DATA_READY;
	    rosc_ctrl_we  = 1'h1;
	  end
	end

	CTRL_DATA_READY: begin
	  entropy_we    = 1'h1;
	  bit_ctr_inc   = 1'h1;
	  rosc_ctrl_new = CTRL_SAMPLE1;
	  rosc_ctrl_we  = 1'h1;
	end

	default: begin
	end
      endcase // case (rosc_ctrl_reg)
    end

endmodule // rosc

//======================================================================
// EOF rosc.v
//======================================================================
