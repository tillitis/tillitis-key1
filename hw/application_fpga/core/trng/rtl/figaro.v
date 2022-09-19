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
              input wire  [31 : 0] write_data,
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


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg           core_read_entropy;
  reg           core_set_sample_rate;
  wire [31 : 0] core_entropy;
  wire          core_ready;
  reg  [31 : 0] tmp_read_data;
  reg           tmp_ready;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign ready     = tmp_ready;


  //----------------------------------------------------------------
  // core instantiation.
  //----------------------------------------------------------------
  figaro_core core(
                   .clk(clk),
                   .reset_n(reset_n),
                   .read_entropy(core_read_entropy),
                   .set_sample_rate(core_set_sample_rate),
                   .sample_rate(write_data[23 : 0]),
                   .entropy(core_entropy),
                   .ready(core_ready)
                  );


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @*
    begin : api
      core_read_entropy    = 1'h0;
      core_set_sample_rate = 1'h0;
      tmp_read_data        = 32'h0;
      tmp_ready            = 1'h0;

      if (cs) begin
	tmp_ready = 1'h1;

	if (we) begin
	  if (address == ADDR_SAMPLE_RATE) begin
	    core_set_sample_rate = 1'h1;
          end
	end

	else begin
	  if (address == ADDR_NAME0) begin
	    tmp_read_data = CORE_NAME0;
          end

	  if (address == ADDR_NAME1) begin
	    tmp_read_data = CORE_NAME1;
	  end

	  if (address == ADDR_VERSION) begin
	    tmp_read_data = CORE_VERSION;
	  end

          if (address == ADDR_STATUS) begin
            tmp_read_data = {31'h0, core_ready};
          end

          if (address == ADDR_ENTROPY) begin
            tmp_read_data     = core_entropy;
            core_read_entropy = 1'h1;
          end
	end
      end
    end // api
endmodule // figaro

//======================================================================
// EOF figaro.v
//======================================================================
