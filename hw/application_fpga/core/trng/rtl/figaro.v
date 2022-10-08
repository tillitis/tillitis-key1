//======================================================================
//
// figaro.v
// --------
// Top level wrapper for the figaro core.
//
//
// Author: Joachim Strombergson
// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
//
//======================================================================

`default_nettype none

module figaro(
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
  localparam ADDR_NAME0        = 8'h00;
  localparam ADDR_NAME1        = 8'h01;
  localparam ADDR_VERSION      = 8'h02;

  localparam ADDR_STATUS       = 8'h09;
  localparam STATUS_READY_BIT  = 0;

  localparam ADDR_SAMPLE_RATE  = 8'h10;

  localparam ADDR_ENTROPY      = 8'h20;

  localparam CORE_NAME0        = 32'h66696761; // "figa"
  localparam CORE_NAME1        = 32'h726f2020; // "ro  "
  localparam CORE_VERSION      = 32'h00000001;

  localparam SAMPLE_RATE       = 24'h0001000;


  //----------------------------------------------------------------
  // Registers.
  //----------------------------------------------------------------
  reg [23 : 0] sample_rate_ctr_reg;
  reg [23 : 0] sample_rate_ctr_new;

  reg [4 : 0]  bit_ctr_reg;
  reg [4 : 0]  bit_ctr_new;
  reg          bit_ctr_we;

  reg [63 : 0] entropy_reg;
  reg [63 : 0] entropy_new;
  reg          entropy_we;

  reg          ready_reg;
  reg          ready_new;
  reg          ready_we;
  reg          ready_set;
  reg          ready_rst;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg  [31 : 0] tmp_read_data;
  reg           tmp_ready;

  /* verilator lint_off UNOPTFLAT */
  wire [31 : 0] f;
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
    for(i = 0 ; i < 32 ; i = i + 1)
      begin: oscillators
	/* verilator lint_off PINMISSING */
	(* keep *) SB_LUT4 #(.LUT_INIT(16'h1)) osc_inv (.I0(f[i]), .O(f[i]));
	/* verilator lint_off PINMISSING */
      end
  endgenerate


  //---------------------------------------------------------------
  // reg_update
  //---------------------------------------------------------------
  always @(posedge clk)
     begin : reg_update
       if (!reset_n) begin
         sample_rate_ctr_reg <= 24'h0;
	 bit_ctr_reg         <= 6'h0;
	 entropy_reg         <= 64'h0;
	 ready_reg           <= 1'h0;
       end
       else begin
         sample_rate_ctr_reg <= sample_rate_ctr_new;

         if (bit_ctr_we) begin
           bit_ctr_reg <= bit_ctr_new;
         end

         if (entropy_we) begin
           entropy_reg <= entropy_new;
         end

	 if (ready_we) begin
	   ready_reg <= ready_new;
	 end
       end
     end


  //----------------------------------------------------------------
  // ready_logic
  //----------------------------------------------------------------
  always @*
    begin : ready_logic
      ready_new = 1'h0;
      ready_we  = 1'h0;

      if (ready_set) begin
	ready_new = 1'h1;
	ready_we  = 1'h1;

      end else if (ready_rst) begin
	ready_new = 1'h0;
	ready_we  = 1'h1;
      end
    end


  //----------------------------------------------------------------
  // entropy_logic
  //----------------------------------------------------------------
  always @*
    begin : entropy_logic
      bit_ctr_new = 6'h0;
      bit_ctr_we  = 1'h0;
      entropy_we  = 1'h0;
      ready_set   = 1'h0;

      entropy_new  = {entropy_reg[62 : 0], ^f};

      sample_rate_ctr_new = sample_rate_ctr_reg + 1'h1;
      if (sample_rate_ctr_reg == SAMPLE_RATE) begin
	sample_rate_ctr_new = 24'h0;
	entropy_we          = 1'h1;
	bit_ctr_new         = bit_ctr_reg + 1'h1;
	bit_ctr_we          = 1'h1;

	if (bit_ctr_reg == 6'h3f) begin
	  bit_ctr_new = 6'h0;
	  ready_set   = 1'h1;
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
      reg [31 : 0] entropy;

      ready_rst     = 1'h0;
      tmp_read_data = 32'h0;
      tmp_ready     = 1'h0;

      entropy = entropy_reg[63 : 32] ^ entropy_reg[31 : 0];

      if (cs) begin
	tmp_ready = 1'h1;
	if (!we) begin
          if (address == ADDR_STATUS) begin
            tmp_read_data = {31'h0, ready_reg};
          end

          if (address == ADDR_ENTROPY) begin
            tmp_read_data = entropy;
	    ready_rst     = 1'h1;
          end
	end
      end
    end // api
endmodule // figaro

//======================================================================
// EOF figaro.v
//======================================================================
